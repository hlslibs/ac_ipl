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
//******************************************************************************************
// Description:
// Usage:
// Notes:
// Revision History:
//    3.4.3  - dgb - Updated compiler checks to work with MS VS 2019
//******************************************************************************************

#ifndef _INCLUDED_AC_IMHIST_H_
#define _INCLUDED_AC_IMHIST_H_

#include <ac_int.h>
#include <ac_fixed.h>
#include <ac_ipl/ac_pixels.h>
#include <ac_channel.h>
#include <mc_scverify.h>

// The design uses static_asserts, which are only supported by C++11 or later compiler standards.
// The #error directive below informs the user if they're not using those standards.
#if (defined(__GNUC__) && (__cplusplus < 201103L))
#error Please use C++11 or a later standard for compilation.
#endif
#if (defined(_MSC_VER) && (_MSC_VER < 1920) && !defined(__EDG__))
#error Please use Microsoft VS 2019 or a later standard for compilation.
#endif

// The design can only accept ac_int (greyscale) or RGB_1PPC (color) inputs. imhistTypeInfo
// contains a "match" enum that helps to trigger a static_assert if the user doesn't use the
// correct types. imhistTypeInfo also has certain type-specific information.

// Generic imhistTypeInfo struct. Is later specialized to deal with different types.
template <unsigned W_MAX, unsigned H_MAX, class T>
struct imhistTypeInfo {
  enum {
    match = false
  };
};

// imhistTypeInfo, specialized for greyscale inputs/outputs.
template <unsigned W_MAX, unsigned H_MAX, int W>
struct imhistTypeInfo<W_MAX, H_MAX, ac_int<W, false> > {
  enum {
    match = true,
    IN_CDEPTH = W, // Input color-depth
  };
  // Max. count possible = Max number of input pixels possible = W_MAX*H_MAX
  typedef ac_int<ac::nbits<W_MAX*H_MAX>::val, false> histArrType;
};

// imhistTypeInfo, specialized for color inputs.
template <unsigned W_MAX, unsigned H_MAX, unsigned W>
struct imhistTypeInfo<W_MAX, H_MAX, ac_ipl::RGB_1PPC<W> > {
  enum {
    match = true,
    IN_CDEPTH = W, // Input color-depth
  };
  // Max. count possible = Max number of input pixels possible = W_MAX*H_MAX
  typedef ac_ipl::RGB_imd<ac_int<ac::nbits<W_MAX*H_MAX>::val, false> > histArrType;
};

// Template parameters:
// IN_TYPE: Input Pixel Type
// OUT_BW: Bitwidth of output
// W_MAX: Max. Supported Width
// H_MAX: Max. Supported Height
template <class IN_TYPE, int OUT_BW, unsigned W_MAX, unsigned H_MAX>
class ac_imhist
{
public:
  typedef imhistTypeInfo<W_MAX, H_MAX, IN_TYPE> stDef;
  // Make sure that the user uses the correct input/output types.
  static_assert(stDef::match, "Input must either be unsigned ac_int or RGB_1PPC.");

  ac_imhist() {
    countVal = 0;
    prevPixIn = 0;
  }

  // Define output and dimension types.
  typedef ac_int<OUT_BW, false> OUT_TYPE;
  // Dimension types are bitwidth-constrained according to the max dimensions possible.
  typedef ac_int<ac::nbits<W_MAX>::val, false> widthInType;
  typedef ac_int<ac::nbits<H_MAX>::val, false> heightInType;

  #pragma hls_pipeline_init_interval 1
  #pragma hls_design interface
  void CCS_BLOCK(run) (
    ac_channel<IN_TYPE>  &streamIn,  // Pixel input stream
    ac_channel<OUT_TYPE> &streamOut, // Pixel output stream
    const widthInType    widthIn,    // Input width
    const heightInType   heightIn    // Input height
  ) {
    // Initialize all histogram counts to zero.
    #pragma hls_pipeline_init_interval 1
    INIT_ARR: for (unsigned i = 0; i < OUT_SIZE; i++) {
      histArr[i] = 0;
    }

    // Read all input pixel and update the histogram count array accordingly.
    #pragma hls_pipeline_init_interval 1
    READ_INPUT: for (unsigned i = 0; i < W_MAX*H_MAX + 1; i++) {
      if (i != widthIn*heightIn) {
        IN_TYPE pixIn = streamIn.read();
        updateArr(pixIn);
      } else {
        IN_TYPE pixIn = 0; // Dummy value, not actually used.
        finalUpdateArr(pixIn);
        break;
      }
    }

    // Read histogram count array and write the counts to the output channel.
    #pragma hls_pipeline_init_interval 1
    WRITE_OUTPUT: for (unsigned i = 0; i < OUT_SIZE; i++) {
      // IN_TYPE is a dummy input and will be optimized away. It is used to enable template-based overloading
      // of the writeOut function.
      IN_TYPE dummyIn;
      OUT_TYPE histOut = writeOut(dummyIn, i);
      streamOut.write(histOut);
    }
  }

private:
  // Find input color depth.
  enum { IN_CDEPTH  = stDef::IN_CDEPTH };

