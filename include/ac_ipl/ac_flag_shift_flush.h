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
//******************************************************************************
//  File : ac_flag_shift_flush.h
//
//  Description:
//    Flag shifters for the AC Window 2.0 Flushing Support design.
//    ac_flag_shift_flush_1d handles shifting in one dimension such that the
//    shifted flags align with the outputs from the data shifters.
//    ac_flag_shift_flush_2d instantiates an ac_flag_shift_flush_1d object
//    to serve as the horizontal flag shifter. It also uses the output from
//    the horizontal shifter to vertically shift the SOF and EOF flag arrays
//    and align those with the 2D data shifter output.
//
//    ac_flag_shift_flush_1d template parameters:
//    - AC_WIN_WIDTH: Window width.
//    - AC_PMODE: Padding mode/method.
//    - AC_BUS_WORDS: Number of pixels passed on bus in parallel.
//        Default value = 1.
//
//    ac_flag_shift_flush_1d methods:
//    - void run(bool sol_ping, bool eol_ping, bool sol_pong, bool eol_pong, ac_window_v2_flush_state window_state, bool ping, bool flush_write_enabled, ac_int<AC_WORDS, false> &sol_flags_ping, ac_int<AC_WORDS, false> &eol_flags_ping, ac_int<AC_WORDS, false> &sol_flags_pong, ac_int<AC_WORDS, false> &eol_flags_pong):
//        Class interface run function.
//        sol_ping/pong and eol_ping/pong are all outputs from the flag generator.
//        window_state tells us which state the window is in.
//        ping is the flag used to switch between ping and pong buffers.
//        flush_write_enabled signals when the design processes inputs in
//        the flushing states.
//        sol_flags_ping/pong and eol_flags_ping/pong are the 1D flag shift
//        registers, produced in pairs according to the ping-pong scheme.
//    - void reset_primary_regs(bool ping):
//        Resets flag registers associated with primary frame. Must be called
//        at the start of the idle state.
//    - void reset():
//        Resets all internal registers of the class.
//    - ac_flag_shift_flush_1d():
//        Default constructor. Calls the reset() function.
//
//    ac_flag_shift_flush_2d template parameters:
//    - AC_WIN_HEIGHT: Window height.
//    - AC_WIN_WIDTH: Window width.
//    - AC_PMODE: Padding mode/method.
//    - AC_BUS_WORDS: Number of pixels passed on bus in parallel.
//        Default value = 1.
//    - AC_REPEAT: Template parameter used to enable architecture for line
//        repetition. For now, this feature is not supported and this value
//        should be set to false, which it is by default.
//
//    ac_flag_shift_flush_2d template methods:
//    - void run(bool sof_ping, bool eof_ping, bool sol_ping, bool eol_ping, bool sof_pong, bool eof_pong, bool sol_pong, bool eol_pong, ac_window_v2_flush_state window_state, bool ping, bool flush_write_enabled, ac_int<AC_WIN_HEIGHT, false> &sof_flags_ping, ac_int<AC_WIN_HEIGHT, false> &eof_flags_ping, ac_int<AC_WORDS, false> &sol_flags_ping, ac_int<AC_WORDS, false> &eol_flags_ping, ac_int<AC_WIN_HEIGHT, false> &sof_flags_pong, ac_int<AC_WIN_HEIGHT, false> &eof_flags_pong, ac_int<AC_WORDS, false> &sol_flags_pong, ac_int<AC_WORDS, false> &eol_flags_pong, bool repeat_line = false):
//        Class interface run function.
//        sof_ping/pong, eof_ping/pong, sol_ping/pong and eol_ping/pong are all
//        outputs from the flag generator.
//        window_state tells us which state the window is in.
//        ping is the flag used to switch between ping and pong buffers.
//        flush_write_enabled signals when the design processes inputs in
//        the flushing states.
//        sof_flags_ping/pong, eof_flags_ping/pong, sol_flags_ping/pong and
//        eof_flags_ping/pong are the flag shift registers, produced in pairs
//        according to the ping-pong scheme.
//    - void reset_primary_line_regs(bool ping):
//        Calls the reset_primary_regs function of ac_flag_shift_flush_1d.
//        Must be called at the start of the idle state.
//    - void reset():
//        Resets all data members.
//    - ac_flag_shift_flush_2d():
//        Default constructor. Calls the reset() function.
//
//******************************************************************************

