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
//*********************************************************************************************************
// File: ac_ctc.h
//
// Description:
//  Code for the hardware implementation of a color-temperature correction algorithm. Color temperature
//  adjustments are made for a given set of input image pixels by obtaining the ratio of the frame and
//  reference white point values, and then multiplying each input pixel by the ratio obtained.
//
// Nomenclature:
//          -------- Color
//          |------- Temperature
//          ||------ Correction
//          |||
//       ac_ctc
//
// Usage:
//  A sample testbench and its implementation look like this:
//  #include <ac_ipl/ac_ctc.h>
//
//  #include <mc_scverify.h>
//
//  CCS_MAIN(int arg, char **argc)
//  {
//    // Initialize ac_ctc class to accept 24 bpp input images with a max dimensions 10x10 and a max.
//    // target color temperature of 10000 K 
//    ac_fft_dit_r2_inpl <8, 10, 10, 10000> ctcInst;
//
//    typedef ac_ipl::RGB_1PPC<8> IO_TYPE;
//
//    // Declare channels for input and output
//    ac_channel<IO_TYPE> streamIn, streamOut;
//
//    // Considering a sample 2x2 input image, write four inputs to input channel.
//    streamIn.write(0);
//    streamIn.write(1);
//    streamIn.write(2);
//    streamIn.write(3);
//
//    // Initialize input variables for dynamic dimension values and target color temperature of 5000 K.
//    unsigned widthIn = 2, heightIn = 2, tempIn = 5000;
//
//    // Call the top-level function.
//    ctcInst.run(streamIn, streamOut, widthIn, heightIn, tempIn);
//
//    CCS_RETURN(0);
//  }
//
// Notes:
//  Attempting to call the function with a type that is not implemented will result
//  in a compile error.
//  Currently, the block only accepts RGB_1PPC inputs.
//
// Revision History:
//    3.4.3  - dgb - Updated compiler checks to work with MS VS 2019
//    3.3.0  - Added CDesignChecker waivers/fixes.
//             CNS violations were waived away and FXD violations were fixed by using floating point
//            literals instead of integer literals.
//
//*********************************************************************************************************

#ifndef _INCLUDED_AC_CTC_H_
#define _INCLUDED_AC_CTC_H_

#include <ac_int.h>
#include <ac_fixed.h>
#include <ac_ipl/ac_pixels.h>
#include <ac_channel.h>
#include <ac_math/ac_div.h>
#include <mc_scverify.h>

#ifndef __SYNTHESIS__
#include <iostream>
#endif

// The design uses static_asserts, which are only supported by C++11 or later compiler standards.
// The #error directive below informs the user if they're not using those standards.
#if (defined(__GNUC__) && (__cplusplus < 201103L))
#error Please use C++11 or a later standard for compilation.
#endif
#if (defined(_MSC_VER) && (_MSC_VER < 1920) && !defined(__EDG__))
#error Please use Microsoft VS 2019 or a later standard for compilation.
#endif

