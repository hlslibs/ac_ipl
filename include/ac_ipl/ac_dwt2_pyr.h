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
// File: ac_dwt2_pyr.h
//
// Description:
//  Code for the hardware implementation of the Wavelet Pyramid algorithm with streaming inputs
//  and outputs. Image dimensions are dynamic with static maximum values provided. Greyscale and color
//  images are supported.
//
//  The library currently supports Haar and Daubechies 2 wavelet pyramids.
//
// Notes:
//  Attempting to call the function with a type that is not implemented will result
//  in a compile error.
//  Number of levels must be positive and at most 10.
//
// Revision History:
//    3.4.3  - dgb - Updated compiler checks to work with MS VS 2019
//    3.4.0  - Added library.
//*********************************************************************************************************

#ifndef _INCLUDED_AC_DWT2_PYR_H_
#define _INCLUDED_AC_DWT2_PYR_H_

#include <ac_int.h>
#include <ac_fixed.h>
#include <ac_channel.h>
#include <ac_ipl/ac_pixels.h>
#include <ac_window_2d_flag_flush_support.h>

// The design uses static_asserts, which are only supported by C++11 or later compiler standards.
// The #error directive below informs the user if they're not using those standards.
#if (defined(__GNUC__) && (__cplusplus < 201103L))
#error Please use C++11 or a later standard for compilation.
#endif
#if (defined(_MSC_VER) && (_MSC_VER < 1920) && !defined(__EDG__))
#error Please use Microsoft VS 2019 or a later standard for compilation.
#endif

#ifndef __SYNTHESIS__
#include <iostream>
#include <string>
#endif

enum ac_dwt2_function { AC_HAAR, AC_DB2 };
// IMPORTANT: If you add one or more wavelet function LUTs with the LUT generator file, make sure you
// add the enumerator for the new wavelet function to the enum list above. For instance, if you add
// an enumerator called "AC_DB4", edit the enum as shown in the comment below:
// enum ac_dwt2_function { AC_HAAR, AC_DB2, AC_DB4 };

template <ac_dwt2_function DWT_FN_VAL>
struct kernel_values_struct {};

// Specialized definitions of kernel_values_struct for your own custom wavelet functions can be printed
// in C++ syntax using the file "ac_dwt2_pyr_lutgen.cpp". By default, this header provides structs for
// the Haar and Daubechies-2 wavelets.

template<>
struct kernel_values_struct <AC_HAAR> {
  enum { K_SZ = 2 }; // Size of 2D kernel LUTs

  typedef ac_fixed<2, 1, true> T; // Type used to store LUT values.

  // 2D Kernel LUT used to calculate approximation coefficients.
  const T k_ap[K_SZ][K_SZ] = {
    {.5, .5},
    {.5, .5}
  };

  // 2D Kernel LUT used to calculate horizontal detail coefficients.
  const T k_ho[K_SZ][K_SZ] = {
    {-.5, -.5},
    {.5, .5}
  };

  // 2D Kernel LUT used to calculate vertical detail coefficients.
  const T k_ve[K_SZ][K_SZ] = {
    {-.5, .5},
    {-.5, .5}
  };

  // 2D Kernel LUT used to calculate diagonal detail coefficients.
  const T k_di[K_SZ][K_SZ] = {
    {.5, -.5},
    {-.5, .5}
  };
};

template<>
struct kernel_values_struct <AC_DB2> {
  enum { K_SZ = 4 }; // Size of 2D kernel LUTs

  typedef ac_fixed<33, 1, true> T; // Type used to store LUT values.

  // 2D Kernel LUT used to calculate approximation coefficients.
  const T k_ap[K_SZ][K_SZ] = {
    {.01674682460725307464599609375, -.02900635101832449436187744140625, -.10825317539274692535400390625, -.0625},
    {-.02900635101832449436187744140625, .05024047358892858028411865234375, .1875, .10825317539274692535400390625},
    {-.10825317539274692535400390625, .1875, .69975952641107141971588134765625, .40400635101832449436187744140625},
    {-.0625, .10825317539274692535400390625, .40400635101832449436187744140625, .23325317539274692535400390625}
  };