#ifndef _INCLUDED_AC_FLAG_SHIFT_FLUSH_H_
#define _INCLUDED_AC_FLAG_SHIFT_FLUSH_H_

#include <ac_int.h>
#include <ac_array.h>

#include "ac_boundary_enums.h"
#include "ac_window_v2_flush_enums.h"

// The code uses static assertions, which are only supported by C++11 or later compiler
// standards. Hence, the user should be informed if they are not using those standards.
#if (defined(__GNUC__) && (__cplusplus < 201103L))
#error Please use C++11 or a later standard for compilation.
#endif
#if (defined(_MSC_VER) && (_MSC_VER < 1920) && !defined(__EDG__))
#error Please use Microsoft VS 2019 or a later standard for compilation.
#endif

template<int AC_WIN_WIDTH, ac_padding_method AC_PMODE, int AC_BUS_WORDS = 1>
class ac_flag_shift_flush_1d
{
public:
  static_assert(AC_WIN_WIDTH > 0, "AC_WIN_WIDTH must be positive.");
  static_assert(AC_BUS_WORDS > 0, "AC_BUS_WORDS must be positive.");

  typedef calc_shifter_words<AC_WIN_WIDTH, AC_PMODE, AC_BUS_WORDS> cswords_type;

  enum {
    AC_WORDS = cswords_type::AC_WORDS,
    NUM_SHIFTS_FILL = cswords_type::NUM_SHIFTS_FILL,
    EXTRA_WORD = cswords_type::EXTRA_WORD,
    AC_SHIFT_WORDS = cswords_type::AC_SHIFT_WORDS
  };

  void run(
    bool sol_ping, bool eol_ping, bool sol_pong, bool eol_pong,
    ac_window_v2_flush_state window_state,
    bool ping, bool flush_write_enabled,
    ac_int<AC_WORDS, false> &sol_flags_ping, ac_int<AC_WORDS, false> &eol_flags_ping,
    ac_int<AC_WORDS, false> &sol_flags_pong, ac_int<AC_WORDS, false> &eol_flags_pong
  ) {
    bool is_window_flushing = (window_state == AC_WINDOW_FLUSH || window_state == AC_WINDOW_FLUSH_LB_ADVANCE);
    bool update_ping =  ping || (is_window_flushing && flush_write_enabled);
    bool update_pong = !ping || (is_window_flushing && flush_write_enabled);

    if (update_ping) {
      sol_regs_ping >>= AC_BUS_WORDS;
      eol_regs_ping >>= AC_BUS_WORDS;
      sol_regs_ping[AC_SHIFT_WORDS - AC_BUS_WORDS] = sol_ping;
      eol_regs_ping[AC_SHIFT_WORDS - 1] = eol_ping;

      sol_flags_ping = sol_regs_ping.template slc<AC_WORDS>(0);
      eol_flags_ping = eol_regs_ping.template slc<AC_WORDS>(0);
    }

    if (update_pong) {
      sol_regs_pong >>= AC_BUS_WORDS;
      eol_regs_pong >>= AC_BUS_WORDS;
      sol_regs_pong[AC_SHIFT_WORDS - AC_BUS_WORDS] = sol_pong;
      eol_regs_pong[AC_SHIFT_WORDS - 1] = eol_pong;

      sol_flags_pong = sol_regs_pong.template slc<AC_WORDS>(0);
      eol_flags_pong = eol_regs_pong.template slc<AC_WORDS>(0);
    }
  }

  void reset_primary_regs(bool ping) {
    if (ping) {
      sol_regs_ping = eol_regs_ping = 0;
    } else {
      sol_regs_pong = eol_regs_pong = 0;
    }
  }

  void reset() {
    sol_regs_ping = eol_regs_ping = sol_regs_pong = eol_regs_pong = 0;
  }

  ac_flag_shift_flush_1d() {
    reset();
  }

private:
  ac_int<AC_SHIFT_WORDS, false> sol_regs_ping, eol_regs_ping, sol_regs_pong, eol_regs_pong;
};