// Mandatory template parameters:
// CDEPTH: Input/Output color depth.
// W_MAX/H_MAX: Max. supported image width/height.
// TEMP_MAX: Max. supported color temperature.
// Optional template parameters:
// R_WP, G_WP, B_WP: User defined white point of video. If all values are 0, the design deduces video white point from
// previously processed input frames.
template <unsigned CDEPTH, unsigned W_MAX, unsigned H_MAX, unsigned TEMP_MAX, unsigned R_WP = 0, unsigned G_WP = 0, unsigned B_WP = 0>
class ac_ctc
{
public:
  // Define IO datatype
  typedef ac_ipl::RGB_1PPC<CDEPTH> IO_TYPE;
  // Dimension and temperature input types are bitwidth-constrained according to the max dimensions/temperature possible.
  typedef ac_int<ac::nbits<W_MAX>::val, false> widthInType;
  typedef ac_int<ac::nbits<H_MAX>::val, false> heightInType;
  typedef ac_int<ac::nbits<TEMP_MAX>::val, false> tempInType;

#pragma hls_pipeline_init_interval 1
#pragma hls_design interface
  void CCS_BLOCK(run) (
    ac_channel<IO_TYPE> &streamIn,  // Pixel input stream
    ac_channel<IO_TYPE> &streamOut, // Pixel output stream
    const widthInType   widthIn,    // Input width
    const heightInType  heightIn,   // Input height
    const tempInType    tempIn      // Input color temperature
  ) {
    AC_ASSERT(tempIn <= TEMP_MAX, "Input color temperature must not exceed maximum supported color temperature.");
    AC_ASSERT(tempIn >= tempLimitLow, "Input color temperature must not fall below minimum supported by design.");

    // Normalize the input color temperature and use the normalized temperature to index into the reference value LUTs.
    const ac_int<ac::nbits<sizeTable - 1>::val, false> index = (tempIn - tempLimitLow)/tempInc;
    refValsType refValR = refValsR[index], refValG = refValsG[index], refValB = refValsB[index];
    // Declare variable to store the ratio between reference and frame white point values.
    ac_ipl::RGB_imd<ac_fixed<n_f_b + 8, 8, false> > ratio;

#pragma hls_waive CNS
    if (!userSuppliedWP) {
      // If the frame white point values for red, green or blue are zero, the corresponding ratio will be set to unity.

      if (frameVal.R == 0) {
        ratio.R = 1.0;
      } else {
        ac_math::ac_div(refValR, frameVal.R, ratio.R);
      }

      if (frameVal.G == 0) {
        ratio.G = 1.0;
      } else {
        ac_math::ac_div(refValG, frameVal.G, ratio.G);
      }

      if (frameVal.B == 0) {
        ratio.B = 1.0;
      } else {
        ac_math::ac_div(refValB, frameVal.B, ratio.B);
      }
    } else {
      ratio.R = ac_fixed<n_f_b + 8, 8, false>(refValsR)/frameVal.R;
      ratio.G = ac_fixed<n_f_b + 8, 8, false>(refValsG)/frameVal.G;
      ratio.B = ac_fixed<n_f_b + 8, 8, false>(refValsB)/frameVal.B;
    }

    ac_int<CDEPTH + 2, false> maxRGBSum = 0, RGBSum;

#pragma hls_pipeline_init_interval 1
    CTC_LOOP: for (unsigned i = 0; i < H_MAX*W_MAX; i++) {
      IO_TYPE pixIn = streamIn.read(), pixOut;
      // Multiply input RGB values with the relevant ratios.
      pixOut.R = (ac_fixed<CDEPTH, CDEPTH, false, AC_RND, AC_SAT>(ratio.R*pixIn.R)).to_int();
      pixOut.G = (ac_fixed<CDEPTH, CDEPTH, false, AC_RND, AC_SAT>(ratio.G*pixIn.G)).to_int();
      pixOut.B = (ac_fixed<CDEPTH, CDEPTH, false, AC_RND, AC_SAT>(ratio.B*pixIn.B)).to_int();
      // TUSER and TLAST flags are copied to the output as-is.
      pixOut.TUSER = pixIn.TUSER;
      pixOut.TLAST = pixIn.TLAST;
#pragma hls_waive CNS
      if (!userSuppliedWP) {
        RGBSum = pixIn.R + pixIn.G + pixIn.B;
        if (RGBSum >= maxRGBSum) {
          // We type-cast to ac_fixed<8, 8, false> in order to avoid FXD violations in CDesignChecker.
          frameVal.R = ac_fixed<8, 8, false>(pixIn.R >> shiftBy);
          frameVal.G = ac_fixed<8, 8, false>(pixIn.G >> shiftBy);
          frameVal.B = ac_fixed<8, 8, false>(pixIn.B >> shiftBy);
          maxRGBSum = RGBSum;
        }
      }
      streamOut.write(pixOut);
      if (i == widthIn*heightIn - 1) { break; }
    }
  }

