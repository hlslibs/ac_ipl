/**************************************************************************
 *                                                                        *
 *  Algorithmic C (tm) Image Processing Library                           *
 *                                                                        *
 *  Software Version: 2025.4                                              *
 *                                                                        *
 *  Release Date    : Tue Nov 11 18:01:30 PST 2025                        *
 *  Release Type    : Production Release                                  *
 *  Release Build   : 2025.4.0                                            *
 *                                                                        *
 *  Copyright 2019 Siemens                                                *
 *                                                                        *
 **************************************************************************
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      * 
 *  You may obtain a copy of the License at                               *
 *                                                                        *
 *      http://www.apache.org/licenses/LICENSE-2.0                        *
 *                                                                        *
 *  Unless required by applicable law or agreed to in writing, software   * 
 *  distributed under the License is distributed on an "AS IS" BASIS,     * 
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or       *
 *  implied.                                                              * 
 *  See the License for the specific language governing permissions and   * 
 *  limitations under the License.                                        *
 **************************************************************************
 *                                                                        *
 *  The most recent version of this package is available at github.       *
 *                                                                        *
 *************************************************************************/
#ifndef _INCLUDED_AC_CANNY_H_
#define _INCLUDED_AC_CANNY_H_

#include <ac_int.h>
#include <ac_fixed.h>
#include <ac_window_2d_flag.h>
#include <ac_math/ac_sqrt_pwl.h>
#include <ac_math/ac_reciprocal_pwl.h>
#include <ac_math/ac_atan_pwl.h>
#include <ac_channel.h>
#include <mc_scverify.h>

template <unsigned CDEPTH, unsigned W_MAX, unsigned H_MAX, bool USE_SINGLEPORT = false>
class ac_canny
{
public:
  // Define IO types.
  typedef ac_int<CDEPTH, false> pixInType;
  typedef ac_int<1, false> pixOutType;
  typedef ac_int<ac::nbits<W_MAX>::val, false> widthInType;
  typedef ac_int<ac::nbits<H_MAX>::val, false> heightInType;

  #pragma hls_design interface
  void CCS_BLOCK(run) (
    ac_channel<pixInType>  &streamIn,   // Pixel input stream
    ac_channel<pixOutType> &streamOut,  // Pixel output stream
    const widthInType      widthIn,     // Input width
    const heightInType     heightIn,    // Input height
    const pixInType        threshLowIn, // Lower threshold for hysteresis
    const pixInType        threshUppIn  // Upper threshold for hysteresis
  ) {
    gaussFilter(streamIn, P1, widthIn, heightIn);
    edgeFilter(P1, P2, P3, widthIn, heightIn);
    NMS(P2, P3, P4, widthIn, heightIn);
    hysThresh(P4, streamOut, widthIn, heightIn, threshLowIn, threshUppIn);
  }

  ac_canny() { }

private:
  // NFRAC_BITS: Fractional bits used to store the results of fixed point intermediate calculations/coefficients. For simplicity's sake,
  // the fixed point calculation results/coefficients all have the same number of fractional bits.
  // FILT_WMODE: Windowing mode of internal ac_window objects which are used in 2D filtering, i.e. Gaussian and Edge detect windows.
  // OTHER_WMODE: Windowing mode of ac_window objects which are used for functions other than 2D filtering, i.e. NMS and hysThresh windows.
  enum {
    NFRAC_BITS = 16,
    FILT_WMODE = USE_SINGLEPORT ? AC_MIRROR | AC_SINGLEPORT : AC_MIRROR,
    OTHER_WMODE = USE_SINGLEPORT ? AC_BOUNDARY | AC_SINGLEPORT : AC_BOUNDARY,
  };

