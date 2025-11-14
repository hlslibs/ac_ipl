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
#ifndef _INCLUDED_AC_LOCALCONTRASTNORM_H_
#define _INCLUDED_AC_LOCALCONTRASTNORM_H_

#include <ac_int.h>
#include <ac_fixed.h>
#include <ac_ipl/ac_pixels.h>
#include <ac_window_2d_flag.h>
#include <ac_math/ac_reciprocal_pwl.h>
#include <ac_math/ac_sqrt_pwl.h>
#include <ac_channel.h>
#include <mc_scverify.h>

#pragma hls_design top
template <unsigned CDEPTH, unsigned W_MAX, unsigned H_MAX, bool USE_SINGLEPORT = false>
class ac_localcontrastnorm
{
public:
  // NFRAC_BITS: Fractional bits used to store the results of fixed point intermediate calculations/coefficients.
  // INTERNAL_WMODE: singleport and dualport implementations are supported.
  // GK_SZ: window size for the Gaussian filter
  enum {
    NFRAC_BITS = 16,
    GK_SZ = 9,
    INTERNAL_WMODE = USE_SINGLEPORT ? AC_BOUNDARY | AC_SINGLEPORT : AC_BOUNDARY
  };
  // Dimension types are bitwidth-constrained according to the max dimensions possible.
  typedef ac_int<ac::nbits<W_MAX>::val, false> widthInType;
  typedef ac_int<ac::nbits<H_MAX>::val, false> heightInType;
  // Define I/O types.
  typedef ac_ipl::RGB_1PPC<CDEPTH> pixInType;
  typedef ac_ipl::RGB_1PPC<CDEPTH> pixOutType;