template <int AC_WIN_HEIGHT, int AC_WIN_WIDTH, ac_padding_method AC_PMODE, int AC_BUS_WORDS = 1, bool AC_REPEAT = false>
class ac_flag_shift_flush_2d
{
public:
  static_assert(AC_WIN_HEIGHT > 0, "AC_WIN_HEIGHT must be positive.");
  static_assert(AC_WIN_WIDTH > 0, "AC_WIN_WIDTH must be positive.");
  static_assert(AC_BUS_WORDS > 0, "AC_BUS_WORDS must be positive.");

  typedef ac_flag_shift_flush_1d<AC_WIN_WIDTH, AC_PMODE, AC_BUS_WORDS> LINE_TYPE;

  typedef calc_shifter_height<AC_WIN_HEIGHT, AC_PMODE> csheight_type;

  enum {
    EXTRA_LINE = csheight_type::EXTRA_LINE,
    AC_SHIFT_HEIGHT = csheight_type::AC_SHIFT_HEIGHT,
    AC_WORDS = LINE_TYPE::AC_WORDS,
    EXTRA_WORD = LINE_TYPE::EXTRA_WORD,
    NUM_SHIFTS_FILL = LINE_TYPE::NUM_SHIFTS_FILL,
    AC_SHIFT_WORDS = LINE_TYPE::AC_SHIFT_WORDS // For external use, if needed.
  };