  // Add a fixed, arbitrary number of fractional bits(16) for gaussOpType and magOpType. The number of integer bits is chosen
  // based on the filter coefficient values used by default, namely, a 5x5, matlab-generated Gaussian filter, as
  // well as a 3x3 sobel edge detection filter.
  // If you use a different type of filter(s), consider changing the integer bitwidth to suit the filter coefficients.
  typedef ac_fixed<NFRAC_BITS + CDEPTH, CDEPTH, false> gaussOpType; // Type for Gaussian filter output.
  typedef ac_fixed<NFRAC_BITS + CDEPTH + 3, CDEPTH + 3, false> magOpType; // Type for edge magnitude output.
  // The angle of the edge is rounded to four possible values, which are represented by four bit patterns:
  // 0/0b00: 0 degrees, 1/0b01: 45 degrees, 2/0b10: 90 degrees, 3/0b11: 135 degrees.
  // A 2-bit ac_int value is hence deemed suitable to store edge angle values.
  typedef ac_int<2, false> angOpType;

  #pragma hls_pipeline_init_interval 1
  #pragma hls_design
  void gaussFilter(
    ac_channel<pixInType>   &streamIn,
    ac_channel<gaussOpType> &gaussOut, // Gaussian filter output.
    const widthInType       widthIn,
    const heightInType      heightIn
  ) {
    const unsigned GK_SZ = 5;

    const ac_fixed<NFRAC_BITS, 0, false> B[GK_SZ][GK_SZ] = {
      {0.012146, 0.026110, 0.033697, 0.026110, 0.012146},
      {0.026110, 0.056127, 0.072438, 0.056127, 0.026110},
      {0.033697, 0.072438, 0.093487, 0.072438, 0.033697},
      {0.026110, 0.056127, 0.072438, 0.056127, 0.026110},
      {0.012146, 0.026110, 0.033697, 0.026110, 0.012146}
    };

    // Declare window object to store input pixel values for gaussian filtering.
    ac_window_2d_flag<pixInType, GK_SZ, GK_SZ, W_MAX, FILT_WMODE> acWindObj;

    ac_int<ac::nbits<H_MAX>::val, false> i = 0;
    ac_int<ac::nbits<W_MAX>::val, false> j = 0;

    bool inRead = true, eofOut = false;

    #pragma hls_pipeline_init_interval 1
    GAUSS_PROC_LOOP: do {
      pixInType pixIn = inRead ? streamIn.read() : pixInType(0);
      // Calculate sof, eof, sol and eol based on image coordinates.
      bool sol = (j == 0);
      bool sof = (i == 0) && sol;
      bool eol = (j == widthIn - 1);
      bool eof = (i == heightIn - 1) && eol;
      // Write input frame value to the window object.
      acWindObj.write(pixIn, sof, eof, sol, eol);
      if (eof) {
        inRead = false; // Stop reading input channel after valid image region.
      }
      j++; // Update column counter after every pixel received.
      if (j == widthIn) {
        j = 0; // Max. possible j value = widthIn - 1.
        i++; // Update row counter after every line received.
        if (i == heightIn) {
          i = 0; // Max. possible i value = heightIn - 1.
        }
      }

      bool sofOut, solOut, eolOut;
      acWindObj.readFlags(sofOut, eofOut, solOut, eolOut);
      if (acWindObj.valid()) {
        // If the window has ramped up, call windFilt() to produce a filtered output.
        gaussOpType gaussOp = windFilt<gaussOpType> (B, acWindObj);
        gaussOut.write(gaussOp);
      }
    } while (!eofOut); // Stop processing once the entire image output has been read.
  }