  // 2D Kernel LUT used to calculate horizontal detail coefficients.
  const T k_ho[K_SZ][K_SZ] = {
    {.0625, -.10825317539274692535400390625, -.40400635101832449436187744140625, -.23325317539274692535400390625},
    {-.10825317539274692535400390625, .1875, .69975952641107141971588134765625, .40400635101832449436187744140625},
    {.02900635101832449436187744140625, -.05024047358892858028411865234375, -.1875, -.10825317539274692535400390625},
    {.01674682460725307464599609375, -.02900635101832449436187744140625, -.10825317539274692535400390625, -.0625}
  };

  // 2D Kernel LUT used to calculate vertical detail coefficients.
  const T k_ve[K_SZ][K_SZ] = {
    {.0625, -.10825317539274692535400390625, .02900635101832449436187744140625, .01674682460725307464599609375},
    {-.10825317539274692535400390625, .1875, -.05024047358892858028411865234375, -.02900635101832449436187744140625},
    {-.40400635101832449436187744140625, .69975952641107141971588134765625, -.1875, -.10825317539274692535400390625},
    {-.23325317539274692535400390625, .40400635101832449436187744140625, -.10825317539274692535400390625, -.0625}
  };

  // 2D Kernel LUT used to calculate diagonal detail coefficients.
  const T k_di[K_SZ][K_SZ] = {
    {.23325317539274692535400390625, -.40400635101832449436187744140625, .10825317539274692535400390625, .0625},
    {-.40400635101832449436187744140625, .69975952641107141971588134765625, -.1875, -.10825317539274692535400390625},
    {.10825317539274692535400390625, -.1875, .05024047358892858028411865234375, .02900635101832449436187744140625},
    {.0625, -.10825317539274692535400390625, .02900635101832449436187744140625, .01674682460725307464599609375}
  };
};

template <class T>
struct out_struct {
  T out_ap;
  T out_ho;
  T out_ve;
  T out_di;
};

// Generic dwt2_pyr_type_info struct. Is later specialized to deal with different types.
template <ac_dwt2_function DWT_FN_VAL, class T, int LEVEL_CNT, int ACC_W, int ACC_I>
struct dwt2_pyr_type_info { };

// dwt2_pyr_type_info, specialized for greyscale inputs/outputs.
template <ac_dwt2_function DWT_FN_VAL, int W, int I, bool S, ac_q_mode Q, ac_o_mode O, int LEVEL_CNT, int ACC_W, int ACC_I>
struct dwt2_pyr_type_info<DWT_FN_VAL, ac_fixed<W, I, S, Q, O>, LEVEL_CNT, ACC_W, ACC_I> {
  typedef ac_fixed<ACC_W, ACC_I, true> acc_type; // accumulator type.
  typedef typename kernel_values_struct<DWT_FN_VAL>::T kernel_type; // Kernel type for storage of kernel values.
};

// dwt2_pyr_type_info, specialized for color inputs/outputs.
template <ac_dwt2_function DWT_FN_VAL, int W, int I, bool S, ac_q_mode Q, ac_o_mode O, int LEVEL_CNT, int ACC_W, int ACC_I>
struct dwt2_pyr_type_info<DWT_FN_VAL, ac_ipl::RGB_imd<ac_fixed<W, I, S, Q, O> >, LEVEL_CNT, ACC_W, ACC_I> {
  typedef ac_ipl::RGB_imd<ac_fixed<ACC_W, ACC_I, true> > acc_type; // accumulator type.
  typedef typename kernel_values_struct<DWT_FN_VAL>::T kernel_fp_type;
  typedef ac_ipl::RGB_imd<kernel_fp_type> kernel_type; // Kernel type for storage of kernel values.
};

