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
//***************************************************************************
// File: ac_dwt_a.h
//
// Description:
//    The ac_dwt_a block is designed to calculate the Discrete Wavelet
//    Transform for a 1-Dimension signal. The design is a sample-based
//    streaming design. The output is down sampled by two. The High-Pass and
//    Low-Pass filters are implemented in Direct Form.
//
//                                                               Resampled Output Signal
//                                 +--------------------+                    |
//                -|               |                    |               -    |
//               / \ -             |                    |              / \   |         _ _
//       |--\   /   |              |                    |         |-_        |    --_ -   \_
//      /    --/    \        ----->|   Wavelet Block    |---->   /   -/   |  |   /   -/
//      |            |             |                    |        |        |  |  /
// ----/              -------      |                    |     ---          --|---          -
//                                 |                    |                    |
//         Input Signal            +--------------------+        High Pass   |    Low Pass
//
// Usage:
//    A sample testbench and its implementation look like
//    this:
//
//    #include <ac_fixed.h>
//    #include <ac_complex.h>
//    #include <ac_ipl/ac_dwt_a.h>
//
//    #include <iostream>
//    using namespace std;
//
//    CCS_MAIN(int arg, char **argc)
//    {
//      CCS_RETURN (0);
//    }
//    #endif
//
// Notes:
//    The design expects an even sample length input signal.
//
// Revision History:
//    3.4.3  - dgb - Updated compiler checks to work with MS VS 2019
//    3.2.0  - sdager - 4/6/20 - Initial version
//
//***************************************************************************

#ifndef _INCLUDED_AC_WAVELET_ANALYSIS_H_
#define _INCLUDED_AC_WAVELET_ANALYSIS_H_

#include <ac_int.h>
#include <ac_fixed.h>
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

// ac_dwt_a template parameters
// LENGTH: Length of input signal
// BPS:    Bits per Sample
// NFT:    Number of filter tabs
// CFP:    Coefficient Precision

template <int LENGTH, int BPS, int NFT = 4, int CFP = 19 >
class ac_dwt_a
{
  // Input type defined should be used for input and output stream
public:
  typedef ac_fixed< BPS, BPS, 0 > inputType;


public: // Constructor
  // Constant Filter Coefficients for Daubechies 4 Wavelet
  ac_dwt_a():
    coeff_H{ -0.129409522551260, -0.224143868042013, 0.836516303737808, -0.482962913144534 },
    coeff_L{  0.482962913144534,  0.836516303737808, 0.224143868042013, -0.129409522551260 } {
    count = 0;
    buff_add = 0;
    ac::init_array< AC_VAL_DC >( buffer_lp, LENGTH / 2 );
    ac::init_array< AC_VAL_0 >( shift_reg, NFT );
  }

public: // Top level interface to the design

  // input : Input streaming interface
  // output: Output straaming interface
  // dyn_len: Dynamic length of Signal Should be kept constant during simulation run
#pragma hls_pipeline_init_interval 1
#pragma hls_design interface
  void CCS_BLOCK( run )( ac_channel< inputType > &input,
                         ac_channel< inputType > &output,
                         ac_int< ac::nbits< LENGTH >::val, 0 > dyn_len ) {
    multType mult_out_h[ NFT ];
    multType mult_out_l[ NFT ];

    // Read input channel
    inputType inp_tmp = input.read();

    // Shift Register Loop
#pragma hls_unroll yes
    SHIFT_LOOP: for (int sft_itr = ( NFT - 1 ); sft_itr > 0; sft_itr--) {
      shift_reg[ sft_itr ] = shift_reg[ sft_itr - 1 ];
    }
    shift_reg[0] = inp_tmp;
    // Masking number is set according to given mask table for boundary
    if (count<NFT) {
      masking=((1<<(count+1))-1);
    } else {
      masking=15;
    }

    // Filter Multiplication Loop compute multiplication array.
#pragma hls_unroll yes
    MULT_LOOP: for ( int mul_itr = 0; mul_itr < NFT; mul_itr++ ) {
      mult_out_h[ mul_itr ] = shift_reg[ mul_itr ] * coeff_H[ NFT - 1 - mul_itr ] * masking[ mul_itr ];
      mult_out_l[ mul_itr ] = shift_reg[ mul_itr ] * coeff_L[ NFT - 1 - mul_itr ] * masking[ mul_itr ];
    }
    multType acc_h, acc_l;

    // Reseting Accumulator register
    acc_h = 0;
    acc_l = 0;
    // Accumulator loop generate adder trees for filters
#pragma hls_unroll yes
    ACC_LOOP: for ( int acc_itr = 0; acc_itr < NFT; acc_itr++ ) {
      acc_h += mult_out_h[ acc_itr ];
      acc_l += mult_out_l[ acc_itr ];
    }

    // Decimating odd samples results in
    if (!( count[0] )) {
      // Writing Output to Channel
      output.write((inputType_rnd_sat)(acc_h));
      // Buffring Low Pass filter output
      buffer_lp[buff_add] = (inputType_rnd_sat)(acc_l);
      buff_add++;
      if (buff_add == ( dyn_len >>1 )) { buff_add = 0; }
    } else {
      if (count == ( dyn_len - 1 )) {
        buff_add = 0;
        // Flushing LP Buffer to output channel
        LPF_LOOP: for ( int iter = 0; iter < LENGTH / 2; iter++ ) {
          output.write(buffer_lp[ iter ]);
          if ( iter == ((dyn_len>>1) - 1)) { break; }
        }
      }
    }
    count++;
    if (count == dyn_len) { count = 0; }
  }

private: // Internal types
  typedef ac_fixed< BPS, BPS, 0, AC_TRN_ZERO, AC_SAT > inputType_rnd_sat;
  // multiplication Type
  typedef ac_fixed< BPS + CFP, BPS + NFT, 1, AC_TRN_ZERO > multType;

private: // Data
  // Buffer for Decimated Low-Pass signal
  inputType buffer_lp[ LENGTH / 2 ];
  multType shift_reg[ NFT ];
  const multType coeff_H[ NFT ];
  const multType coeff_L[ NFT ];
  ac_int< NFT,0 > masking;
  ac_int< ac::nbits< LENGTH + 1 >::val, 0 > count;
  ac_int< ac::nbits< LENGTH / 2 >::val, 0 > buff_add;

};

#endif