  #pragma hls_pipeline_init_interval 1
  #pragma hls_design
  void edgeFilter(
    ac_channel<gaussOpType> &gaussOut, // Gaussian filter output.
    ac_channel<magOpType>   &magOut,   // Edge magnitude output.
    ac_channel<angOpType>   &angOut,   // Edge angle/direction output.
    const widthInType       widthIn,
    const heightInType      heightIn
  ) {
    // The pre-calculated bitwidths for the output of edge detected depend on the supplied 3x3 sobel edge filter implementation.
    // Any other implmentation might require different bitwidths.
    typedef ac_fixed<magOpType::width, magOpType::i_width, true> edgeFiltOpType;

    // Define kernels for edge detect filter. In this case, two 3x3 sobel edge detect kernels are used.
    const unsigned EK_SZ = 3;
    const ac_int<3, true> KGx[EK_SZ][EK_SZ] = {
      {1, 0, -1},
      {2, 0, -2},
      {1, 0, -1}
    };
    const ac_int<3, true> KGy[EK_SZ][EK_SZ] = {
      {-1, -2, -1},
      { 0,  0,  0},
      { 1,  2,  1}
    };
    
    // Declare window object to store input pixel values for edge detection.
    ac_window_2d_flag<gaussOpType, EK_SZ, EK_SZ, W_MAX, FILT_WMODE> acWindObj;

    ac_int<ac::nbits<H_MAX>::val, false> i = 0;
    ac_int<ac::nbits<W_MAX>::val, false> j = 0;

    bool inRead = true, eofOut = false;
    
    // The mechanism by which EDGE_PROC_LOOP operates to read input values and flush out all filtered outputs
    // is very similar to the mechanism of GAUSS_PROC_LOOP. Please refer to the functioning of
    // that, to get a better understanding of how the edge detection loop works.
    #pragma hls_pipeline_init_interval 1
    EDGE_PROC_LOOP: do {
      gaussOpType gaussOp = inRead ? gaussOut.read() : gaussOpType(0.0);
      bool sol = (j == 0);
      bool sof = (i == 0) && sol;
      bool eol = (j == widthIn - 1);
      bool eof = (i == heightIn - 1) && eol;
      acWindObj.write(gaussOp, sof, eof, sol, eol);
      if (eof) {
        inRead = false;
      }
      j++;
      if (j == widthIn) {
        j = 0;
        i++;
        if (i == heightIn) {
          i = 0;
        }
      }

      bool sofOut, solOut, eolOut;
      acWindObj.readFlags(sofOut, eofOut, solOut, eolOut);
      if (acWindObj.valid()) {
        // Calculate horizontal and vertical image derivatives.
        edgeFiltOpType Gx = windFilt<edgeFiltOpType> (KGx, acWindObj);
        edgeFiltOpType Gy = windFilt<edgeFiltOpType> (KGy, acWindObj);
        magOpType magOp;
        angOpType angOp;
        // Calculate magnitude of the edge, as well as its direction.
        edgeOpCalc(Gx, Gy, magOp, angOp);
        magOut.write(magOp);
        angOut.write(angOp);
      }
    } while (!eofOut); // Stop processing once the entire image output has been read.
  }

  // Non-maximum suppression block.
  #pragma hls_pipeline_init_interval 1
  #pragma hls_design
  void NMS(
    ac_channel<magOpType> &magOut,
    ac_channel<angOpType> &angOut,
    ac_channel<pixInType> &NMS_magOut, // Non-maximum suppressed (NMS) magnitude output.
    const widthInType     widthIn,
    const heightInType    heightIn
  ) {
    // A 3x3 window is used for storing edge magnitude values, and a 2x2 window is used to store edge
    // angle values, with the design only using the top left window value for non-maximum suppression.
    // Both windows assume zero padding.
    ac_window_2d_flag<magOpType, 3, 3, W_MAX, OTHER_WMODE> acWindMagObj(0.0);
    ac_window_2d_flag<angOpType, 2, 2, W_MAX, OTHER_WMODE> acWindAngObj(0.0);

    ac_int<ac::nbits<H_MAX>::val, false> i = 0;
    ac_int<ac::nbits<W_MAX>::val, false> j = 0;

    bool inRead = true, eofOut = false;
    
    // The mechanism by which NMS_PROC_LOOP operates to read input values and flush out all outputs
    // is very similar to the mechanism of GAUSS_PROC_LOOP. Please refer to the functioning of
    // that, to get a better understanding of how the NMS processing loop works.
    #pragma hls_pipeline_init_interval 1
    NMS_PROC_LOOP: do {
      magOpType magOp = inRead ? magOut.read() : magOpType(0.0);
      angOpType angOp = inRead ? angOut.read() : angOpType(0);
      bool sol = (j == 0);
      bool sof = (i == 0) && sol;
      bool eol = (j == widthIn - 1);
      bool eof = (i == heightIn - 1) && eol;
      acWindMagObj.write(magOp, sof, eof, sol, eol);
      acWindAngObj.write(angOp, sof, eof, sol, eol);
      if (eof) {
        inRead = false;
      }
      j++;
      if (j == widthIn) {
        j = 0;
        i++;
        if (i == heightIn) {
          i = 0;
        }
      }

      bool sofOut, solOut, eolOut;
      acWindMagObj.readFlags(sofOut, eofOut, solOut, eolOut);
      if (acWindMagObj.valid()) {
        pixInType NMS_magOut_temp;
        // Calculate NMS output using NMS_outCalc function.
        NMS_outCalc(acWindMagObj, acWindAngObj, NMS_magOut_temp);
        NMS_magOut.write(NMS_magOut_temp);
      }
    } while (!eofOut); // Stop processing once the entire image output has been read.
  }

