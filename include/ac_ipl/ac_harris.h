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

#ifndef _INCLUDED_AC_HARRIS_H_
#define _INCLUDED_AC_HARRIS_H_

#include <ac_int.h>
#include <ac_fixed.h>
#include <ac_ipl/ac_pixels.h>
#include <ac_window_2d_flag.h>
#include <ac_math/ac_reciprocal_pwl.h>
#include <ac_channel.h>
#include <mc_scverify.h>

// helper struct
template<int N>
struct max_s {
  template<typename T>
  static T max(T *a) {
    T m0 = max_s<N/2>::max(a);
    T m1 = max_s<N-N/2>::max(a + N/2);
    return m0 > m1 ? m0 : m1;
  }
};
// terminate template recursion
template<> struct max_s<1> {
  template<typename T>
  static T max(T *a) {
    return a[0];
  }
};

// Function to calculate maximum value in an array
template<int N, typename T>
T max(T *a)
{
  return max_s<N>::max(a);
}

#pragma hls_design top
template <class IN_TYPE, class OUT_TYPE, unsigned CDEPTH, unsigned W_MAX, unsigned H_MAX, bool USE_SINGLEPORT = false>
class ac_harris
{
public:
  // NFRAC_BITS: Fractional bits used to store the results of fixed point intermediate calculations/coefficients.
  // INTERNAL_WMODE: singleport and dualport implementations are supported.
  // GK_SZ: window size for the Gaussian filter
  // EK_SZ: window size for the derivative masks and for finding the local maxima
  enum {
    NFRAC_BITS = 16,
    GK_SZ = 5,
    EK_SZ = 3,
    INTERNAL_WMODE = USE_SINGLEPORT ? AC_BOUNDARY | AC_SINGLEPORT : AC_BOUNDARY
  };
  // Dimension types are bitwidth-constrained according to the max dimensions possible.
  typedef ac_int<ac::nbits<W_MAX>::val, false> widthInType;
  typedef ac_int<ac::nbits<H_MAX>::val, false> heightInType;
  typedef ac_int<2, false>                     componentType;
  typedef ac_int<3, false>                     epsilonType;
  typedef ac_int<14, false>                    thresholdType;

  #pragma hls_design interface
  void CCS_BLOCK(run) (
    ac_channel<IN_TYPE>  &streamIn,   // Pixel input stream
    ac_channel<OUT_TYPE> &streamOut,  // Pixel output stream
    const widthInType    widthIn,     // Input width
    const heightInType   heightIn,    // Input height
    const componentType  component,    // Component type
    const epsilonType    epsilon,
    const thresholdType  threshold
  ) {
    intensity(streamIn, P1, P2, widthIn, heightIn, component);
    harrisresponse(P1, P2, P3, widthIn, heightIn, epsilon);
    localmaxima(P3, P4, P5, widthIn, heightIn);
    thresholding(P4, P5, streamOut, widthIn, heightIn, threshold);
  }

  ac_harris() { }

private:
  typedef ac_int<CDEPTH, false> CompType;
  typedef ac_int<CDEPTH + 2, true> IntensityType; // Type for intensity output.
  typedef ac_int<(2*CDEPTH) + 4, true> IntensitySqType;
  typedef ac_fixed<NFRAC_BITS + (2*CDEPTH) + 4, (2*CDEPTH) + 4, true> gaussOpType; // Type for Gaussian filter output .
  typedef ac_fixed<NFRAC_BITS + (4*CDEPTH) + 10, (4*CDEPTH) + 10, true> HarrisResType;