template <ac_dwt2_function DWT_FN_VAL, class IN_TYPE, class OUT_TYPE, class W_IN_TYPE, class H_IN_TYPE, int IN_W_MAX, int IN_H_MAX, int LEVEL_CNT, int ACC_W, int ACC_I, bool USE_SP>
class ac_dwt2_pyr_block
{
public:
  typedef dwt2_pyr_type_info<DWT_FN_VAL, OUT_TYPE, LEVEL_CNT, ACC_W, ACC_I> st_def;
  // accumulator type.
  typedef typename st_def::acc_type acc_type;
  typedef out_struct<OUT_TYPE> OUT_ST_TYPE;

  ac_dwt2_pyr_block() { }

  #pragma hls_design interface
  void run (
    ac_channel<IN_TYPE>  &stream_in, // Input stream.
    ac_channel<acc_type> &stream_inter, // Interconnect output stream.
    ac_channel<OUT_ST_TYPE> &stream_out, // Stream that sends output to external consumer.
    const W_IN_TYPE w_in, // Dimension input: Level 1 input width.
    const H_IN_TYPE h_in // Dimension input: Level 1 input height.
  ) {
    // The number of bits required for storing dimension inputs reduces by 1 at every level, due to halving of dimension values. Store
    // the dimension input values in variables which take this reduction into account.
    ac_int<ac::nbits<IN_W_MAX>::val, false> w_in_lev = (w_in >> LEVEL_CNT);
    ac_int<ac::nbits<IN_H_MAX>::val, false> h_in_lev = (h_in >> LEVEL_CNT);
    
    #ifndef __SYNTHESIS__
    std::string input_width_assert_msg = "Input width for level ";
    input_width_assert_msg += std::to_string(LEVEL_CNT + 1);
    input_width_assert_msg += " must be greater than Kernel Size.";
    std::string input_height_assert_msg = "Input height for level ";
    input_height_assert_msg += std::to_string(LEVEL_CNT + 1);
    input_height_assert_msg += " must be greater than Kernel Size.";
    
    AC_ASSERT(w_in_lev > K_SZ, (char *)input_width_assert_msg.c_str());
    AC_ASSERT(h_in_lev > K_SZ, (char *)input_height_assert_msg.c_str());
    #endif

    #pragma hls_pipeline_init_interval 1
    ROW_LOOP: for (int i = 0; i < IN_H_MAX + K_SZ/2 - 1; i++) {
      bool eof_out; // EOF value for output frame. Since this value is used to break out of ROW_LOOP, we must declare it as a variable in ROW_LOOP and not COL_LOOP.
      #pragma hls_pipeline_init_interval 1
      COL_LOOP: for (int j = 0; j < IN_W_MAX + K_SZ/2 - 1; j++) {
        IN_TYPE pix_in;
        // Fill in streaming pixel inputs if we're in the image region.
        if (i < h_in_lev && j < w_in_lev) {
          pix_in = stream_in.read();
        } else {
          pix_in = 0.0;
        }
        // Set framing signals for input pixel based on loop iterator variables.
        bool sol_in = (j == 0);
        bool sof_in = (i == 0 && sol_in);
        bool eol_in = (j == w_in_lev - 1);
        bool eof_in = (i == h_in_lev - 1 && eol_in);
        // Write value to window object/linebuffers.
        win_inst.write(pix_in, sof_in, eof_in, sol_in, eol_in);
        bool sof_out, sol_out, eol_out; // Framing signals for output.
        win_inst.readFlags(sof_out, eof_out, sol_out, eol_out);

        if (win_inst.valid() && i%2 == int(K_SZ%4 != 0) && j%2 == int(K_SZ%4 != 0)) {
          out_struct<acc_type> acc_var;
          acc_var.out_ap = 0.0;
          acc_var.out_ho = 0.0;
          acc_var.out_ve = 0.0;
          acc_var.out_di = 0.0;
          // Window values are stored in temporary array to enable easy debugging via GDB.
          IN_TYPE win_arr[K_SZ][K_SZ];
          #pragma hls_unroll yes
          DWT2_PYR_FILT_ROW: for (int r = 0; r < K_SZ; r++) {
            #pragma hls_unroll yes
            DWT2_PYR_FILT_COL: for (int c = 0; c < K_SZ; c++) {
              win_arr[r][c] = win_inst(r - K_SZ/2, c - K_SZ/2); // Fill in window values into temp. array.
              kernel_values_type k_v_i;
              acc_var.out_ap += win_arr[r][c]*kernel_type(k_v_i.k_ap[r][c]);
              acc_var.out_ho += win_arr[r][c]*kernel_type(k_v_i.k_ho[r][c]);
              acc_var.out_ve += win_arr[r][c]*kernel_type(k_v_i.k_ve[r][c]);
              acc_var.out_di += win_arr[r][c]*kernel_type(k_v_i.k_di[r][c]);
            }
          }
          stream_inter.write(acc_var.out_ap);
          OUT_ST_TYPE out_st_inst;
          out_st_inst.out_ap = acc_var.out_ap;
          out_st_inst.out_ho = acc_var.out_ho;
          out_st_inst.out_ve = acc_var.out_ve;
          out_st_inst.out_di = acc_var.out_di;
          stream_out.write(out_st_inst);
        }

        if (eol_out) {
          break; // Break once we've reached rightmost pixel of output frame.
        }
      }
      if (eof_out) {
        break; // Break once we've reached last pixel in output frame.
      }
    }
  }

private:
  typedef kernel_values_struct<DWT_FN_VAL> kernel_values_type;