  ac_ctc() {
    static_assert(TEMP_MAX <= tempLimitUpp, "TEMP_MAX must not exceed upper temperature limit supported by design LUTs.");
    static_assert((R_WP == 0 && G_WP == 0 && B_WP == 0) || userSuppliedWP, "Mixing of zero and non-zero values for user-supplied white point is not allowed.");
    static_assert(R_WP <= 255 && G_WP <= 255 && B_WP <= 255, "All user defined white point values must not exceed 255.");

    if (userSuppliedWP) {
      // We type-cast to ac_fixed<8, 8, false> in order to avoid FXD violations in CDesignChecker.
      frameVal.R = ac_fixed<8, 8, false>(R_WP);
      frameVal.G = ac_fixed<8, 8, false>(G_WP);
      frameVal.B = ac_fixed<8, 8, false>(B_WP);
    } else {
      frameVal = 255.0; // Choose a default white point of R = 255, G = 255, B = 255 at the start of the processing, i.e. for the first frame.
    }
  }

private:
  ac_ipl::RGB_imd<ac_fixed<8, 8, false> > frameVal;

  enum {
    userSuppliedWP = R_WP > 0 && G_WP > 0 && B_WP > 0,
    shiftBy = CDEPTH - 8,
    n_f_b = 16,
  };

  // Each coefficient in the LUT varies from the last one in the sense that the temperature associated
  // increments by the value specified through the tempInc variable. e.g. If the temperature associated
  // with the second LUT coefficient is 1100 K and tempInc = 100, the third LUT coefficient will have a
  // temperature = 1100 + 100 = 1200 K, the fourth will have T = 1200 + 100 = 1300 K and so on.

  // The user can copy-paste their own LUT values by referring to the code found in ac_ctc_lutgen.cpp

  // Start of code outputted by ac_ctc_lutgen.cpp
  enum {
    tempLimitLow = 1000,
    tempLimitUpp = 13700,
    tempInc = 100,
    sizeTable = (tempLimitUpp - tempLimitLow)/tempInc + 1,
  };

  typedef ac_fixed<8, 8, false> refValsType;