  void run(
    bool sof_ping, bool eof_ping, bool sol_ping, bool eol_ping,
    bool sof_pong, bool eof_pong, bool sol_pong, bool eol_pong,
    ac_window_v2_flush_state window_state,
    bool ping, bool flush_write_enabled,
    ac_int<AC_WIN_HEIGHT, false> &sof_flags_ping, ac_int<AC_WIN_HEIGHT, false> &eof_flags_ping,
    ac_int<AC_WORDS, false> &sol_flags_ping, ac_int<AC_WORDS, false> &eol_flags_ping,
    ac_int<AC_WIN_HEIGHT, false> &sof_flags_pong, ac_int<AC_WIN_HEIGHT, false> &eof_flags_pong,
    ac_int<AC_WORDS, false> &sol_flags_pong, ac_int<AC_WORDS, false> &eol_flags_pong,
    bool repeat_line = false
  ) {
    AC_ASSERT(AC_REPEAT || !repeat_line, "repeat_line cannot be set to true if AC_REPEAT == false.");
    
    line.run(sol_ping, eol_ping, sol_pong, eol_pong, window_state, ping, flush_write_enabled, sol_flags_ping, eol_flags_ping, sol_flags_pong, eol_flags_pong);

    constexpr int LAST_COL_IDX = NUM_SHIFTS_FILL + EXTRA_WORD - 1;

    bool is_window_flushing = (window_state == AC_WINDOW_FLUSH || window_state == AC_WINDOW_FLUSH_LB_ADVANCE);
    bool update_ping =  ping || (is_window_flushing && flush_write_enabled);
    bool update_pong = !ping || (is_window_flushing && flush_write_enabled);

    bool dont_repeat = !(AC_REPEAT && repeat_line);

    if (update_ping) {
      // CCOV_EXCL_START
      #pragma hls_waive CCC
      #pragma hls_unroll yes
      FSHIFT_PING_2D_I: for (int j = 0; j < LAST_COL_IDX; j++) {
        #pragma hls_unroll yes
        FSHIFT_PING_2D_J: for (int i = 0; i < AC_SHIFT_HEIGHT; i++) {
          sof_regs_ping[i][j] = sof_regs_ping[i][j + 1];
          eof_regs_ping[i][j] = eof_regs_ping[i][j + 1];
        }
      }
      // CCOV_EXCL_STOP

      #pragma hls_unroll yes
      FSHIFT_PING_2D_LAST_COL: for (int i = 0; i < AC_SHIFT_HEIGHT - 1; i++) {
        if (sol_ping && dont_repeat) {
          sof_regs_ping[i][LAST_COL_IDX] = sof_regs_ping[i + 1][LAST_COL_IDX];
        }
        if (eol_old_ping && dont_repeat) {
          eof_regs_ping[i][LAST_COL_IDX] = eof_regs_ping[i + 1][LAST_COL_IDX];
        }
      }

      if (sol_ping && dont_repeat) {
        sof_regs_ping[AC_SHIFT_HEIGHT - 1][LAST_COL_IDX] = sof_ping;
      }

      if (eol_old_ping && dont_repeat) {
        eof_regs_ping[AC_SHIFT_HEIGHT - 1][LAST_COL_IDX] = eof_old_ping;
      }

      #pragma hls_unroll yes
      FSHIFT_PING_2D_COPY_TO_OUT: for (int i = 0; i < AC_WIN_HEIGHT; i++) {
        #pragma hls_waive UMR
        sof_flags_ping[i] = sof_regs_ping[i][0];
        #pragma hls_waive UMR
        eof_flags_ping[i] = eof_regs_ping[i][0];
      }

      eof_old_ping = (dont_repeat && eof_ping) || (!dont_repeat && eof_old_ping);
      eol_old_ping = eol_ping;
    }
    
    if (update_pong) {
      // CCOV_EXCL_START
      #pragma hls_waive CCC
      #pragma hls_unroll yes
      FSHIFT_PONG_2D_I: for (int j = 0; j < LAST_COL_IDX; j++) {
        #pragma hls_unroll yes
        FSHIFT_PONG_2D_J: for (int i = 0; i < AC_SHIFT_HEIGHT; i++) {
          sof_regs_pong[i][j] = sof_regs_pong[i][j + 1];
          eof_regs_pong[i][j] = eof_regs_pong[i][j + 1];
        }
      }
      // CCOV_EXCL_STOP

      #pragma hls_unroll yes
      FSHIFT_PONG_2D_LAST_COL: for (int i = 0; i < AC_SHIFT_HEIGHT - 1; i++) {
        if (sol_pong && dont_repeat) {
          sof_regs_pong[i][LAST_COL_IDX] = sof_regs_pong[i + 1][LAST_COL_IDX];
        }
        if (eol_old_pong && dont_repeat) {
          eof_regs_pong[i][LAST_COL_IDX] = eof_regs_pong[i + 1][LAST_COL_IDX];
        }
      }

      if (sol_pong && dont_repeat) {
        sof_regs_pong[AC_SHIFT_HEIGHT - 1][LAST_COL_IDX] = sof_pong;
      }

      if (eol_old_pong && dont_repeat) {
        eof_regs_pong[AC_SHIFT_HEIGHT - 1][LAST_COL_IDX] = eof_old_pong;
      }

      #pragma hls_unroll yes
      FSHIFT_PONG_2D_COPY_TO_OUT: for (int i = 0; i < AC_WIN_HEIGHT; i++) {
        #pragma hls_waive UMR
        sof_flags_pong[i] = sof_regs_pong[i][0];
        #pragma hls_waive UMR
        eof_flags_pong[i] = eof_regs_pong[i][0];
      }

      eof_old_pong = (dont_repeat && eof_pong) || (!dont_repeat && eof_old_pong);
      eol_old_pong = eol_pong;
    }
  }
  
  void reset_primary_line_regs(bool ping) {
    line.reset_primary_regs(ping);
  }

  void reset() {
    line.reset();

    #pragma hls_unroll yes
    RESET_FSHIFT_2D_I: for (int i = 0; i < AC_SHIFT_HEIGHT; i++) {
      #pragma hls_unroll yes
      RESET_FSHIFT_2D_J: for (int j = 0; j < NUM_SHIFTS_FILL + EXTRA_WORD; j++) {
        sof_regs_ping[i][j] = eof_regs_ping[i][j] = sof_regs_pong[i][j] = eof_regs_pong[i][j] = false;
      }
    }

    eof_old_ping = eol_old_ping = eof_old_pong = eol_old_pong = false;
  }

  ac_flag_shift_flush_2d() {
    reset();
  }

private:
  LINE_TYPE line;
  ac_array<bool, AC_SHIFT_HEIGHT, NUM_SHIFTS_FILL + EXTRA_WORD> sof_regs_ping, eof_regs_ping, sof_regs_pong, eof_regs_pong;
  bool eof_old_ping, eol_old_ping, eof_old_pong, eol_old_pong;
};

#endif