  enum {
    // USE_SP configures the type of linebuffers used by ac_window object. If USE_SP is true, AC_SINGLEPORT is part of the window mode data.
    win_mode = USE_SP ? AC_CLIP | AC_SINGLEPORT : AC_CLIP,
    K_SZ = kernel_values_type::K_SZ,
  };

  static_assert(K_SZ%2 == 0, "Kernel size must be even to ensure the correct functioning of loops.");

  typedef typename st_def::kernel_type kernel_type;  
  typedef ac_window_2d_flag_flush_support<IN_TYPE, K_SZ, K_SZ, AC_MAX(IN_W_MAX, K_SZ + 1), win_mode> win_type;
  // Instantiate ac_window.
  win_type win_inst;
};

// Type checking struct for ac_dwt2_pyr. It's later specialized to deal with different types.
template <class T1, class T2>
struct dwt2_pyr_type_check {
  enum { match = false };
};

// Type checking struct, specialized for greyscale inputs and outputs.
template <int W1, int W2, int I, bool S, ac_q_mode Q2, ac_o_mode O2>
struct dwt2_pyr_type_check<ac_int<W1, false>, ac_fixed<W2, I, S, Q2, O2> > {
  enum { match = true };
};

// Type checking struct, specialized for color inputs and outputs.
template <int W1, int W2, int I, bool S, ac_q_mode Q2, ac_o_mode O2>
struct dwt2_pyr_type_check<ac_ipl::RGB_imd<ac_int<W1, false> >, ac_ipl::RGB_imd<ac_fixed<W2, I, S, Q2, O2> > > {
  enum { match = true };
};

#include <mc_scverify.h>

#pragma hls_design top
template <ac_dwt2_function DWT_FN_VAL, class IN_TYPE, class OUT_TYPE, int W_MAX, int H_MAX, int N_LEVELS, int ACC_W = 32, int ACC_I = 16, bool USE_SP = true>
class ac_dwt2_pyr
{
public:
  // Make sure that the user uses the correct input/output types.
  static_assert(dwt2_pyr_type_check<IN_TYPE, OUT_TYPE>::match, "Input and/or output type not supported by design. The following pairs of input and output types are supported:\n1. Input: Unsigned ac_int + Output: ac_fixed\n2. Input: ac_ipl::RGB_imd<Unsigned ac_int> + Output: ac_ipl::RGB_imd<ac_fixed>");
  static_assert(N_LEVELS <= 10, "Number of levels must not exceed 10.");
  static_assert(N_LEVELS >= 1, "Number of levels must be positive.");

  // Dimension types are bitwidth-constrained according to the max dimensions possible.
  typedef ac_int<ac::nbits<W_MAX>::val, false> W_IN_TYPE;
  typedef ac_int<ac::nbits<H_MAX>::val, false> H_IN_TYPE;