  const refValsType refValsR[sizeTable] = {
    255.0, // T = 1000 K
    255.0, // T = 1100 K
    255.0, // T = 1200 K
    255.0, // T = 1300 K
    255.0, // T = 1400 K
    255.0, // T = 1500 K
    255.0, // T = 1600 K
    255.0, // T = 1700 K
    255.0, // T = 1800 K
    255.0, // T = 1900 K
    255.0, // T = 2000 K
    255.0, // T = 2100 K
    255.0, // T = 2200 K
    255.0, // T = 2300 K
    255.0, // T = 2400 K
    255.0, // T = 2500 K
    255.0, // T = 2600 K
    255.0, // T = 2700 K
    255.0, // T = 2800 K
    255.0, // T = 2900 K
    255.0, // T = 3000 K
    255.0, // T = 3100 K
    255.0, // T = 3200 K
    255.0, // T = 3300 K
    255.0, // T = 3400 K
    255.0, // T = 3500 K
    255.0, // T = 3600 K
    255.0, // T = 3700 K
    255.0, // T = 3800 K
    255.0, // T = 3900 K
    255.0, // T = 4000 K
    255.0, // T = 4100 K
    255.0, // T = 4200 K
    255.0, // T = 4300 K
    255.0, // T = 4400 K
    255.0, // T = 4500 K
    255.0, // T = 4600 K
    255.0, // T = 4700 K
    255.0, // T = 4800 K
    255.0, // T = 4900 K
    255.0, // T = 5000 K
    255.0, // T = 5100 K
    255.0, // T = 5200 K
    255.0, // T = 5300 K
    255.0, // T = 5400 K
    255.0, // T = 5500 K
    255.0, // T = 5600 K
    255.0, // T = 5700 K
    255.0, // T = 5800 K
    255.0, // T = 5900 K
    255.0, // T = 6000 K
    255.0, // T = 6100 K
    255.0, // T = 6200 K
    255.0, // T = 6300 K
    255.0, // T = 6400 K
    255.0, // T = 6500 K
    255.0, // T = 6600 K
    254.0, // T = 6700 K
    250.0, // T = 6800 K
    246.0, // T = 6900 K
    243.0, // T = 7000 K
    240.0, // T = 7100 K
    237.0, // T = 7200 K
    234.0, // T = 7300 K
    232.0, // T = 7400 K
    230.0, // T = 7500 K
    228.0, // T = 7600 K
    226.0, // T = 7700 K
    224.0, // T = 7800 K
    223.0, // T = 7900 K
    221.0, // T = 8000 K
    220.0, // T = 8100 K
    218.0, // T = 8200 K
    217.0, // T = 8300 K
    216.0, // T = 8400 K
    215.0, // T = 8500 K
    214.0, // T = 8600 K
    213.0, // T = 8700 K
    212.0, // T = 8800 K
    211.0, // T = 8900 K
    210.0, // T = 9000 K
    209.0, // T = 9100 K
    208.0, // T = 9200 K
    207.0, // T = 9300 K
    206.0, // T = 9400 K
    205.0, // T = 9500 K
    205.0, // T = 9600 K
    204.0, // T = 9700 K
    203.0, // T = 9800 K
    202.0, // T = 9900 K
    202.0, // T = 10000 K
    201.0, // T = 10100 K
    200.0, // T = 10200 K
    200.0, // T = 10300 K
    199.0, // T = 10400 K
    199.0, // T = 10500 K
    198.0, // T = 10600 K
    197.0, // T = 10700 K
    197.0, // T = 10800 K
    196.0, // T = 10900 K
    196.0, // T = 11000 K
    195.0, // T = 11100 K
    195.0, // T = 11200 K
    194.0, // T = 11300 K
    194.0, // T = 11400 K
    193.0, // T = 11500 K
    193.0, // T = 11600 K
    192.0, // T = 11700 K
    192.0, // T = 11800 K
    192.0, // T = 11900 K
    191.0, // T = 12000 K
    191.0, // T = 12100 K
    190.0, // T = 12200 K
    190.0, // T = 12300 K
    189.0, // T = 12400 K
    189.0, // T = 12500 K
    189.0, // T = 12600 K
    188.0, // T = 12700 K
    188.0, // T = 12800 K
    188.0, // T = 12900 K
    187.0, // T = 13000 K
    187.0, // T = 13100 K
    187.0, // T = 13200 K
    186.0, // T = 13300 K
    186.0, // T = 13400 K
    186.0, // T = 13500 K
    185.0, // T = 13600 K
    185.0  // T = 13700 K
  };

