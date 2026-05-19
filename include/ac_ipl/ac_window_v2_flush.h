/**************************************************************************
 *                                                                        *
 *  Algorithmic C (tm) Image Processing Library                           *
 *                                                                        *
 *  Software Version: 2026.2                                              *
 *                                                                        *
 *  Release Date    : Thu May 14 10:31:24 PDT 2026                        *
 *  Release Type    : Production Release                                  *
 *  Release Build   : 2026.2.0                                            *
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
//  File : ac_window_v2_flush.h
//
//  Description:
//    Top-level AC Window 2.0 design for flushing support. Integrates all the
//    sub-blocks to carry out windowing. For now, only 2D windows are
//    supported, via the ac_window_v2_flush_2d class.
//
//    ac_window_v2_flush_2d template parameters:
//    - PIX_TYPE: Pixel type.
//    - AC_IMG_HEIGHT: Maximum possible image height.
//    - AC_IMG_WIDTH: Maximum possible image width.
//    - AC_WIN_HEIGHT: Window height.
//    - AC_WIN_WIDTH: Window width.
//    - AC_BUFF_ARCH: Buffer architecture.
//    - AC_PMODE: Padding method/mode.
//    - AC_BUS_WORDS: Number of pixels passed on bus in parallel. Default
//        value is 1.
//    - HIGH_SPEED_REPLICATE: Template parameter used by boundary processing
//        class to switch between AC_REPLICATE architectures. Default value
//        is false.
//    - AC_REPEAT: Template parameter used to enable architecture for line
//        repetition. For now, this feature is not supported and this value
//        should be set to false, which it is by default.
//
//    ac_window_v2_flush_2d methods:
//    - void run (
//        ac_array<PIX_TYPE, AC_BUS_WORDS> din,
//        bool write,
//        W_TYPE width,
//        H_TYPE height,
//        ac_array<PIX_TYPE, AC_WIN_HEIGHT, AC_WORDS> &dout,
//        bool &sof_out,
//        bool &eof_out,
//        bool &sol_out,
//        bool &eol_out,
//        bool &vld_out,
//        bool &dont_read_data,
//        bool repeat_line = false
//      ):
//        Class interface run function. din is the pixel input(s), write
///       controls writes to the internal window storage, width and
//        height are the image width and height, respectively, dout is the
//        windowed output, *_out outputs are the output flags.
//        next_inside_image is an output flag which can be used to gate
//        against input reads while the window is flushing.
//        repeat_flag tells the window when to repeat over the same set of
//        lines. For now, this feature is not supported and repeat_line must
//        be false.
//    - void switch_frames():
//        Switches between the buffers used for primary and secondary frames
//        following a ping-pong scheme. Called by the design when it's
//        transitioning out of flushing.
//    - void reset():
//        Calls reset() method of all the constituent blocks except for the
//        linebuffer and boundary processor.
//    - ac_window_v2_flush_2d():
//        Default constructor. Calls the reset() method and sets the
//        padded_val data member to 0.
//    - ac_window_v2_flush_2d(PIX_TYPE pval) : padded_val(pval):
//        Parameterized constructor. Calls the reset() method and sets the
//        padded_val data member to the pval input.
//
//****************************************************************************

#ifndef _INCLUDED_AC_WINDOW_V2_FLUSH_H_
#define _INCLUDED_AC_WINDOW_V2_FLUSH_H_

#include "ac_linebuffer_flush.h"
#include "ac_flag_gen_flush.h"
#include "ac_flag_shift_flush.h"
#include "ac_shift_N.h"
#include "ac_boundary.h"
#include "ac_flag_gen_out_flush.h"

template <class PIX_TYPE, int AC_IMG_HEIGHT, int AC_IMG_WIDTH, int AC_WIN_HEIGHT, int AC_WIN_WIDTH, ac_buff_arch_flush AC_BUFF_ARCH, ac_padding_method AC_PMODE, int AC_BUS_WORDS = 1, bool HIGH_SPEED_REPLICATE = false, bool AC_REPEAT = false>
class ac_window_v2_flush_2d
{
public:
  static_assert(AC_PMODE != AC_NO_PADDING, "Flush support windowing only operates if boundary processing is enabled.");
  static_assert(AC_WIN_WIDTH > 0, "AC_WIN_WIDTH must be positive.");
  static_assert(AC_WIN_HEIGHT > 0, "AC_WIN_HEIGHT must be positive.");
  #ifdef AC_WIN_V2_REVERSE_PADDING
  static_assert(AC_WIN_HEIGHT >= 2, "AC_WIN_HEIGHT must not be less than 2.");
  #else
  static_assert(AC_WIN_HEIGHT >= 3, "AC_WIN_HEIGHT must not be less than 3.");
  #endif

  typedef ac_shift_N_2d<PIX_TYPE, AC_WIN_HEIGHT, AC_WIN_WIDTH, AC_PMODE, AC_BUS_WORDS> DSHIFT_TYPE;
  typedef ac_boundary_2d<PIX_TYPE, AC_WIN_HEIGHT, AC_WIN_WIDTH, AC_PMODE, AC_BUS_WORDS, HIGH_SPEED_REPLICATE> BOUND_TYPE;
  typedef ac_int<ac::nbits<AC_IMG_WIDTH>::val, false> W_TYPE;
  typedef ac_int<ac::nbits<AC_IMG_HEIGHT>::val, false> H_TYPE;

  enum {
    AC_WORDS = BOUND_TYPE::AC_WORDS,
    EXTRA_WORD = BOUND_TYPE::EXTRA_WORD,
    EXTRA_LINE = BOUND_TYPE::EXTRA_LINE, // For external use, if needed.
    AC_SHIFT_HEIGHT = BOUND_TYPE::AC_SHIFT_HEIGHT,
    LPAD_WORDS = BOUND_TYPE::LPAD_WORDS, // For external use, if needed.
    RPAD_WORDS = BOUND_TYPE::RPAD_WORDS, // For external use, if needed.
    TPAD_LINES = BOUND_TYPE::TPAD_LINES,
    BPAD_LINES = BOUND_TYPE::BPAD_LINES, // For external use, if needed.
    NUM_SHIFTS_FILL = DSHIFT_TYPE::NUM_SHIFTS_FILL, // For external use, if needed.
    PACKED_WIDTH = AC_IMG_WIDTH/AC_BUS_WORDS, // For external use, if needed.
    NUM_FLUSH_ITERS = (TPAD_LINES + EXTRA_LINE)*PACKED_WIDTH + (NUM_SHIFTS_FILL - 1) + EXTRA_WORD // For external use, if needed.
  };

  typedef ac_packed_vector<PIX_TYPE, AC_BUS_WORDS> LB_IN_TYPE;
  typedef ac_array<PIX_TYPE, AC_SHIFT_HEIGHT, AC_BUS_WORDS> LB_OUT_TYPE;
  typedef ac_linebuffer_flush<LB_IN_TYPE, LB_OUT_TYPE, AC_IMG_WIDTH, AC_SHIFT_HEIGHT, AC_BUFF_ARCH, AC_REPEAT> LB_TYPE;

  void run (
    ac_array<PIX_TYPE, AC_BUS_WORDS> din,
    bool write,
    W_TYPE width,
    H_TYPE height,
    ac_array<PIX_TYPE, AC_WIN_HEIGHT, AC_WORDS> &dout,
    bool &sof_out,
    bool &eof_out,
    bool &sol_out,
    bool &eol_out,
    bool &vld_out,
    bool &dont_read_data,
    bool repeat_line = false
  ) {
    AC_ASSERT(AC_REPEAT || !repeat_line, "repeat_line cannot be set to true if AC_REPEAT == false.");

    // Declare "ping" and "pong" input flags.
    bool sof_ping = false, eof_ping = false, sol_ping = false, eol_ping = false, sof_pong = false, eof_pong = false, sol_pong = false, eol_pong = false;
    bool only_update_line = (AC_REPEAT && repeat_line);

    // Declare ping and pong flag shift registers.
    ac_int<AC_WIN_HEIGHT, false> sof_flags_ping = 0, eof_flags_ping = 0, sof_flags_pong = 0, eof_flags_pong = 0;
    ac_int<AC_WORDS, false> sol_flags_ping = 0, eol_flags_ping = 0, sol_flags_pong = 0, eol_flags_pong = 0;

    // Write data only when write flag is true and we don't have to stall to fix any linebuffer misalignment.
    bool write_ = write && !dont_read_data_;
    bool is_window_flushing = (window_state == AC_WINDOW_FLUSH || window_state == AC_WINDOW_FLUSH_LB_ADVANCE);

    if (write_ || is_window_flushing || only_update_line) {
      // Call input flag generator.
      fgen.run(width, height, window_state, ping, write_,
               sof_ping, eof_ping, sol_ping, eol_ping, sof_pong, eof_pong, sol_pong, eol_pong, only_update_line);

      // Call flag shifter.
      fshift.run(sof_ping, eof_ping, sol_ping, eol_ping, sof_pong, eof_pong, sol_pong, eol_pong, window_state, ping, write_,
                 sof_flags_ping, eof_flags_ping, sol_flags_ping, eol_flags_ping, sof_flags_pong, eof_flags_pong, sol_flags_pong, eol_flags_pong, repeat_line);
    }

    #ifndef __SYNTHESIS__
    // CCOV_EXCL_START
    if (AC_BUFF_ARCH != AC_1R1W_FLUSH) {
      AC_ASSERT(width%(2*AC_BUS_WORDS) == 0, "Image width must be divisible by 2*AC_BUS_WORDS if using singleport linebuffers.");
    }
    // CCOV_EXCL_STOP
    #endif

    bool sol_primary =  ping ? sol_ping : sol_pong;
    bool sol_secondary = !ping ? sol_ping : sol_pong;

    // Could the read and write addresses for the linebuffers be different?
    bool rw_addr_diff = (dont_read_data_ || window_state == AC_WINDOW_FLUSH);
    // Are the read and write addresses for the linebuffers always the same?
    bool rw_addr_same = !rw_addr_diff && write;
    // Are we calling the run() function of the linebuffer?
    bool call_linebuf = (rw_addr_diff || write);

    // sol_lb_in corresponds to the valid pixel data received by the linebuffer.
    bool sol_lb_in = (rw_addr_diff || window_state == AC_WINDOW_FLUSH_LB_ADVANCE) ? sol_secondary : sol_primary;

    if (rw_addr_diff && sol_primary) {
      linebuf_read_addr = 0;
    }

    if (call_linebuf && sol_lb_in) {
      linebuf_write_addr = 0; // Write address always corresponds to the pixel input.
    }

    if (rw_addr_same) {
      // If the read and write addresses are always supposed to be the same the circular buffer selection
      // also needs to be the same. Call the align_buf_cnts method to make sure that's the case.
      linebuf.align_buf_cnts();
      linebuf_read_addr = linebuf_write_addr;
    }

    LB_IN_TYPE linebuf_in;
    linebuf_in.pack_data(din); // ac_array data is packed into a packed_vector.
    LB_OUT_TYPE linebuf_out;
    #pragma hls_unroll yes
    INITIALIZE_LB_OUT_I: for (int i = 0; i < AC_SHIFT_HEIGHT; i++) {
      #pragma hls_unroll yes
      INITIALIZE_LB_OUT_J: for (int j = 0; j < AC_BUS_WORDS; j++) {
        linebuf_out[i][j] = PIX_TYPE(0);
      }
    }

    if (call_linebuf) {
      linebuf.run(linebuf_in, linebuf_write_addr, linebuf_read_addr, write_, linebuf_out, repeat_line);
      linebuf_write_addr += int(write_);
    }

    if (rw_addr_diff) {
      // If the read and write addresses might be different, that means that we're flushing and that there
      // will always be data to read from the linebuffer, even if there is none to write to it. Always increment
      // the read address in such a case, independent of the write address.
      linebuf_read_addr++;
    }

    ac_array<PIX_TYPE, AC_SHIFT_HEIGHT, AC_WORDS + EXTRA_WORD> dshift_out;
    #pragma hls_unroll yes
    INITIALIZE_DSHIFT_OUT_I: for (int i = 0; i < AC_SHIFT_HEIGHT; i++) {
      #pragma hls_unroll yes
      INITIALIZE_DSHIFT_OUT_J: for (int j = 0; j < AC_WORDS + EXTRA_WORD; j++) { dshift_out[i][j] = PIX_TYPE(0); }
    }

    if (write_ || is_window_flushing) {
      dshift.rshift(linebuf_out, dshift_out); // Data is only shifted when there are outputs to be read.
    }

    // Prepare input flags for boundary shifter and output flag generator.
    ac_int<AC_WIN_HEIGHT, false> sof_flags_primary = ping ? sof_flags_ping : sof_flags_pong;
    ac_int<AC_WIN_HEIGHT, false> eof_flags_primary = ping ? eof_flags_ping : eof_flags_pong;
    ac_int<AC_WORDS, false> sol_flags_primary = ping ? sol_flags_ping : sol_flags_pong;
    ac_int<AC_WORDS, false> eol_flags_primary = ping ? eol_flags_ping : eol_flags_pong;
    BOUND_TYPE bound(padded_val);
    bound.run(dshift_out, sof_flags_primary, eof_flags_primary, sol_flags_primary, eol_flags_primary, dout);

    ac_flag_gen_out_flush_2d<AC_WIN_HEIGHT, AC_WIN_WIDTH, AC_BUS_WORDS> fgen_out;
    bool sof_out_temp, eof_out_temp;
    fgen_out.run(sof_flags_primary, eof_flags_primary, sol_flags_primary, eol_flags_primary, window_state, write_, sof_out_temp, eof_out_temp, sol_out, eol_out, vld_out);

    bool eof_primary = ping ? eof_ping : eof_pong;

    dont_read_data_ = false;

    ac_int<AC_WIN_HEIGHT, false> sof_flags_secondary = !ping ? sof_flags_ping : sof_flags_pong;
    bool eol_secondary = !ping ? eol_ping : eol_pong;

    constexpr bool DUMMY = false;
    fsm<DUMMY, AC_REPEAT>::run(sof_out_temp, eof_primary, eof_out_temp, eol_secondary, only_update_line, sof_flags_secondary, *this);

    dont_read_data = dont_read_data_;

    sof_out = sof_out_temp;
    eof_out = eof_out_temp;
  }

  bool switch_frames() {
    window_state = AC_WINDOW_IDLE;
    fgen.reset_primary_line_cnt(ping);
    fshift.reset_primary_line_regs(ping);
    ping = !ping;
    dont_read_data_ = linebuf.misaligned();
    return dont_read_data_; // Need this value for external use if AC_REPEAT = true.
  }

  void reset() {
    fgen.reset();
    fshift.reset();
    dshift.reset();
    window_state = AC_WINDOW_IDLE;
    linebuf_read_addr = linebuf_write_addr = 0;
    ping = true;
    dont_read_data_ = false;
  }

  ac_window_v2_flush_2d() : padded_val(0) {
    reset();
  }

  ac_window_v2_flush_2d(PIX_TYPE pval) : padded_val(pval) {
    reset();
  }

private:
  // AC_REPEAT_ = true.
  template <bool DUMMY, bool AC_REPEAT_>
  struct fsm {
    #pragma CCOV_EXCL yes
    static void run(
      const bool sof_out_temp, const bool eof_primary, const bool eof_out_temp, const bool eol_secondary,
      const bool only_update_line,
      const ac_int<AC_WIN_HEIGHT, false> sof_flags_secondary,
      ac_window_v2_flush_2d &win_obj
    ) {
      switch (win_obj.window_state) {
        case AC_WINDOW_IDLE:
          if (sof_out_temp) {
            win_obj.window_state = AC_WINDOW_ACTIVE;
          }
          break;
        case AC_WINDOW_ACTIVE:
          if (eof_primary) {
            win_obj.window_state = AC_WINDOW_FLUSH;
          }
          break;
        case AC_WINDOW_FLUSH: {
          bool move_to_lb_advance = !eof_out_temp && sof_flags_secondary[TPAD_LINES + 1] && eol_secondary && !only_update_line;
          if (move_to_lb_advance) {
            win_obj.window_state = AC_WINDOW_FLUSH_LB_ADVANCE;
            win_obj.dont_read_data_ = win_obj.linebuf.misaligned();
          }
          break;
        }
        default: // AC_WINDOW_FLUSH_LB_ADVANCE
          // Do nothing, user must call switch_frames().
          break;
      }
    }
  };

  // AC_REPEAT_ = false.
  template <bool DUMMY>
  struct fsm<DUMMY, false> {
    #pragma CCOV_EXCL yes
    static void run(
      const bool sof_out_temp, const bool eof_primary, const bool eof_out_temp, const bool eol_secondary,
      const bool only_update_line,
      const ac_int<AC_WIN_HEIGHT, false> sof_flags_secondary,
      ac_window_v2_flush_2d &win_obj
    ) {
      switch (win_obj.window_state) {
        case AC_WINDOW_IDLE:
          if (sof_out_temp) {
            win_obj.window_state = AC_WINDOW_ACTIVE;
          }
          break;
        case AC_WINDOW_ACTIVE:
          if (eof_primary) {
            win_obj.window_state = AC_WINDOW_FLUSH;
          }
          break;
        case AC_WINDOW_FLUSH:
          if (eof_out_temp) {
            win_obj.switch_frames();
          } else if (sof_flags_secondary[TPAD_LINES + 1] && eol_secondary) {
            win_obj.window_state = AC_WINDOW_FLUSH_LB_ADVANCE;
            win_obj.dont_read_data_ = win_obj.linebuf.misaligned();
          }
          break;
        default: // AC_WINDOW_FLUSH_LB_ADVANCE
          if (eof_out_temp) {
            win_obj.switch_frames();
          }
          break;
      }
    }
  };

  ac_flag_gen_flush_2d<AC_IMG_WIDTH, AC_IMG_HEIGHT, AC_BUS_WORDS> fgen;
  LB_TYPE linebuf;
  ac_flag_shift_flush_2d<AC_WIN_HEIGHT, AC_WIN_WIDTH, AC_PMODE, AC_BUS_WORDS, AC_REPEAT> fshift;
  DSHIFT_TYPE dshift;

  ac_window_v2_flush_state window_state;
  bool ping, dont_read_data_;
  typename LB_TYPE::ADDR_TYPE linebuf_read_addr, linebuf_write_addr;
  PIX_TYPE padded_val;
};

#endif