  #pragma hls_pipeline_init_interval 1
  #pragma hls_design
  void intensity(
    ac_channel<IN_TYPE>          &streamIn,
    ac_channel<IntensityType>    &intensityx,
    ac_channel<IntensityType>    &intensityy,
    const widthInType            widthIn,
    const heightInType           heightIn,
    const componentType          component
  ) {
    CompType comp;
    // Define derivative masks.
    const ac_int<2, true> Dx[EK_SZ][EK_SZ] = {
      {-1, 0, 1},
      {-1, 0, 1},
      {-1, 0, 1}
    };
    const ac_int<2, true> Dy[EK_SZ][EK_SZ] = {
      {-1, -1, -1},
      { 0,  0,  0},
      { 1,  1,  1}
    };
    // Declare window object to store input pixel values. The window uses zero padding.
    ac_window_2d_flag<CompType, EK_SZ, EK_SZ, W_MAX, INTERNAL_WMODE> acWindObj(0);

    // Because the window needs (EK_SZ/2) input rows to ramp up and start producing outputs,
    // extra cycles are provided for after all the inputs are read, to flush out
    // all valid filter outputs. This is done by adding an extra (EK_SZ/2) iterations for
    // INIT_ROW_LOOP and INIT_COL_LOOP.
    #pragma hls_pipeline_init_interval 1
    INT_ROW_LOOP: for (unsigned i = 0; i < H_MAX + (EK_SZ/2); i++) {
      #pragma hls_pipeline_init_interval 1
      INT_COL_LOOP: for (unsigned j = 0; j < W_MAX + (EK_SZ/2); j++) {
        bool prod_output = (i != heightIn + (EK_SZ/2) - 1 && j < widthIn) || (i == heightIn + (EK_SZ/2) - 1);
        if (i < heightIn && j < widthIn) {
          IN_TYPE pixIn = streamIn.read();
          extractcomp(pixIn, comp, component);
          bool sol = (j == 0);
          bool sof = (i == 0) && sol;
          bool eol = (j == widthIn - 1);
          bool eof = (i == heightIn - 1) && eol;
          acWindObj.write(comp, sof, eof, sol, eol);
          // acWindInObj.write(pixIn, sof, eof, sol, eol);
        } else if (prod_output) {
          bool sol = (j == 0);
          bool eol = (j == widthIn - 1);
          acWindObj.write(0.0, false, false, sol, eol);
        }
        if (acWindObj.valid() && prod_output) {
          // Calculate horizontal and vertical image intensities.
          IntensityType   Ix  = windFilt<IntensityType> (Dx, acWindObj);
          IntensityType   Iy  = windFilt<IntensityType> (Dy, acWindObj);
          intensityx.write(Ix);
          intensityy.write(Iy);
        }
        if (j == widthIn + (EK_SZ/2) - 1) { break; }
      }
      if (i == heightIn + (EK_SZ/2) - 1) { break; }
    }
  }

  #pragma hls_pipeline_init_interval 1
  #pragma hls_design
  void harrisresponse(
    ac_channel<IntensityType>    &intensityx,
    ac_channel<IntensityType>    &intensityy,
    ac_channel<HarrisResType>    &harrisres,
    const widthInType            widthIn,
    const heightInType           heightIn,
    const epsilonType            epsilon
  ) {
    // 5x5 Gaussain filter
    const ac_fixed<NFRAC_BITS, 0, false> B[GK_SZ][GK_SZ] = {
      {0.00296902, 0.01330621, 0.02193823, 0.01330621, 0.00296902},
      {0.01330621, 0.05963430, 0.09832033, 0.05963430, 0.01330621},
      {0.02193823, 0.09832033, 0.16210282, 0.09832033, 0.02193823},
      {0.01330621, 0.05963430, 0.09832033, 0.05963430, 0.01330621},
      {0.00296902, 0.01330621, 0.02193823, 0.01330621, 0.00296902}
    };
    typedef ac_fixed<(2*CDEPTH) + 6, (2*CDEPTH) + 6, true> InterType;
    ac_fixed<48, 3, true> recden;
    // Declare window object,  to store pixel values for gaussian filtering.
    // The window object uses zero padding by default.
    ac_window_2d_flag<IntensitySqType, GK_SZ, GK_SZ, W_MAX, INTERNAL_WMODE>   acWindxxObj(0);
    ac_window_2d_flag<IntensitySqType, GK_SZ, GK_SZ, W_MAX, INTERNAL_WMODE>   acWindyyObj(0);
    ac_window_2d_flag<IntensitySqType, GK_SZ, GK_SZ, W_MAX, INTERNAL_WMODE>   acWindxyObj(0);

    // The below windowing is similar to the one above. Please refer to the intensity function.
    #pragma hls_pipeline_init_interval 1
    HAR_ROW_LOOP: for (unsigned i = 0; i < H_MAX + (GK_SZ/2); i++) {
      #pragma hls_pipeline_init_interval 1
      HAR_COL_LOOP: for (unsigned j = 0; j < W_MAX + (GK_SZ/2); j++) {
        // The extra GK_SZ/2 iterations for HAR_COL_LOOP only get used up at the very end, when flushing
        // Gaussian outputs that are a result of zero padding to the bottom right of the image.
        // The flag below ensures that the extra iterations are used up at the right time.
        bool prod_output = (i != heightIn + (GK_SZ/2) - 1 && j < widthIn) || (i == heightIn + (GK_SZ/2) - 1);
        if (i < heightIn && j < widthIn) { // Used to prevent excessive input channel reads.
          IntensityType   Ix  = intensityx.read();
          IntensityType   Iy  = intensityy.read();
          IntensitySqType Ixy = Ix*Iy;
          IntensitySqType Ixx = Ix*Ix;
          IntensitySqType Iyy = Iy*Iy;
          // Calculate sof, eof, sol and eol based on image coordinates.
          bool sol = (j == 0);
          bool sof = (i == 0) && sol;
          bool eol = (j == widthIn - 1);
          bool eof = (i == heightIn - 1) && eol;
          // Write input frame value to the window object.
          acWindxxObj.write(Ixx, sof, eof, sol, eol);
          acWindyyObj.write(Iyy, sof, eof, sol, eol);
          acWindxyObj.write(Ixy, sof, eof, sol, eol);
        } else if (prod_output) {
          // sof and eof values will both be false, because we're no longer
          // receiving input pixel values.
          bool sol = (j == 0);
          bool eol = (j == widthIn - 1);
          // The .write function in this case only serves to advance the pointers of the window object.
          // The pixel value being passed to it (0) is a dummy value.
          acWindxxObj.write(0, false, false, sol, eol);
          acWindyyObj.write(0, false, false, sol, eol);
          acWindxyObj.write(0, false, false, sol, eol);
        }
        if (acWindxxObj.valid() && acWindyyObj.valid() && acWindxyObj.valid() && prod_output) {
          // If the window has ramped up, call windFilt() to produce a filtered output.
          gaussOpType   gaussOpxx  = windFilt<gaussOpType> (B, acWindxxObj);
          gaussOpType   gaussOpyy  = windFilt<gaussOpType> (B, acWindyyObj);
          gaussOpType   gaussOpxy  = windFilt<gaussOpType> (B, acWindxyObj);

          // Calculating Harris Response which is given by 2*(det(A))/(trace(A)+epsilon)
          HarrisResType num = 2*((gaussOpxx*gaussOpyy) - (gaussOpxy*gaussOpxy));
          InterType     den = gaussOpxx + gaussOpyy + epsilon;
          ac_math::ac_reciprocal_pwl(den, recden);
          HarrisResType response = num * recden;
          harrisres.write(response);
        }
        if (j == widthIn + (GK_SZ/2) - 1) { break; }
      }
      if (i == heightIn + (GK_SZ/2) - 1) { break; }
    }
  }