  const refValsType refValsG[sizeTable] = {
    68.0,  // T = 1000 K
    77.0,  // T = 1100 K
    86.0,  // T = 1200 K
    94.0,  // T = 1300 K
    101.0, // T = 1400 K
    108.0, // T = 1500 K
    115.0, // T = 1600 K
    121.0, // T = 1700 K
    126.0, // T = 1800 K
    132.0, // T = 1900 K
    137.0, // T = 2000 K
    142.0, // T = 2100 K
    146.0, // T = 2200 K
    151.0, // T = 2300 K
    155.0, // T = 2400 K
    159.0, // T = 2500 K
    163.0, // T = 2600 K
    167.0, // T = 2700 K
    170.0, // T = 2800 K
    174.0, // T = 2900 K
    177.0, // T = 3000 K
    180.0, // T = 3100 K
    184.0, // T = 3200 K
    187.0, // T = 3300 K
    190.0, // T = 3400 K
    193.0, // T = 3500 K
    195.0, // T = 3600 K
    198.0, // T = 3700 K
    201.0, // T = 3800 K
    203.0, // T = 3900 K
    206.0, // T = 4000 K
    208.0, // T = 4100 K
    211.0, // T = 4200 K
    213.0, // T = 4300 K
    215.0, // T = 4400 K
    218.0, // T = 4500 K
    220.0, // T = 4600 K
    222.0, // T = 4700 K
    224.0, // T = 4800 K
    226.0, // T = 4900 K
    228.0, // T = 5000 K
    230.0, // T = 5100 K
    232.0, // T = 5200 K
    234.0, // T = 5300 K
    236.0, // T = 5400 K
    237.0, // T = 5500 K
    239.0, // T = 5600 K
    241.0, // T = 5700 K
    243.0, // T = 5800 K
    244.0, // T = 5900 K
    246.0, // T = 6000 K
    248.0, // T = 6100 K
    249.0, // T = 6200 K
    251.0, // T = 6300 K
    253.0, // T = 6400 K
    254.0, // T = 6500 K
    255.0, // T = 6600 K
    249.0, // T = 6700 K
    246.0, // T = 6800 K
    244.0, // T = 6900 K
    242.0, // T = 7000 K
    240.0, // T = 7100 K
    239.0, // T = 7200 K
    237.0, // T = 7300 K
    236.0, // T = 7400 K
    235.0, // T = 7500 K
    234.0, // T = 7600 K
    233.0, // T = 7700 K
    232.0, // T = 7800 K
    231.0, // T = 7900 K
    230.0, // T = 8000 K
    229.0, // T = 8100 K
    228.0, // T = 8200 K
    227.0, // T = 8300 K
    227.0, // T = 8400 K
    226.0, // T = 8500 K
    225.0, // T = 8600 K
    225.0, // T = 8700 K
    224.0, // T = 8800 K
    223.0, // T = 8900 K
    223.0, // T = 9000 K
    222.0, // T = 9100 K
    222.0, // T = 9200 K
    221.0, // T = 9300 K
    221.0, // T = 9400 K
    220.0, // T = 9500 K
    220.0, // T = 9600 K
    219.0, // T = 9700 K
    219.0, // T = 9800 K
    218.0, // T = 9900 K
    218.0, // T = 10000 K
    218.0, // T = 10100 K
    217.0, // T = 10200 K
    217.0, // T = 10300 K
    217.0, // T = 10400 K
    216.0, // T = 10500 K
    216.0, // T = 10600 K
    215.0, // T = 10700 K
    215.0, // T = 10800 K
    215.0, // T = 10900 K
    214.0, // T = 11000 K
    214.0, // T = 11100 K
    214.0, // T = 11200 K
    213.0, // T = 11300 K
    213.0, // T = 11400 K
    213.0, // T = 11500 K
    213.0, // T = 11600 K
    212.0, // T = 11700 K
    212.0, // T = 11800 K
    212.0, // T = 11900 K
    211.0, // T = 12000 K
    211.0, // T = 12100 K
    211.0, // T = 12200 K
    211.0, // T = 12300 K
    210.0, // T = 12400 K
    210.0, // T = 12500 K
    210.0, // T = 12600 K
    210.0, // T = 12700 K
    210.0, // T = 12800 K
    209.0, // T = 12900 K
    209.0, // T = 13000 K
    209.0, // T = 13100 K
    209.0, // T = 13200 K
    208.0, // T = 13300 K
    208.0, // T = 13400 K
    208.0, // T = 13500 K
    208.0, // T = 13600 K
    208.0  // T = 13700 K
  };