  #pragma hls_pipeline_init_interval 1
  #pragma hls_design
  void hysThresh(
    ac_channel<pixInType>  &NMS_magOut,
    ac_channel<pixOutType> &streamOut, // Output of the canny edge detector/hysteresis edge tracker.
    const widthInType      widthIn,
    const heightInType     heightIn,
    const pixInType        threshLowIn,
    const pixInType        threshUppIn
  ) {
    // Window object used for blob analysis. Also assumes zero padding.
    ac_window_2d_flag<pixInType, 3, 3, W_MAX, OTHER_WMODE> acWindObj(0);

    ac_int<ac::nbits<H_MAX>::val, false> i = 0;
    ac_int<ac::nbits<W_MAX>::val, false> j = 0;

    bool inRead = true, eofOut = false;
    
    // The mechanism by which HYS_PROC_LOOP operates to read input values and flush out all outputs
    // is very similar to the mechanism of GAUSS_PROC_LOOP. Please refer to the functioning of
    // that, to get a better understanding of how the hysterisis thresholding loop works.
    #pragma hls_pipeline_init_interval 1
    HYS_PROC_LOOP: do {
      pixInType NMS_op = inRead ? NMS_magOut.read() : pixInType(0);
      bool sol = (j == 0);
      bool sof = (i == 0) && sol;
      bool eol = (j == widthIn - 1);
      bool eof = (i == heightIn - 1) && eol;
      acWindObj.write(NMS_op, sof, eof, sol, eol);
      if (eof) {
        inRead = false;
      }
      j++;
      if (j == widthIn) {
        j = 0;
        i++;
        if (i == heightIn) {
          i = 0;
        }
      }

      bool sofOut, solOut, eolOut;
      acWindObj.readFlags(sofOut, eofOut, solOut, eolOut);
      if (acWindObj.valid()) {
        // The hysCalc() function calculates the hysteresis edge tracking output.
        pixOutType hysOp = hysCalc(acWindObj, threshLowIn, threshUppIn);
        // The hysteresis edge tracking output is the final output of the canny edge detector.
        streamOut.write(hysOp);
      }
    } while (!eofOut); // Stop processing once the entire image output has been read.
  }

// Carry out filtering with kernel and window values.
  template<class filtOpType, class acWindType, class kType, int K_SZ, int IN_WMODE>
  filtOpType windFilt(
    const kType (&kernel)[K_SZ][K_SZ],
    const ac_window_2d_flag<acWindType, K_SZ, K_SZ, W_MAX, IN_WMODE> &acWindObj
  ) {
    filtOpType filtOp = 0.0;
    // Window output is stored in temporary array. While you can also directly multiply values from
    // acWindObj and get the same QofR, having a temporary array can be more convenient for debugging.
    acWindType acWindOut[K_SZ][K_SZ];
    #pragma hls_unroll yes
    CONV_OP_ROW_LOOP: for (int r = 0; r < int(K_SZ); r++) {
      #pragma hls_unroll yes
      CONV_OP_COL_LOOP: for (int c = 0; c < int(K_SZ); c++) {
        acWindOut[r][c] = acWindObj(r - (K_SZ/2), c - (K_SZ/2));
        filtOp += acWindOut[r][c]*kernel[r][c];
      }
    }
    return filtOp;
  }

// Calculate edge magnitude and angle/direction.
  template<class edgeFiltOpType>
  void edgeOpCalc(const edgeFiltOpType &Gx, const edgeFiltOpType &Gy, magOpType &magOp, angOpType &angOp) {
    enum {
      W_ = edgeFiltOpType::width,
      I_ = edgeFiltOpType::i_width
    };
    // Use an intermediate variable to store (magnitude output)^2.
    ac_fixed<NFRAC_BITS + 2*I_, 2*I_, false> magOpSquared = (Gx*Gx + Gy*Gy);
    // Calculate edge magnitude output.
    magOp = ac_math::ac_sqrt_pwl<magOpType>(magOpSquared);
    const ac_fixed<NFRAC_BITS + 1, 1, false> threshAngles[2] = {0.3927, 1.1781}; // 22.5 and 67.5 degrees, respectively, stored in radians.
    if (Gx == 0) {
      angOp = 2; // If the derivative in the horizontal direction is zero, then it can automatically be assumed that the edge points upward/downward. Map the angle to 90 degrees in such a case.
    } else {
      typedef ac_fixed<W_, I_, false> absType;
      absType absGx = Gx < 0 ? absType(-Gx) : absType(Gx);
      absType absGy = Gy < 0 ? absType(-Gy) : absType(Gy);
      typedef ac_fixed<NFRAC_BITS + (W_ - I_) + 1, (W_ - I_) + 1, false> recipType;
      // Find abs(1/Gx)
      recipType recipAbsGx = ac_math::ac_reciprocal_pwl<recipType>(absGx);
      ac_fixed<NFRAC_BITS + 8,  8, false, AC_RND, AC_SAT> tanTheta = absGy*recipAbsGx; // abs(Gy/Gx) = abs(Gy)*abs(1/Gx).
      typedef ac_fixed<NFRAC_BITS + 1,  1, false> thetaType;
      // Calculate the angle of the edge, assuming that it is in the first quadrant.
      thetaType theta = ac_math::ac_atan_pwl<thetaType>(tanTheta);
      if (theta < threshAngles[0]) {
        angOp = 0; // If theta is less than 22.5 degrees, map angle to 0 degrees.
      } else if (theta < threshAngles[1]) {
        if ((Gx >= 0 && Gy >= 0) || (Gx < 0 && Gy < 0)) {
          angOp = 1; // If theta lies between 22.5 and 67.5 degrees, while also lying in the 1st/3rd quadrant, map the angle to 45 degrees.
        } else {
          angOp = 3; // If theta lies between 22.5 and 67.5 degrees, while also lying in the 2nd/4th quadrant, map the angle to 135 degrees.
        }
      } else {
        angOp = 2; // If none of the other conditions are satisfied, the angle is mapped to 90 degrees.
      }
    }
  }

// Calculate NMS output.
  template <int IN_WMODE>
  void NMS_outCalc(
    const ac_window_2d_flag<magOpType, 3, 3, W_MAX, IN_WMODE> &acWindMagObj,
    const ac_window_2d_flag<angOpType, 2, 2, W_MAX, IN_WMODE> &acWindAngObj,
    pixInType &NMS_magOut_temp
  ) {
    magOpType acWindMagOut[3][3], NMS_magOutFi;
    // Extract magnitude outputs in a 3x3 window.
    #pragma hls_unroll yes
    NMS_OP_ROW_LOOP: for (int r = 0; r < 3; r++) {
      #pragma hls_unroll yes
      NMS_OP_COL_LOOP: for (int c = 0; c < 3; c++) {
        acWindMagOut[r][c] = acWindMagObj(r - 1, c - 1);
      }
    }
    // The window for the edge angle outputs is a 2x2 window. The top left value coincides with the angle output for the center of the
    // 3x3 magnitude window.
    angOpType acWindAngOut = acWindAngObj(-1, -1);
    // Carry out non-maximal suppression with the center pixel and two of the neighboring pixel values. The neighboring pixel values
    // are chosen based on the orientation of the edge.
    if (acWindAngOut == 0) {
      NMS_magOutFi = NMS_findMax(acWindMagOut[1][1], acWindMagOut[1][0], acWindMagOut[1][2]); // Edge is oriented in east-west direction.
    } else if (acWindAngOut == 1) {
      NMS_magOutFi = NMS_findMax(acWindMagOut[1][1], acWindMagOut[0][2], acWindMagOut[2][0]); // Edge is oriented in northeast-southwest direction.
    } else if (acWindAngOut == 2) {
      NMS_magOutFi = NMS_findMax(acWindMagOut[1][1], acWindMagOut[0][1], acWindMagOut[2][1]); // Edge is oriented in north-south direction.
    } else if (acWindAngOut == 3) {
      NMS_magOutFi = NMS_findMax(acWindMagOut[1][1], acWindMagOut[0][0], acWindMagOut[2][2]); // Edge is oriented in northwest-southeast direction.
    }
    // Convert fixed point NMS output to an integer. The integer bitwidth is the same as the pixel color depth. Saturation and rounding is turned on for this conversion.
    NMS_magOut_temp = (ac_fixed<CDEPTH, CDEPTH, false, AC_RND, AC_SAT>(NMS_magOutFi)).to_int();
  }

// The first pixel input to NMS_findMax is always the center pixel, while the other two are neighboring pixel values.
  magOpType NMS_findMax(const magOpType &centerPix, const magOpType &neighborPix1, const magOpType &neighborPix2) {
    // Find the maximum of all three function inputs.
    magOpType maxMagOp = centerPix > neighborPix1 ? centerPix : neighborPix1;
    maxMagOp = maxMagOp > neighborPix2 ? maxMagOp : neighborPix2;
    // If the center pixel has a value as large as/larger than its neighbors, return the center pixel value as is. If not, return 0.
    return centerPix == maxMagOp ? centerPix : magOpType(0);
  }

// Calculate hysteresis edge tracking output by applying blob analysis.
  template <int IN_WMODE>
  pixOutType hysCalc(
    const ac_window_2d_flag<pixInType, 3, 3, W_MAX, IN_WMODE> &acWindObj,
    const pixInType threshLowIn,
    const pixInType threshUppIn
  ) {
    pixInType acWindOut[3][3];
    pixOutType hysOp = 0;
    bool higherThanThresh = false;
    #pragma hls_unroll yes
    HYS_OP_ROW_LOOP: for (int r = 0; r < 3; r++) {
      #pragma hls_unroll yes
      HYS_OP_COL_LOOP: for (int c = 0; c < 3; c++) {
        acWindOut[r][c] = acWindObj(r - 1, c - 1);
        // If a pixel lies between the upper and lower thresholds, we must look at the 8-connected neighbor pixels to figure whether any of those lie above the upper threshold.
        // The higherThanThresh flag is set to true if that is the case.
        if (r != 1 && c != 1) {
          higherThanThresh = higherThanThresh || (acWindOut[r][c] > threshUppIn);
        }
      }
    }

    if (acWindOut[1][1] > threshUppIn) {
      hysOp = 1;
    } else if (acWindOut[1][1] > threshLowIn && higherThanThresh) {
      hysOp = 1;
    }
    return hysOp;
  }

  ac_channel<gaussOpType> P1; // Interconnect channel with gaussian filter output.
  ac_channel<magOpType>   P2; // Interconnect channel with magnitude output from edge detector
  ac_channel<angOpType>   P3; // Interconnect channel with angle output from edge detector
  ac_channel<pixInType>   P4; // Interconnect channel with NMS magnitude output
};

#endif // #ifndef _INCLUDED_AC_CANNY_H_