  typedef ac_ipl::RGB_imd<ac_fixed<NFRAC_BITS + CDEPTH, CDEPTH, true> > pixgaussOpType; // Type for Gaussian filter output.
  typedef ac_ipl::RGB_imd<ac_fixed<(((3*NFRAC_BITS) + (2*CDEPTH))>>1), CDEPTH, false> > pixstddevType; // Type for standard deviation

public:
  ac_localcontrastnorm() {}

#pragma hls_pipeline_init_interval 1
#pragma hls_design interface
  void CCS_BLOCK(run) (
    ac_channel<pixInType>  &streamIn,   // Pixel input stream
    ac_channel<pixOutType> &streamOut,  // Pixel output stream
    const widthInType      &widthIn,    // Input width stream
    const heightInType     &heightIn    // Input height stream
  ) {
#ifndef __SYNTHESIS__
    AC_ASSERT(streamIn.debug_size() == widthIn*heightIn, "Please write all image input pixels to channel before calling the run function.");
#endif
    getcenteredimage(streamIn, P1, widthIn, heightIn);
    getstddeviation(P1, streamOut, widthIn, heightIn); // P1: Centered image = Input - Gaissian Output. P3: Standard Deviation
  }

private:
#pragma hls_pipeline_init_interval 1
#pragma hls_design
  void getcenteredimage(
    ac_channel<pixInType>      &streamIn,
    ac_channel<pixgaussOpType> &centeredimage, // Gaussian filter output.
    const widthInType          widthIn,
    const heightInType         heightIn
  ) {
    typedef ac_int<CDEPTH, false> InType;
    pixgaussOpType gaussOp;
    pixgaussOpType centeredimageval;

    // Declare window object to store input pixel values for gaussian filtering.
    // The window object uses zero padding by default.
    ac_window_2d_flag<InType, GK_SZ, GK_SZ, W_MAX, INTERNAL_WMODE> acWindRObj(0);
    ac_window_2d_flag<InType, GK_SZ, GK_SZ, W_MAX, INTERNAL_WMODE> acWindGObj(0);
    ac_window_2d_flag<InType, GK_SZ, GK_SZ, W_MAX, INTERNAL_WMODE> acWindBObj(0);

    // Because the window needs (GK_SZ/2) input rows to ramp up and start producing outputs,
    // extra cycles are provided for after all the inputs are read, to flush out
    // all valid filter outputs. This is done by adding an extra (GK_SZ/2) iterations for
    // GAUSS_ROW_LOOP and GAUSS_COL_LOOP.
#pragma hls_pipeline_init_interval 1
    IN_ROW_LOOP: for (unsigned i = 0; i < H_MAX + (GK_SZ/2); i++) {
#pragma hls_pipeline_init_interval 1
      IN_COL_LOOP: for (unsigned j = 0; j < W_MAX + (GK_SZ/2); j++) {
        // The extra GK_SZ/2 iterations for IN_COL_LOOP only get used up at the very end, when flushing
        // Gaussian outputs that are a result of zero padding to the bottom right of the image.
        // The flag below ensures that the extra iterations are used up at the right time.
        bool prod_output = (i != heightIn + (GK_SZ/2) - 1 && j < widthIn) || (i == heightIn + (GK_SZ/2) - 1);
        if (i < heightIn && j < widthIn) { // Used to prevent excessive input channel reads.
          pixInType pixIn = streamIn.read();
          // Calculate sof, eof, sol and eol based on image coordinates.
          bool sol = (j == 0);
          bool sof = (i == 0) && sol;
          bool eol = (j == widthIn - 1);
          bool eof = (i == heightIn - 1) && eol;
          // Write input frame value to the window object.
          acWindRObj.write(pixIn.R, sof, eof, sol, eol);
          acWindGObj.write(pixIn.G, sof, eof, sol, eol);
          acWindBObj.write(pixIn.B, sof, eof, sol, eol);
        } else if (prod_output) {
          // sof and eof values will both be false, because we're no longer
          // receiving input pixel values.
          bool sol = (j == 0);
          bool eol = (j == widthIn - 1);
          // The .write function in thie case only serves to advance the pointers of the window object.
          // The pixel value being passed to it (0) is a dummy value.
          acWindRObj.write(0, false, false, sol, eol);
          acWindGObj.write(0, false, false, sol, eol);
          acWindBObj.write(0, false, false, sol, eol);
        }
        if (acWindRObj.valid() && acWindGObj.valid() && acWindBObj.valid() && prod_output) {
          // If the window has ramped up, call gaussFilt() to produce a filtered output.
          gaussOp   = gaussFilt<pixgaussOpType> (acWindRObj, acWindGObj, acWindBObj);

          // Get the centered image which is basically the image input - gaussian filtered output of input image
          centeredimageval.R = acWindRObj(4 - (GK_SZ/2), 4 - (GK_SZ/2)) - gaussOp.R;
          centeredimageval.G = acWindGObj(4 - (GK_SZ/2), 4 - (GK_SZ/2)) - gaussOp.G;
          centeredimageval.B = acWindBObj(4 - (GK_SZ/2), 4 - (GK_SZ/2)) - gaussOp.B;

          centeredimage.write(centeredimageval);
        }
        if (j == widthIn + (GK_SZ/2) - 1) { break; }
      }
      if (i == heightIn + (GK_SZ/2) - 1) { break; }
    }

#ifndef __SYNTHESIS__
    AC_ASSERT(streamIn.debug_size() == 0, "Input to gaussian filter block not completely consumed.");
    AC_ASSERT(centeredimage.debug_size() == widthIn*heightIn, "Incorrect output size for gaussian filter block.");
#endif
  }

#pragma hls_pipeline_init_interval 1
#pragma hls_design
  void getstddeviation(
    ac_channel<pixgaussOpType> &centeredimage,
    ac_channel<pixOutType>     &streamOut,
    const widthInType          widthIn,
    const heightInType         heightIn
  ) {
    pixstddevType stddev;

    typedef ac_fixed<NFRAC_BITS + CDEPTH, CDEPTH, true> gaussOpType;
    typedef ac_ipl::RGB_imd<ac_fixed<2*(NFRAC_BITS + CDEPTH), 2*CDEPTH, false> > pixsqimgType;
    typedef ac_fixed<2*(NFRAC_BITS + CDEPTH), 2*CDEPTH, false> sqimgType;
    typedef ac_ipl::RGB_imd<ac_fixed<(3*NFRAC_BITS) + (2*CDEPTH), 2*CDEPTH, false> > pixvarType; // Type for variance
    typedef ac_fixed<(3*NFRAC_BITS) + (2*CDEPTH), 2*CDEPTH, false> varType;
    typedef ac_fixed<(((3*NFRAC_BITS) + (2*CDEPTH))>>1), CDEPTH, false> stddevType;
    pixvarType variance;
    pixgaussOpType copycenterimgval;
	
		ac_ipl::RGB_imd<ac_fixed<19, 3, false> > recstddev;
    //Fixed point datatype of the output
    ac_ipl::RGB_imd<ac_fixed<NFRAC_BITS + CDEPTH + 19, CDEPTH + 3, false> > intout;

    pixOutType lcnint;

    // Declare window object to store input pixel values for gaussian filtering.
    // The window object uses zero padding by default.
    ac_window_2d_flag<sqimgType, GK_SZ, GK_SZ, W_MAX, INTERNAL_WMODE> acWindRObj(0);
    ac_window_2d_flag<sqimgType, GK_SZ, GK_SZ, W_MAX, INTERNAL_WMODE> acWindGObj(0);
    ac_window_2d_flag<sqimgType, GK_SZ, GK_SZ, W_MAX, INTERNAL_WMODE> acWindBObj(0);

    ac_window_2d_flag<gaussOpType, GK_SZ, GK_SZ, W_MAX, INTERNAL_WMODE> cenWindRObj(0);
    ac_window_2d_flag<gaussOpType, GK_SZ, GK_SZ, W_MAX, INTERNAL_WMODE> cenWindGObj(0);
    ac_window_2d_flag<gaussOpType, GK_SZ, GK_SZ, W_MAX, INTERNAL_WMODE> cenWindBObj(0);

    // The below windowing is similar to the one above. Please refer to the above function.
#pragma hls_pipeline_init_interval 1
    CEN_ROW_LOOP: for (unsigned i = 0; i < H_MAX + (GK_SZ/2); i++) {
#pragma hls_pipeline_init_interval 1
      CEN_COL_LOOP: for (unsigned j = 0; j < W_MAX + (GK_SZ/2); j++) {
        bool prod_output = (i != heightIn + (GK_SZ/2) - 1 && j < widthIn) || (i == heightIn + (GK_SZ/2) - 1);
        if (i < heightIn && j < widthIn) { // Used to prevent excessive input channel reads.
          pixgaussOpType centerimgval = centeredimage.read();
          pixsqimgType sqimgval = centerimgval * centerimgval;
          bool sol = (j == 0);
          bool sof = (i == 0) && sol;
          bool eol = (j == widthIn - 1);
          bool eof = (i == heightIn - 1) && eol;
          // Write input frame value to the window object.
          acWindRObj.write(sqimgval.R, sof, eof, sol, eol);
          acWindGObj.write(sqimgval.G, sof, eof, sol, eol);
          acWindBObj.write(sqimgval.B, sof, eof, sol, eol);

          cenWindRObj.write(centerimgval.R, sof, eof, sol, eol);
          cenWindGObj.write(centerimgval.G, sof, eof, sol, eol);
          cenWindBObj.write(centerimgval.B, sof, eof, sol, eol);

        } else if (prod_output) {
          bool sol = (j == 0);
          bool eol = (j == widthIn - 1);
          acWindRObj.write(0, false, false, sol, eol);
          acWindGObj.write(0, false, false, sol, eol);
          acWindBObj.write(0, false, false, sol, eol);

          cenWindRObj.write(0, false, false, sol, eol);
          cenWindGObj.write(0, false, false, sol, eol);
          cenWindBObj.write(0, false, false, sol, eol);
        }
        if (acWindRObj.valid() && acWindGObj.valid() && acWindBObj.valid() && cenWindRObj.valid() && cenWindGObj.valid() && cenWindBObj.valid() && prod_output ) {
          // variance is convolution of the square of centered image and the gaussian filter
          variance   = gaussFilt<pixvarType> (acWindRObj, acWindGObj, acWindBObj);

          // The standard deviation of the image is the square root of the variance
          stddev.R = ac_math::ac_sqrt_pwl<stddevType>(variance.R);
          stddev.G = ac_math::ac_sqrt_pwl<stddevType>(variance.G);
          stddev.B = ac_math::ac_sqrt_pwl<stddevType>(variance.B);

          copycenterimgval.R = cenWindRObj(4 - (GK_SZ/2), 4 - (GK_SZ/2));
          copycenterimgval.G = cenWindGObj(4 - (GK_SZ/2), 4 - (GK_SZ/2));
          copycenterimgval.B = cenWindBObj(4 - (GK_SZ/2), 4 - (GK_SZ/2));
		  
					stddev.R = (stddev.R>1e-4)?stddev.R:1e-4;
					stddev.G = (stddev.G>1e-4)?stddev.G:1e-4;
					stddev.B = (stddev.B>1e-4)?stddev.B:1e-4;

					ac_math::ac_reciprocal_pwl(stddev.R, recstddev.R);
					ac_math::ac_reciprocal_pwl(stddev.G, recstddev.G);
					ac_math::ac_reciprocal_pwl(stddev.B, recstddev.B);

					// The output is the centered image divided by the standard deviation
					intout = copycenterimgval * recstddev;

					// Converting form fixed point output to integer output
					lcnint.R = intout.R.to_int();
					lcnint.G = intout.G.to_int();
					lcnint.B = intout.B.to_int();
					// Initialize TUSER and TLAST for output pixel stream.
					lcnint.TUSER = (i == 0 && j == 0);
					lcnint.TLAST = (j == widthIn - 1);
					streamOut.write(lcnint);

        }
        if (j == widthIn + (GK_SZ/2) - 1) { break; }
      }
      if (i == heightIn + (GK_SZ/2) - 1) { break; }
    }

#ifndef __SYNTHESIS__
    AC_ASSERT(centeredimage.debug_size() == 0, "Input to gaussian filter block not completely consumed.");
#endif
  }