  const refValsType refValsB[sizeTable] = {
    0.0,   // T = 1000 K
    0.0,   // T = 1100 K
    0.0,   // T = 1200 K
    0.0,   // T = 1300 K
    0.0,   // T = 1400 K
    0.0,   // T = 1500 K
    0.0,   // T = 1600 K
    0.0,   // T = 1700 K
    0.0,   // T = 1800 K
    0.0,   // T = 1900 K
    14.0,  // T = 2000 K
    27.0,  // T = 2100 K
    39.0,  // T = 2200 K
    50.0,  // T = 2300 K
    61.0,  // T = 2400 K
    70.0,  // T = 2500 K
    79.0,  // T = 2600 K
    87.0,  // T = 2700 K
    95.0,  // T = 2800 K
    103.0, // T = 2900 K
    110.0, // T = 3000 K
    117.0, // T = 3100 K
    123.0, // T = 3200 K
    129.0, // T = 3300 K
    135.0, // T = 3400 K
    141.0, // T = 3500 K
    146.0, // T = 3600 K
    151.0, // T = 3700 K
    157.0, // T = 3800 K
    161.0, // T = 3900 K
    166.0, // T = 4000 K
    171.0, // T = 4100 K
    175.0, // T = 4200 K
    179.0, // T = 4300 K
    183.0, // T = 4400 K
    187.0, // T = 4500 K
    191.0, // T = 4600 K
    195.0, // T = 4700 K
    199.0, // T = 4800 K
    202.0, // T = 4900 K
    206.0, // T = 5000 K
    209.0, // T = 5100 K
    213.0, // T = 5200 K
    216.0, // T = 5300 K
    219.0, // T = 5400 K
    222.0, // T = 5500 K
    225.0, // T = 5600 K
    228.0, // T = 5700 K
    231.0, // T = 5800 K
    234.0, // T = 5900 K
    237.0, // T = 6000 K
    240.0, // T = 6100 K
    242.0, // T = 6200 K
    245.0, // T = 6300 K
    248.0, // T = 6400 K
    250.0, // T = 6500 K
    255.0, // T = 6600 K
    255.0, // T = 6700 K
    255.0, // T = 6800 K
    255.0, // T = 6900 K
    255.0, // T = 7000 K
    255.0, // T = 7100 K
    255.0, // T = 7200 K
    255.0, // T = 7300 K
    255.0, // T = 7400 K
    255.0, // T = 7500 K
    255.0, // T = 7600 K
    255.0, // T = 7700 K
    255.0, // T = 7800 K
    255.0, // T = 7900 K
    255.0, // T = 8000 K
    255.0, // T = 8100 K
    255.0, // T = 8200 K
    255.0, // T = 8300 K
    255.0, // T = 8400 K
    255.0, // T = 8500 K
    255.0, // T = 8600 K
    255.0, // T = 8700 K
    255.0, // T = 8800 K
    255.0, // T = 8900 K
    255.0, // T = 9000 K
    255.0, // T = 9100 K
    255.0, // T = 9200 K
    255.0, // T = 9300 K
    255.0, // T = 9400 K
    255.0, // T = 9500 K
    255.0, // T = 9600 K
    255.0, // T = 9700 K
    255.0, // T = 9800 K
    255.0, // T = 9900 K
    255.0, // T = 10000 K
    255.0, // T = 10100 K
    255.0, // T = 10200 K
    255.0, // T = 10300 K
    255.0, // T = 10400 K
    255.0, // T = 10500 K
    255.0, // T = 10600 K
    255.0, // T = 10700 K
    255.0, // T = 10800 K
    255.0, // T = 10900 K
    255.0, // T = 11000 K
    255.0, // T = 11100 K
    255.0, // T = 11200 K
    255.0, // T = 11300 K
    255.0, // T = 11400 K
    255.0, // T = 11500 K
    255.0, // T = 11600 K
    255.0, // T = 11700 K
    255.0, // T = 11800 K
    255.0, // T = 11900 K
    255.0, // T = 12000 K
    255.0, // T = 12100 K
    255.0, // T = 12200 K
    255.0, // T = 12300 K
    255.0, // T = 12400 K
    255.0, // T = 12500 K
    255.0, // T = 12600 K
    255.0, // T = 12700 K
    255.0, // T = 12800 K
    255.0, // T = 12900 K
    255.0, // T = 13000 K
    255.0, // T = 13100 K
    255.0, // T = 13200 K
    255.0, // T = 13300 K
    255.0, // T = 13400 K
    255.0, // T = 13500 K
    255.0, // T = 13600 K
    255.0  // T = 13700 K
  };
  // End of code outputted by ac_ctc_lutgen.cpp
};
#endif