  #pragma hls_design interface
  void CCS_BLOCK(run) (
    ac_channel<IN_TYPE>  &stream_in, // Input stream
    ac_channel<out_struct<OUT_TYPE> > stream_out[N_LEVELS], // Output stream.
    const W_IN_TYPE w_in, // Dimension input: width.
    const H_IN_TYPE h_in // Dimension input: height.
  ) {
    // Call all level blocks with the correct input/output variables.
    ac_dwt2_pyr_block_inst_0.run(stream_in, stream_inter[0], stream_out[0], w_in, h_in);
    #pragma hls_waive CNS
    if (N_LEVELS >= 2) {
      ac_dwt2_pyr_block_inst_1.run(stream_inter[0], stream_inter[1], stream_out[1], w_in, h_in);
    }
    #pragma hls_waive CNS
    if (N_LEVELS >= 3) {
      ac_dwt2_pyr_block_inst_2.run(stream_inter[1], stream_inter[2], stream_out[2], w_in, h_in);
    }
    #pragma hls_waive CNS
    if (N_LEVELS >= 4) {
      ac_dwt2_pyr_block_inst_3.run(stream_inter[2], stream_inter[3], stream_out[3], w_in, h_in);
    }
    #pragma hls_waive CNS
    if (N_LEVELS >= 5) {
      ac_dwt2_pyr_block_inst_4.run(stream_inter[3], stream_inter[4], stream_out[4], w_in, h_in);
    }
    #pragma hls_waive CNS
    if (N_LEVELS >= 6) {
      ac_dwt2_pyr_block_inst_5.run(stream_inter[4], stream_inter[5], stream_out[5], w_in, h_in);
    }
    #pragma hls_waive CNS
    if (N_LEVELS >= 7) {
      ac_dwt2_pyr_block_inst_6.run(stream_inter[5], stream_inter[6], stream_out[6], w_in, h_in);
    }
    #pragma hls_waive CNS
    if (N_LEVELS >= 8) {
      ac_dwt2_pyr_block_inst_7.run(stream_inter[6], stream_inter[7], stream_out[7], w_in, h_in);
    }
    #pragma hls_waive CNS
    if (N_LEVELS >= 9) {
      ac_dwt2_pyr_block_inst_8.run(stream_inter[7], stream_inter[8], stream_out[8], w_in, h_in);
    }
    #pragma hls_waive CNS
    if (N_LEVELS >= 10) {
      ac_dwt2_pyr_block_inst_9.run(stream_inter[8], stream_inter[9], stream_out[9], w_in, h_in);
    }

  }

  ac_dwt2_pyr() { }

private:
  // Declare max input and output widths for each level.
  enum {
    IN_W_MAX_0  = W_MAX,
    IN_H_MAX_0  = H_MAX,
    OUT_W_MAX_0 = AC_MAX( IN_W_MAX_0/2, 1),
    OUT_W_MAX_1 = AC_MAX(OUT_W_MAX_0/2, 1),
    OUT_W_MAX_2 = AC_MAX(OUT_W_MAX_1/2, 1),
    OUT_W_MAX_3 = AC_MAX(OUT_W_MAX_2/2, 1),
    OUT_W_MAX_4 = AC_MAX(OUT_W_MAX_3/2, 1),
    OUT_W_MAX_5 = AC_MAX(OUT_W_MAX_4/2, 1),
    OUT_W_MAX_6 = AC_MAX(OUT_W_MAX_5/2, 1),
    OUT_W_MAX_7 = AC_MAX(OUT_W_MAX_6/2, 1),
    OUT_W_MAX_8 = AC_MAX(OUT_W_MAX_7/2, 1),
    OUT_W_MAX_9 = AC_MAX(OUT_W_MAX_8/2, 1),
    OUT_H_MAX_0 = AC_MAX( IN_H_MAX_0/2, 1),
    OUT_H_MAX_1 = AC_MAX(OUT_H_MAX_0/2, 1),
    OUT_H_MAX_2 = AC_MAX(OUT_H_MAX_1/2, 1),
    OUT_H_MAX_3 = AC_MAX(OUT_H_MAX_2/2, 1),
    OUT_H_MAX_4 = AC_MAX(OUT_H_MAX_3/2, 1),
    OUT_H_MAX_5 = AC_MAX(OUT_H_MAX_4/2, 1),
    OUT_H_MAX_6 = AC_MAX(OUT_H_MAX_5/2, 1),
    OUT_H_MAX_7 = AC_MAX(OUT_H_MAX_6/2, 1),
    OUT_H_MAX_8 = AC_MAX(OUT_H_MAX_7/2, 1),
    OUT_H_MAX_9 = AC_MAX(OUT_H_MAX_8/2, 1),
  };