  // This function updates the histogram count array for greyscale inputs.
  void updateArr(const ac_int<IN_CDEPTH, false> pixIn) {
    if (prevPixIn != pixIn) {
      // If current pixel input is different than the previous input, write the histogram count associated with the
      // previous input to RAM, and then read the RAM to extract the histogram  value associated with the current
      // input. Doing the simultaneous read-write only when the previous pixel input differs from the current one
      // eliminates possible feedback violations and allows us to safely ignore any memory precedences reported by Catapult.
      histArr[prevPixIn] = countVal;
      countVal = histArr[pixIn];
      countVal++;
      prevPixIn = pixIn;
    } else {
      countVal++; // Only update the countVal variable and don't access the RAM, so as to avoid triggering feedback violations.
    }
  }

  // This function updates the histogram count array for color inputs.
  void updateArr(const ac_ipl::RGB_1PPC<IN_CDEPTH> pixIn) {
    // Update histogram values separately for each color component, based on color values.
    // The histogram values are updated in a manner similar to the ac_int version above, but now spread across all color components.
    if (prevPixIn.R != pixIn.R) {
      histArr[prevPixIn.R].R = countVal.R;
      countVal.R = histArr[pixIn.R].R;
      countVal.R++;
      prevPixIn.R = pixIn.R;
    } else {
      countVal.R++;
    }
    
    if (prevPixIn.G != pixIn.G) {
      histArr[prevPixIn.G].G = countVal.G;
      countVal.G = histArr[pixIn.G].G;
      countVal.G++;
      prevPixIn.G = pixIn.G;
    } else {
      countVal.G++;
    }
    
    if (prevPixIn.B != pixIn.B) {
      histArr[prevPixIn.B].B = countVal.B;
      countVal.B = histArr[pixIn.B].B;
      countVal.B++;
      prevPixIn.B = pixIn.B;
    } else {
      countVal.B++;
    }
  }

  // This function updates the histogram count array for greyscale inputs, for the final READ_INPUT loop iteration.
  // pixIn is a dummy value and not actually used. It's only there to facilitate function overloading.
  void finalUpdateArr(const ac_int<IN_CDEPTH, false> pixIn) {
    histArr[prevPixIn] = countVal;
  }
  
  // This function updates the histogram count array for color inputs, for the final READ_INPUT loop iteration.
  // pixIn is a dummy value and not actually used. It's only there to facilitate function overloading.
  void finalUpdateArr(const ac_ipl::RGB_1PPC<IN_CDEPTH> pixIn) {
    histArr[prevPixIn.R].R = countVal.R;
    histArr[prevPixIn.G].G = countVal.G;
    histArr[prevPixIn.B].B = countVal.B;
  }
  
  // This function writes the histogram output to the temporary output variable (histOut), for greyscale inputs.
  OUT_TYPE writeOut(const ac_int<IN_CDEPTH, false> dummyIn, const unsigned i) {
    OUT_TYPE histOut;
    histOut = histArr[i];
    return histOut;
  }

  // This function does the same, but for color inputs.
  OUT_TYPE writeOut(const ac_ipl::RGB_1PPC<IN_CDEPTH> dummyIn, const unsigned i) {
    OUT_TYPE histOut;
    histOut = histArr[i].R + histArr[i].G + histArr[i].B; // Accumulate histogram counts across color components.
    return histOut;
  }

  // size of output array = number of intensity levels possible = 2^IN_CDEPTH.
  enum { OUT_SIZE = 1 << IN_CDEPTH };

  // If the input image is greyscale, then the histogram count array will be an array of ac_int values.
  // If the input image is color, then the histogram count array will be an array of RGB_imd values.
  // The stDef templatized struct allows us to choose between the two array types based on the input type.
  typedef typename stDef::histArrType histArrType;
  histArrType histArr[OUT_SIZE];
  histArrType countVal;
  IN_TYPE prevPixIn;

};

#endif
