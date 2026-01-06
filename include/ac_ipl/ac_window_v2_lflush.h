/**************************************************************************
 *                                                                        *
 *  Algorithmic C (tm) Image Processing Library                           *
 *                                                                        *
 *  Software Version: 2025.4                                              *
 *                                                                        *
 *  Release Date    : Thu Dec 11 10:35:33 PST 2025                        *
 *  Release Type    : Production Release                                  *
 *  Release Build   : 2025.4.1                                            *
 *                                                                        *
 *  Copyright 2023 Siemens                                                *
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
//****************************************************************************
//  File : ac_window_v2_lflush.h
//
//  Description:
//    Top-level AC Window 2.0 design for Line Flushing. Integrates all the
//    relevant sub-blocks to carry out windowing. Only 2D windows are
//    supported.
//
//  ac_window_v2_lflush_2d template parameters:
//    - PIX_TYPE: Pixel type.
//    - AC_IMG_HEIGHT: Maximum possible image height.
//    - AC_IMG_WIDTH: Maximum possible image width.
//    - AC_WIN_HEIGHT: Window height.
//    - AC_WIN_WIDTH: Window width.
//    - BUFF_TYPE: Buffer type.
//    - AC_PMODE: Padding method/mode.
//    - AC_BUS_WORDS: Number of pixels passed on bus in parallel. Default
//        value is 1.
//    - HIGH_SPEED_REPLICATE: Template parameter used by boundary processing
//        class to switch between AC_REPLICATE architectures. Default value
//        is false.
//    - AC_REPEAT: Enables line repetition. Setting this to true adds storage
//        for an extra line, among other architectural changes. Useful for
//        applications like image interpolation and resizing. Default value
//        is false.
//
//  ac_window_v2_lflush_2d methods:
//    - void run (ac_array<PIX_TYPE, AC_BUS_WORDS> din, W_TYPE width, H_TYPE height, ac_array<PIX_TYPE, AC_WIN_HEIGHT, AC_WORDS> &dout, bool &sof_out, bool &eof_out, bool &sol_out, bool &eol_out, bool &vld_out, bool &read_data, bool repeat_line = false):
//        Class interface run function.
//        din is the pixel input(s).
//        width and height and the frame width and height, respectively.
//        dout is the windowed output.
//        *_out flags are the output flags.
//        read_data helps gate input reads.
//        repeat_line tells the window when to repeat over the same set of
//        lines. It is set to false by default (no line repetition) and can
//        only be set to true if AC_REPEAT = true.
//    - void reset_flags():
//        Resets various flags internal to the design.
//        If AC_REPEAT = true, it is called implicitly.
//        If AC_REPEAT = false, the user must explicitly call the function
//        before they send in a new frame for processing.
//    - void reset():
//        Resets all class members except for padded_val.
//    - ac_window_v2_lflush_2d():
//        Default constructor. Calls the reset() method and sets padded_val
//        to zero.
//    - ac_window_v2_lflush_2d(PIX_TYPE pval) : padded_val(pval):
//        Parameterized constructor. Calls the reset() method and sets
//        the padded_val data member to the pval input.
//
//****************************************************************************

#ifndef _INCLUDED_AC_WINDOW_V2_LFLUSH_H_
#define _INCLUDED_AC_WINDOW_V2_LFLUSH_H_

#include "ac_linebuffer.h"
#include "ac_flag_gen_lflush.h"
#include "ac_flag_shift.h"
#include "ac_shift_N.h"
#include "ac_boundary.h"
#include "ac_flag_gen_out_lflush.h"

template <class PIX_TYPE, int AC_IMG_HEIGHT, int AC_IMG_WIDTH, int AC_WIN_HEIGHT, int AC_WIN_WIDTH, ac_buff_type BUFF_TYPE, ac_padding_method AC_PMODE, int AC_BUS_WORDS = 1, bool HIGH_SPEED_REPLICATE = false, bool AC_REPEAT = false>
class ac_window_v2_lflush_2d
{
public:
  static_assert(AC_PMODE != AC_NO_PADDING, "Line flushing windowing only operates if boundary processing is enabled.");

  typedef ac_boundary_2d<PIX_TYPE, AC_WIN_HEIGHT, AC_WIN_WIDTH, AC_PMODE, AC_BUS_WORDS, HIGH_SPEED_REPLICATE> BOUND_2D_TYPE;
  typedef ac_int<ac::nbits<AC_IMG_WIDTH>::val, false> W_TYPE;
  typedef ac_int<ac::nbits<AC_IMG_HEIGHT>::val, false> H_TYPE;
  typedef ac_flag_shift_2d<AC_WIN_HEIGHT, AC_WIN_WIDTH, AC_PMODE, AC_BUS_WORDS, AC_REPEAT> FSHIFT_2D_TYPE;

  enum {
    AC_WORDS = BOUND_2D_TYPE::AC_WORDS,
    EXTRA_WORD = BOUND_2D_TYPE::EXTRA_WORD,
    LPAD_WORDS = BOUND_2D_TYPE::LPAD_WORDS, // For external use, if needed.
    RPAD_WORDS = BOUND_2D_TYPE::RPAD_WORDS, // For external use, if needed.
    TPAD_LINES = BOUND_2D_TYPE::TPAD_LINES, // For external use, if needed.
    BPAD_LINES = BOUND_2D_TYPE::BPAD_LINES,
    AC_SHIFT_HEIGHT = BOUND_2D_TYPE::AC_SHIFT_HEIGHT,
    BER_IT = BPAD_LINES + BOUND_2D_TYPE::EXTRA_LINE, // Iterations for bottom extension region (BER).
    RER_IT = FSHIFT_2D_TYPE::NUM_SHIFTS_FILL + FSHIFT_2D_TYPE::EXTRA_WORD - 1, // Iterations for right extension region (RER).
  };

  typedef ac_packed_vector<PIX_TYPE, AC_BUS_WORDS> LB_IN_TYPE;
  typedef ac_array<PIX_TYPE, AC_SHIFT_HEIGHT, AC_BUS_WORDS> LB_OUT_TYPE;
  typedef ac_linebuffer<LB_IN_TYPE, LB_OUT_TYPE, AC_IMG_WIDTH, AC_SHIFT_HEIGHT, BUFF_TYPE, AC_REPEAT> LB_TYPE;

  void run (
    ac_array<PIX_TYPE, AC_BUS_WORDS> din,
    W_TYPE width,
    H_TYPE height,
    ac_array<PIX_TYPE, AC_WIN_HEIGHT, AC_WORDS> &dout,
    bool &sof_out,
    bool &eof_out,
    bool &sol_out,
    bool &eol_out,
    bool &vld_out,
    bool &read_data,
    bool repeat_line = false
  ) {
    AC_ASSERT(AC_REPEAT || !repeat_line, "repeat_line cannot be set to true if AC_REPEAT == false.");
    
    // Call input flag generator.
    bool sof, eof, sol, eol;
    fgen_2d.run(width, height, window_hstate, window_vstate, sof, eof, sol, eol, repeat_line);

    // Call flag shifter.
    ac_int<AC_WIN_HEIGHT, false> sof_flags, eof_flags;
    ac_int<AC_WORDS, false> sol_flags, eol_flags;
    fshift_2d.run(sof, eof, sol, eol, sof_flags, eof_flags, sol_flags, eol_flags, repeat_line);
    
    #ifndef __SYNTHESIS__
    // CCOV_EXCL_START
    if (BUFF_TYPE != AC_DUAL) {
      AC_ASSERT(width%(2*AC_BUS_WORDS) == 0, "Image width must be divisible by 2*AC_BUS_WORDS if using singleport linebuffers.");
    }
    // CCOV_EXCL_STOP
    #endif
    
    // Call linebuffer.
    if (sol) {
      linebuf_addr = 0;
    }
    
    LB_OUT_TYPE linebuf_out;
    #pragma hls_unroll yes
    INIT_LB_OUT_TO_ZERO_I: for (int i = 0; i < AC_SHIFT_HEIGHT; i++) {
      #pragma hls_unroll yes
      INIT_LB_OUT_TO_ZERO_J: for (int j = 0; j < AC_BUS_WORDS; j++) {
        linebuf_out[i][j] = PIX_TYPE(0);
      }
    }
    
    bool dont_repeat = !(AC_REPEAT && repeat_line);
    
    // CCOV_EXCL_START
    if (window_hstate != AC_WINDOW_ER_LF) {
      LB_IN_TYPE linebuf_in;
      linebuf_in.pack_data(din);
      linebuf_2d.run(dont_repeat, linebuf_in, linebuf_addr, linebuf_out);
      linebuf_addr++;
    }
    // CCOV_EXCL_STOP

    // Call data shifter.
    ac_array<PIX_TYPE, AC_SHIFT_HEIGHT, AC_WORDS + EXTRA_WORD> dshift_out;
    dshift_2d.rshift(linebuf_out, dshift_out);
    
    // The flag shifter library is meant to work with Standard Windowing and doesn't have inbuilt
    // support for line repetition with right extension regions in the Line Flushing design.
    // As a result, the eof_flags array may be shifted prematurely during line repetition. To cancel
    // out this premature shift, we left-shift the eof_flags array by 1 if repeating lines after a
    // right extension region, if present.
    eof_flags <<= int(!dont_repeat && RER_IT != 0);

    // Call boundary processing.
    BOUND_2D_TYPE bound_2d(padded_val);
    bound_2d.run(dshift_out, sof_flags, eof_flags, sol_flags, eol_flags, dout);

    // Call output flag generator.
    ac_flag_gen_out_lflush_2d<AC_WIN_HEIGHT, AC_WIN_WIDTH, AC_BUS_WORDS> fgen_out_2d;
    bool sof_out_temp, eof_out_temp, sol_out_temp, eol_out_temp;
    fgen_out_2d.run(sof_flags, eof_flags, sol_flags, eol_flags, window_hstate, window_vstate, sof_out_temp, eof_out_temp, sol_out_temp, eol_out_temp, vld_out);
    
    // Call horizontal FSM.
    constexpr bool RER_EXPECTED = (RER_IT != 0);
    fsm<RER_EXPECTED, true>::run(sol_out_temp, eol, eol_out_temp, window_hstate);
    // Call vertical FSM.
    constexpr bool VLD_END_EXPECTED = !AC_REPEAT;
    bool vld_end_vertical = VLD_END_EXPECTED && eof_out_temp;
    fsm<true, VLD_END_EXPECTED>::run(sof_out_temp, eof, vld_end_vertical, window_vstate);
    
    // If output EOF was encountered and AC_REPEAT == false, we reset various AC Window 2.0 flags to
    // prepare for the next frame.
    // However, if AC_REPEAT == true, we're probably resizing the image and the output EOF might not be
    // a reliable indicator of when to reset these flags. The user should instead explicitly call the
    // reset_flags() function before they send in a frame for windowing.
    
    // CCOV_EXCL_START
    #pragma hls_waive CNS
    if (vld_end_vertical) {
      reset_flags();
    }
    // CCOV_EXCL_STOP
    
    #pragma hls_waive CCC
    read_data = (window_hstate != AC_WINDOW_ER_LF && window_vstate != AC_WINDOW_ER_LF);
    
    sof_out = sof_out_temp;
    eof_out = eof_out_temp;
    sol_out = sol_out_temp;
    eol_out = eol_out_temp;
  }

  void reset_flags() {
    fgen_2d.reset();
    fshift_2d.reset();
    // CCOV_EXCL_START
    #pragma hls_waive CNS
    if (AC_REPEAT) {
      window_vstate = AC_WINDOW_RAMP_LF;
    }
    // CCOV_EXCL_STOP
  }

  void reset() {
    fgen_2d.reset();
    fshift_2d.reset();
    dshift_2d.reset();
    window_hstate = AC_WINDOW_RAMP_LF;
    window_vstate = AC_WINDOW_RAMP_LF;
    linebuf_addr = 0;
  }

  ac_window_v2_lflush_2d() : padded_val(0) {
    fgen_2d.reset();
    fshift_2d.reset();
    linebuf_addr.template set_val<AC_VAL_DC>();
    window_hstate = AC_WINDOW_RAMP_LF;
    window_vstate = AC_WINDOW_RAMP_LF;
  }

  ac_window_v2_lflush_2d(PIX_TYPE pval) : padded_val(pval) {
    fgen_2d.reset();
    fshift_2d.reset();
    linebuf_addr.template set_val<AC_VAL_DC>();
    window_hstate = AC_WINDOW_RAMP_LF;
    window_vstate = AC_WINDOW_RAMP_LF;
  }

private:
  // Both extension region and vld_end = true are expected.
  template <bool ER_EXPECTED, bool VLD_END_EXPECTED>
  struct fsm {
    static void run(bool vld_start, bool image_end, bool vld_end, ac_window_v2_lflush_state &window_state) {
      switch (window_state) {
        case AC_WINDOW_RAMP_LF:
          if (vld_start) {
            window_state = AC_WINDOW_BETWEEN_LF;
          }
          break;
        case AC_WINDOW_BETWEEN_LF:
          if (vld_end) {
            window_state = AC_WINDOW_RAMP_LF;
          } else if (image_end) {
            window_state = AC_WINDOW_ER_LF;
          }
          break;
        default: // AC_WINDOW_ER_LF
          if (vld_end) {
            window_state = AC_WINDOW_RAMP_LF;
          }
          break;
      }
    }
  };

  // Extension region is not expected, i.e. ER_EXPECTED = false.
  template <bool VLD_END_EXPECTED>
  struct fsm<false, VLD_END_EXPECTED> {
    static void run(bool vld_start, bool image_end, bool vld_end, ac_window_v2_lflush_state &window_state) {
      // The absence of an extension region also implies an absence of an initial rampup period.
      // Hence, the window's state is always AC_WINDOW_BETWEEN_LF.
      window_state = AC_WINDOW_BETWEEN_LF;
    }
  };

  // vld_end = true is not expected, i.e. VLD_END_EXPECTED = false.
  template <bool ER_EXPECTED>
  struct fsm<ER_EXPECTED, false> {
    static void run(bool vld_start, bool image_end, bool vld_end, ac_window_v2_lflush_state &window_state) {
      switch (window_state) {
        case AC_WINDOW_RAMP_LF:
          if (vld_start) {
            window_state = AC_WINDOW_BETWEEN_LF;
          }
          break;
        case AC_WINDOW_BETWEEN_LF:
          if (image_end) {
            window_state = AC_WINDOW_ER_LF;
          }
          break;
        default: // AC_WINDOW_ER_LF
          // Do nothing in extension region.
          break;
      }
    }
  };

  ac_flag_gen_lflush_2d<AC_IMG_WIDTH, AC_IMG_HEIGHT, AC_BUS_WORDS, AC_REPEAT> fgen_2d;
  LB_TYPE linebuf_2d;
  FSHIFT_2D_TYPE fshift_2d;
  ac_shift_N_2d<PIX_TYPE, AC_WIN_HEIGHT, AC_WIN_WIDTH, AC_PMODE, AC_BUS_WORDS> dshift_2d;

  ac_window_v2_lflush_state window_hstate, window_vstate;
  typename LB_TYPE::addrType linebuf_addr;
  PIX_TYPE padded_val;
};

#endif