  typedef typename dwt2_pyr_type_info<DWT_FN_VAL, OUT_TYPE, 0, ACC_W, ACC_I>::acc_type inter_type;

  ac_dwt2_pyr_block<DWT_FN_VAL, IN_TYPE,    OUT_TYPE, W_IN_TYPE, H_IN_TYPE,  IN_W_MAX_0,  IN_H_MAX_0, 0, ACC_W, ACC_I, USE_SP> ac_dwt2_pyr_block_inst_0;
  ac_dwt2_pyr_block<DWT_FN_VAL, inter_type, OUT_TYPE, W_IN_TYPE, H_IN_TYPE, OUT_W_MAX_0, OUT_H_MAX_0, 1, ACC_W, ACC_I, USE_SP> ac_dwt2_pyr_block_inst_1;
  ac_dwt2_pyr_block<DWT_FN_VAL, inter_type, OUT_TYPE, W_IN_TYPE, H_IN_TYPE, OUT_W_MAX_1, OUT_H_MAX_1, 2, ACC_W, ACC_I, USE_SP> ac_dwt2_pyr_block_inst_2;
  ac_dwt2_pyr_block<DWT_FN_VAL, inter_type, OUT_TYPE, W_IN_TYPE, H_IN_TYPE, OUT_W_MAX_2, OUT_H_MAX_2, 3, ACC_W, ACC_I, USE_SP> ac_dwt2_pyr_block_inst_3;
  ac_dwt2_pyr_block<DWT_FN_VAL, inter_type, OUT_TYPE, W_IN_TYPE, H_IN_TYPE, OUT_W_MAX_3, OUT_H_MAX_3, 4, ACC_W, ACC_I, USE_SP> ac_dwt2_pyr_block_inst_4;
  ac_dwt2_pyr_block<DWT_FN_VAL, inter_type, OUT_TYPE, W_IN_TYPE, H_IN_TYPE, OUT_W_MAX_4, OUT_H_MAX_4, 5, ACC_W, ACC_I, USE_SP> ac_dwt2_pyr_block_inst_5;
  ac_dwt2_pyr_block<DWT_FN_VAL, inter_type, OUT_TYPE, W_IN_TYPE, H_IN_TYPE, OUT_W_MAX_5, OUT_H_MAX_5, 6, ACC_W, ACC_I, USE_SP> ac_dwt2_pyr_block_inst_6;
  ac_dwt2_pyr_block<DWT_FN_VAL, inter_type, OUT_TYPE, W_IN_TYPE, H_IN_TYPE, OUT_W_MAX_6, OUT_H_MAX_6, 7, ACC_W, ACC_I, USE_SP> ac_dwt2_pyr_block_inst_7;
  ac_dwt2_pyr_block<DWT_FN_VAL, inter_type, OUT_TYPE, W_IN_TYPE, H_IN_TYPE, OUT_W_MAX_7, OUT_H_MAX_7, 8, ACC_W, ACC_I, USE_SP> ac_dwt2_pyr_block_inst_8;
  ac_dwt2_pyr_block<DWT_FN_VAL, inter_type, OUT_TYPE, W_IN_TYPE, H_IN_TYPE, OUT_W_MAX_8, OUT_H_MAX_8, 9, ACC_W, ACC_I, USE_SP> ac_dwt2_pyr_block_inst_9;

  // Declare interconnect channels.
  ac_channel<inter_type> stream_inter[10];
};

#endif