  #pragma hls_pipeline_init_interval 1
  #pragma hls_design
  void localmaxima(
    ac_channel<HarrisResType>    &harrisres,
    ac_channel<HarrisResType>    &harrisrescopy,
    ac_channel<HarrisResType>    &maxima,
    const widthInType            widthIn,
    const heightInType           heightIn
  ) {
    ac_window_2d_flag<HarrisResType, EK_SZ, EK_SZ, W_MAX, INTERNAL_WMODE> acWindObj(0.0);
    // The below windowing is similar to the one above. Please refer to the intensity function.
    #pragma hls_pipeline_init_interval 1
    LOC_ROW_LOOP: for (unsigned i = 0; i < H_MAX + (EK_SZ/2); i++) {
      #pragma hls_pipeline_init_interval 1
      LOC_COL_LOOP: for (unsigned j = 0; j < W_MAX + (EK_SZ/2); j++) {
        // The extra EK_SZ/2 iterations for LOC_COL_LOOP only get used up at the very end, when flushing
        // outputs that are a result of zero padding to the bottom right of the image.
        // The flag below ensures that the extra iterations are used up at the right time.
        bool prod_output = (i != heightIn + (EK_SZ/2) - 1 && j < widthIn) || (i == heightIn + (EK_SZ/2) - 1);
        if (i < heightIn && j < widthIn) { // Used to prevent excessive input channel reads.
          HarrisResType response  = harrisres.read();

          // Calculate sof, eof, sol and eol based on image coordinates.
          bool sol = (j == 0);
          bool sof = (i == 0) && sol;
          bool eol = (j == widthIn - 1);
          bool eof = (i == heightIn - 1) && eol;
          // Write input frame value to the window object.
          acWindObj.write(response, sof, eof, sol, eol);
        } else if (prod_output) {
          // sof and eof values will both be false, because we're no longer
          // receiving input pixel values.
          bool sol = (j == 0);
          bool eol = (j == widthIn - 1);
          // The .write function in this case only serves to advance the pointers of the window object.
          // The pixel value being passed to it (0) is a dummy value.
          acWindObj.write(0.0, false, false, sol, eol);
        }
        if (acWindObj.valid() && prod_output) {
          // If the window has ramped up, call the maximum function to get the local maxima in the EK_SZxEK_SZ neighbourhood.
          HarrisResType maxval = maximum<HarrisResType> (acWindObj);
          maxima.write(maxval);
          harrisrescopy.write(acWindObj(1 - (EK_SZ/2), 1 - (EK_SZ/2)));
        }
        if (j == widthIn + (EK_SZ/2) - 1) { break; }
      }
      if (i == heightIn + (EK_SZ/2) - 1) { break; }
    }
  }