  // Carry out filtering with kernel and window values.
  template<class filtOpType, class acWindType, int K_SZ>
  filtOpType gaussFilt(
    const ac_window_2d_flag<acWindType, K_SZ, K_SZ, W_MAX, INTERNAL_WMODE> &acWindRObj,
    const ac_window_2d_flag<acWindType, K_SZ, K_SZ, W_MAX, INTERNAL_WMODE> &acWindGObj,
    const ac_window_2d_flag<acWindType, K_SZ, K_SZ, W_MAX, INTERNAL_WMODE> &acWindBObj
  ) {
    // Gaussian kernel
    const ac_fixed<NFRAC_BITS, 0, false> G_kernel[K_SZ][K_SZ] = {
      {0.000763447 , 0.00183141  , 0.00342153  , 0.0049783   ,  0.00564116 , 0.0049783   , 0.00342153  , 0.00183141  , 0.000763447 },
      {0.00183141  , 0.00439334  , 0.00820783  , 0.0119423   ,  0.0135324  , 0.0119423   , 0.00820783  , 0.00439334  , 0.00183141  },
      {0.00342153  , 0.00820783  , 0.0153342   , 0.0223112   ,  0.0252819  , 0.0223112   , 0.0153342   , 0.00820783  , 0.00342153  },
      {0.0049783   , 0.0119423   , 0.0223112   , 0.0324626   ,  0.036785   , 0.0324626   , 0.0223112   , 0.0119423   , 0.0049783   },
      {0.00564116  , 0.0135324   , 0.0252819   , 0.036785    ,  0.0416828  , 0.036785    , 0.0252819   , 0.0135324   , 0.00564116  },
      {0.0049783   , 0.0119423   , 0.0223112   , 0.0324626   ,  0.036785   , 0.0324626   , 0.0223112   , 0.0119423   , 0.0049783   },
      {0.00342153  , 0.00820783  , 0.0153342   , 0.0223112   ,  0.0252819  , 0.0223112   , 0.0153342   , 0.00820783  , 0.00342153  },
      {0.00183141  , 0.00439334  , 0.00820783  , 0.0119423   ,  0.0135324  , 0.0119423   , 0.00820783  , 0.00439334  , 0.00183141  },
      {0.000763447 , 0.00183141  , 0.00342153  , 0.0049783   ,  0.00564116 , 0.0049783   , 0.00342153  , 0.00183141  , 0.000763447 }
    };

    filtOpType filtOp = 0;
    // Window output is stored in temporary array. While you can also directly multiply values from
    // acWindObj and get the same QofR, having a temporary array can be more convenient for debugging.
    acWindType acWindROut[K_SZ][K_SZ];
    acWindType acWindGOut[K_SZ][K_SZ];
    acWindType acWindBOut[K_SZ][K_SZ];
#pragma hls_unroll yes
    CONV_OP_ROW_LOOP: for (int r = 0; r < int(K_SZ); r++) {
#pragma hls_unroll yes
      CONV_OP_COL_LOOP: for (int c = 0; c < int(K_SZ); c++) {
        acWindROut[r][c] = acWindRObj(r - (K_SZ/2), c - (K_SZ/2));
        acWindGOut[r][c] = acWindGObj(r - (K_SZ/2), c - (K_SZ/2));
        acWindBOut[r][c] = acWindBObj(r - (K_SZ/2), c - (K_SZ/2));
        //cout << " The actual window values are: " << acWindOut[r][c] << endl;
        filtOp.R += acWindROut[r][c]*(G_kernel[r][c]);
        filtOp.G += acWindGOut[r][c]*(G_kernel[r][c]);
        filtOp.B += acWindBOut[r][c]*(G_kernel[r][c]);
      }
    }
    return filtOp;
  }

  ac_channel<pixgaussOpType> P1; // Interconnect channel with centered image.
};

#endif
