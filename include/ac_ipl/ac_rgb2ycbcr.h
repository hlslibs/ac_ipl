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
 *  Copyright 2025 Siemens                                                *
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
#ifndef _INCLUDED_AC_CSC_H_
#define _INCLUDED_AC_CSC_H_

#if (defined(__GNUC__) && (__cplusplus < 201103L))
#error Please use C++11 or a later standard for compilation.
#endif
#if (defined(_MSC_VER) && (_MSC_VER < 1920) && !defined(__EDG__))
#error Please use Microsoft VS 2019 or a later standard for compilation.
#endif

#include <ac_int.h>
// Include headers for data types supported by these implementations
#include <ac_fixed.h>
#include <ac_channel.h>
#include <ac_matrix.h>
#include <ac_ipl/ac_pixels.h>

#if !defined(__SYNTHESIS__) && defined(AC_CSC_H_DEBUG)
#include <iostream>
using namespace std;
#endif

namespace ac_csc {

  template <int FractBits, typename Standard, typename PixIn_type, int AcImgHeight, int AcImgWidth>
  struct standard_checker;

  // Specialization for BT601
  template <int FractBits, typename PixIn_type, int AcImgHeight, int AcImgWidth>
  struct standard_checker<FractBits, ac_ipl::BT601<FractBits>, PixIn_type, AcImgHeight, AcImgWidth> {
    static void check() {
      static_assert(PixIn_type::base_type::width == 8 || PixIn_type::base_type::width == 10,
                    "Both 8-bit and 10-bit depth encoding for BT.601 is supported.");
      static_assert(AcImgHeight <= 576,
                    "Maximum Height should be less than or equal to 576 as BT.601 standard is specifically defined for SDTV images.");
      static_assert(AcImgWidth <= 720,
                    "Maximum Width should be less than or equal to 720 as BT.601 standard is specifically defined for SDTV images.");
    }
  };

  // Specialization for BT709
  template <int FractBits, typename PixIn_type, int AcImgHeight, int AcImgWidth>
  struct standard_checker<FractBits, ac_ipl::BT709<FractBits>, PixIn_type, AcImgHeight, AcImgWidth> {
    static void check() {
      static_assert(PixIn_type::base_type::width == 8 || PixIn_type::base_type::width == 10,
                    "Both 8-bit and 10-bit depth encoding for BT.709 is supported.");
      static_assert(AcImgHeight <= 1080,
                    "Maximum Height should be less than or equal to 1080 as BT.709 standard is specifically defined for HDTV images.");
      static_assert(AcImgWidth <= 1920,
                    "Maximum Width should be less than or equal to 1920 as BT.709 standard is specifically defined for HDTV images.");
    }
  };

  // Specialization for BT2020
  template <int FractBits, typename PixIn_type, int AcImgHeight, int AcImgWidth>
  struct standard_checker<FractBits, ac_ipl::BT2020<FractBits>, PixIn_type, AcImgHeight, AcImgWidth> {
    static void check() {
      static_assert(PixIn_type::base_type::width == 10 || PixIn_type::base_type::width == 12,
                    "Both 10-bit and 12-bit depth encoding for BT.2020 is supported.");
      static_assert(AcImgHeight <= 4320,
                    "Maximum Height should be less than or equal to 4320 as BT.2020 standard is specifically defined for UHD images.");
      static_assert(AcImgWidth <= 7680,
                    "Maximum Width should be less than or equal to 7680 as BT.2020 standard is specifically defined for UHD images.");
    }
  };

  // Specialization for BT2100
  template <int FractBits, typename PixIn_type, int AcImgHeight, int AcImgWidth>
  struct standard_checker<FractBits, ac_ipl::BT2100<FractBits>, PixIn_type, AcImgHeight, AcImgWidth> {
    static void check() {
      static_assert(PixIn_type::base_type::width == 10 || PixIn_type::base_type::width == 12,
                    "Both 10-bit and 12-bit depth encoding for BT.2100 is supported.");
      static_assert(AcImgHeight <= 4320,
                    "Maximum Height should be less than or equal to 4320 as BT.2100 standard is specifically defined for UHD and HDR images.");
      static_assert(AcImgWidth <= 7680,
                    "Maximum Width should be less than or equal to 7680 as BT.2100 standard is specifically defined for UHD and HDR images.");
    }
  };

  //Ac_matrix implementation with fixed point coeffs
  template <typename PixIn_type, typename PixOut_type, int AcImgHeight, int AcImgWidth, ac_q_mode Q = AC_TRN, int FractBits = 18, bool StudioSwing = false, typename Standard = ac_ipl::BT601<FractBits>>
  class ac_rgb2ycbcr
  {
  public:

    //Number of bits used for encoding (e.g., 8-bit or 10-bit)
    static constexpr int CDEPTH = PixIn_type::base_type::width;
    //Typedefs for matrix values
    typedef ac_matrix<ac_fixed<1 + FractBits, 1, true>, 3, 3> constCoeffMatrix_type;
    typedef ac_matrix<ac_fixed<CDEPTH, CDEPTH, false>, 3, 1> rgbMatrix_type;
    typedef ac_matrix<ac_fixed<CDEPTH + 2 + FractBits, CDEPTH + 2, true>, 3, 1> multMatrix_type;
    
    typedef ac_int<ac::nbits<AcImgHeight*AcImgWidth - 1>::val, false> cnt_type;

    rgbMatrix_type rgbMatrix;

    #pragma hls_pipeline_init_interval 1
    void cvtColor (
      ac_channel<PixIn_type> &din_ch,
      ac_channel<PixOut_type> &dout_ch,
      const ac_int<ac::nbits<AcImgHeight>::val, false> &height,
      const ac_int<ac::nbits<AcImgWidth>::val, false> &width
    ) {

      bool eof = false;
      cnt_type cnt = 0;

      // Perform compile-time checks based on Standard
      standard_checker<FractBits, Standard, PixIn_type, AcImgHeight, AcImgWidth>::check();

      AC_ASSERT(height <= AcImgHeight, "The height exceeds the maximum height.");
      AC_ASSERT(width <= AcImgWidth, "The width exceeds the maximum width.");

      constCoeffMatrix_type rgb2ycbcrMatrix;

      rgb2ycbcrMatrix = Standard::getFullSwingMatrix();

      do {
          PixIn_type din = din_ch.read();
          PixOut_type dout;

          //RGB input matrix
          rgbMatrix(0,0) = (typename rgbMatrix_type::type)din.get_R();
          rgbMatrix(1,0) = (typename rgbMatrix_type::type)din.get_G();
          rgbMatrix(2,0) = (typename rgbMatrix_type::type)din.get_B();

          dout = ac_ipl::ac_rgb_2_ycbcr<Standard, StudioSwing, CDEPTH, FractBits, Q, multMatrix_type>(rgb2ycbcrMatrix, rgbMatrix);

          dout_ch.write(dout);
          eof = (cnt == height*width - 1);
          cnt++;        
      } while (!eof);

    }

  };
};

#endif