  #pragma hls_pipeline_init_interval 1
  #pragma hls_design
  void thresholding(
    ac_channel<HarrisResType>    &harrisrescopy,
    ac_channel<HarrisResType>    &maxima,
    ac_channel<OUT_TYPE>         &streamOut,
    const widthInType            widthIn,
    const heightInType           heightIn,
    const thresholdType          threshold
  ) {
    #pragma hls_pipeline_init_interval 1
    THR_ROW_LOOP: for (unsigned i = 0; i < H_MAX; i++) {
      #pragma hls_pipeline_init_interval 1
      THR_COL_LOOP: for (unsigned j = 0; j < W_MAX; j++) {

        HarrisResType response = harrisrescopy.read();
        HarrisResType maxval   = maxima.read();
        OUT_TYPE output;

        // If the response is equal to the local maxima and greater than threshold, corner is detected. If corner is detected, then call color
        // function to color the corner points
        OUT_TYPE outval = ((response.to_int()) == (maxval.to_int()) && response > threshold)?(color<OUT_TYPE>(output,i,j,widthIn)):(OUT_TYPE)0;
        streamOut.write(outval);

        if (j == widthIn - 1) { break; }
      }
      if (i == heightIn - 1) { break; }
    }
  }

  // Carry out filtering with the window values.
  template<class filtOpType, class acWindType, class kType, int K_SZ>
  filtOpType windFilt(
    const kType (&kernel)[K_SZ][K_SZ],
    const ac_window_2d_flag<acWindType, K_SZ, K_SZ, W_MAX, INTERNAL_WMODE> &acWindObj
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

  // Find the local maxima value.
  template<class maxType, class acWindType, int K_SZ>
  maxType maximum(
    const ac_window_2d_flag<acWindType, K_SZ, K_SZ, W_MAX, INTERNAL_WMODE> &acWindObj
  ) {
    acWindType acWindOut[K_SZ*K_SZ];
    #pragma hls_unroll yes
    CONV_OP_ROW_LOOP: for (int r = 0; r < int(K_SZ); r++) {
      #pragma hls_unroll yes
      CONV_OP_COL_LOOP: for (int c = 0; c < int(K_SZ); c++) {
        acWindOut[r*K_SZ + c] = acWindObj(r - (K_SZ/2), c - (K_SZ/2));
      }
    }

    maxType x = max<K_SZ *K_SZ>(acWindOut);
    return x;
  }


  // Extract component from RGB pixel
  void extractcomp(const ac_ipl::RGB_imd<ac_int<CDEPTH, false> > pixIn, ac_int<CDEPTH, false> &comp, const componentType component) {
    comp = (component == 0)?(pixIn.R):(component == 1)?(pixIn.G):pixIn.B;
  }

  // For grayscale pixel, assign the pixel itself to the component
  void extractcomp(const ac_int<CDEPTH, false> pixIn, ac_int<CDEPTH, false> &comp, const componentType component) {
    comp = pixIn;
  }

  // For RGB pixel, corner points are colored white
  template<class OutType>
  OutType color(const ac_ipl::RGB_imd<ac_int<CDEPTH, false> > pixOut, unsigned i, unsigned j, const widthInType widthIn) {
    OutType outval;
    outval.R = 255;
    outval.G = 255;
    outval.B = 255;
    return outval;
  }

  // For grayscale pixel, corner points are colored white
  template<class OutType>
  OutType color(const ac_int<CDEPTH, false> pixOut, unsigned i, unsigned j, const widthInType widthIn) {
    OutType outval;
    outval = 255;
    return outval;
  }

  ac_channel<IntensityType>   P1; // Interconnect channel with intensity output in x direction
  ac_channel<IntensityType>   P2; // Interconnect channel with intensity output in y direction
  ac_channel<HarrisResType>   P3; // Interconnect channel with harris response
  ac_channel<HarrisResType>   P4; // Interconnect channel with copy of harris response
  ac_channel<HarrisResType>   P5; // Interconnect channel with local maxima values
};

#endif // #ifndef _INCLUDED_AC_HARRIS_H_

