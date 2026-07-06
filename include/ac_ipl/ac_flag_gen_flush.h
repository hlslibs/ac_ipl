/**************************************************************************
 *                                                                        *
 *  Algorithmic C (tm) Image Processing Library                           *
 *                                                                        *
 *  Software Version: 2026.2                                              *
 *                                                                        *
 *  Release Date    : Tue Jun 30 15:08:22 PDT 2026                        *
 *  Release Type    : Production Release                                  *
 *  Release Build   : 2026.2.1                                            *
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
//  File : ac_flag_gen_flush.h
//
//  Description:
//    Synchronization flag generator for the for the AC Window 2.0 Flushing
//    Support design. ac_flag_gen_flush_1d handles flag generation for 1D lines
//    of pixels and produces sol and eol flags, while ac_flag_gen_flush_2d
//    handles flag generation for 2D frames. ac_flag_gen_flush_2d is built on
//    top of ac_flag_gen_flush_1d, with the latter computing sol and eol flags
//    while the former computes sof and eof flags on top of that.
//
//    ac_flag_gen_flush_1d template parameters:
//    - AC_WIDTH: Maximum line width.
//    - AC_BUS_WORDS: Number of pixels passed on bus in parallel. Default
//        value is 1.
//
//    ac_flag_gen_flush_1d methods:
//    - void run (W_TYPE width, ac_window_v2_flush_state window_state, bool ping, bool flush_write_enabled, bool &sol_ping, bool &eol_ping, bool &sol_pong, bool &eol_pong):
//        Class interface run function.
//        width is the line width.
//        window_state tells us which state the window is in.
//        ping is the flag used to switch between ping and pong buffers.
//        flush_write_enabled signals when the design processes inputs in
//        the flushing states.
//        sol_ping, eol_ping, sol_pong and eol_pong are the output flags.
//    - void reset_primary_cnt(bool ping): Resets the counter associated with
//        the primary frame to zero. Must be called at start of the idle state.
//    - void reset():
//        Resets counters to zero.
//    - ac_flag_gen_flush_1d():
//        Default constructor. Calls the reset() function.
//
//    ac_flag_gen_flush_2d template parameters:
//    - AC_WIDTH: Maximum frame width.
//    - AC_HEIGHT: Maximum frame height.
//    - AC_BUS_WORDS: Number of pixels passed on bus in parallel. Default
//        value is 1.
//
//    ac_flag_gen_flush_2d methods:
//    - void run (W_TYPE width, H_TYPE height, ac_window_v2_flush_state window_state, bool ping, bool flush_write_enabled, bool &sof_ping, bool &eof_ping, bool &sol_ping, bool &eol_ping, bool &sof_pong, bool &eof_pong, bool &sol_pong, bool &eol_pong, bool only_update_line = false)
//        Class interface run function.
//        width and height are the frame width and height, respectively.
//        window_state tells us which state the window is in.
//        ping is the flag used to switch between ping and pong buffers.
//        flush_write_enabled signals when the design processes inputs in
//        the flushing states.
//        The sof_*, eof_*, sol_* and eol_* flags are the outputs of this
//        library, produced in pairs in accordance with the ping-pong scheme.
//    - void reset_primary_line_cnt(bool ping):
//        Calls the reset_primary_cnt method of the 1D flag generator. Must be
//        called at the start of the idle state.
//    - bool dim_change_error():
//        Returns true if the frame dimensions are changed incorrectly.
//    - void reset():
//        Resets all data members.
//    - ac_flag_gen_flush_2d():
//        Default constructor. Calls the reset() function.
//        
//******************************************************************************


#ifndef _INCLUDED_AC_FLAG_GEN_FLUSH_H_
#define _INCLUDED_AC_FLAG_GEN_FLUSH_H_

#include <ac_int.h>
#include "ac_window_v2_flush_enums.h"

// The code uses static assertions, which are only supported by C++11 or later compiler
// standards. Hence, the user should be informed if they are not using those standards.
#if (defined(__GNUC__) && (__cplusplus < 201103L))
#error Please use C++11 or a later standard for compilation.
#endif
#if (defined(_MSC_VER) && (_MSC_VER < 1920) && !defined(__EDG__))
#error Please use Microsoft VS 2019 or a later standard for compilation.
#endif

template<int AC_WIDTH, int AC_BUS_WORDS = 1>
class ac_flag_gen_flush_1d
{
public:
  static_assert(AC_WIDTH > 0, "AC_WIDTH must be positive.");
  static_assert(AC_BUS_WORDS > 0, "AC_BUS_WORDS must be positive.");
  static_assert(AC_WIDTH%AC_BUS_WORDS == 0, "AC_WIDTH must be perfectly divisible by AC_BUS_WORDS.");

  typedef ac_int<ac::nbits<AC_WIDTH>::val, false> W_TYPE;

  void run (
    W_TYPE width,
    ac_window_v2_flush_state window_state,
    bool ping, bool flush_write_enabled,
    bool &sol_ping, bool &eol_ping, bool &sol_pong, bool &eol_pong
  ) {
    AC_ASSERT(width%AC_BUS_WORDS == 0, "width must be divisible by AC_BUS_WORDS.");
    
    bool is_window_flushing = (window_state == AC_WINDOW_FLUSH || window_state == AC_WINDOW_FLUSH_LB_ADVANCE);
    bool update_ping =  ping || (is_window_flushing && flush_write_enabled);
    bool update_pong = !ping || (is_window_flushing && flush_write_enabled);
    
    sol_ping = update_ping && (cnt_ping == 0);
    sol_pong = update_pong && (cnt_pong == 0);
    bool eol_ping_temp = update_ping && (cnt_ping == width - AC_BUS_WORDS);
    bool eol_pong_temp = update_pong && (cnt_pong == width - AC_BUS_WORDS);
    
    if (update_ping) {
      if (eol_ping_temp) {
        cnt_ping = 0;
      } else {
        cnt_ping += AC_BUS_WORDS;
      }
    }
    
    if (update_pong) {
      if (eol_pong_temp) {
        cnt_pong = 0;
      } else {
        cnt_pong += AC_BUS_WORDS;
      }
    }
    
    eol_ping = eol_ping_temp;
    eol_pong = eol_pong_temp;
  }

  void reset_primary_cnt(bool ping) {
    if (ping) {
      cnt_ping = 0;
    } else {
      cnt_pong = 0;
    }
  }

  void reset() {
    cnt_ping = cnt_pong = 0;
  }

  ac_flag_gen_flush_1d() {
    reset();
  }

private:
  ac_int<ac::nbits<AC_WIDTH - AC_BUS_WORDS>::val, false> cnt_ping, cnt_pong;
};

template<int AC_WIDTH, int AC_HEIGHT, int AC_BUS_WORDS = 1>
class ac_flag_gen_flush_2d
{
public:
  static_assert(AC_WIDTH > 0, "AC_WIDTH must be positive.");
  static_assert(AC_HEIGHT > 0, "AC_HEIGHT must be positive.");
  static_assert(AC_BUS_WORDS > 0, "AC_BUS_WORDS must be positive.");

  typedef ac_int<ac::nbits<AC_WIDTH>::val, false> W_TYPE;
  typedef ac_int<ac::nbits<AC_HEIGHT>::val, false> H_TYPE;

  void run (
    W_TYPE width, H_TYPE height,
    ac_window_v2_flush_state window_state,
    bool ping, bool flush_write_enabled,
    bool &sof_ping, bool &eof_ping, bool &sol_ping, bool &eol_ping,
    bool &sof_pong, bool &eof_pong, bool &sol_pong, bool &eol_pong,
    bool only_update_line = false // Only update line flags, i.e. SOL and EOL.
  ) {
    bool eol_ping_temp, eol_pong_temp;
    line.run(width, window_state, ping, flush_write_enabled, sol_ping, eol_ping_temp, sol_pong, eol_pong_temp);
    
    bool is_window_flushing = (window_state == AC_WINDOW_FLUSH || window_state == AC_WINDOW_FLUSH_LB_ADVANCE);
    bool update_ping =  ping ? !is_window_flushing : (is_window_flushing && flush_write_enabled);
    bool update_pong = !ping ? !is_window_flushing : (is_window_flushing && flush_write_enabled);
    
    sof_ping = update_ping && !only_update_line && (cnt_ping == 0) && sol_ping;
    sof_pong = update_pong && !only_update_line && (cnt_pong == 0) && sol_pong;
    bool eof_ping_temp = update_ping && !only_update_line && (cnt_ping == height - 1) && eol_ping_temp;
    bool eof_pong_temp = update_pong && !only_update_line && (cnt_pong == height - 1) && eol_pong_temp;
    
    if (update_ping) {
      if (eof_ping_temp) {
        cnt_ping = 0;
      } else {
        cnt_ping += int(eol_ping_temp && !only_update_line);
      }
    }
    
    if (update_pong) {
      if (eof_pong_temp) {
        cnt_pong = 0;
      } else {
        cnt_pong += int(eol_pong_temp && !only_update_line);
      }
    }
    
    eof_ping = eof_ping_temp;
    eol_ping = eol_ping_temp;
    eof_pong = eof_pong_temp;
    eol_pong = eol_pong_temp;
    
    // Error checking.
    bool sof_primary = ping ? sof_ping : sof_pong;
    
    // sof_primary is true only when SOF pixel is received in the idle state.
    if (sof_primary) {
      // Store the width and height of the frame to be processed.
      frame_width = width;
      frame_height = height;
    } else {
      incorrect_dim_change = (width != frame_width) || (height != frame_height);
      #ifndef DISABLE_AC_FLAG_GEN_FLUSH_ASSERT
      // The dimension inputs should only change when an SOF pixel is received in the idle state.
      // If a dimension input changes on any other pixel and if the DISABLE_AC_FLAG_GEN_ASSERT
      // macro is not defined, the assert below will be triggered.
      AC_ASSERT(!incorrect_dim_change, "Dimension input(s) changed unexpectedly. Make sure the dimensions only change when an SOF pixel is received in the idle state.");
      #endif
    }
  }

  bool dim_change_error() {
    return incorrect_dim_change;
  }

  void reset_primary_line_cnt(bool ping) {
    line.reset_primary_cnt(ping);
  }

  void reset() {
    line.reset();
    cnt_ping = cnt_pong = 0;
    incorrect_dim_change = false;
    frame_width = frame_height = 0;
  }

  ac_flag_gen_flush_2d() {
    reset();
  }

private:
  ac_flag_gen_flush_1d<AC_WIDTH, AC_BUS_WORDS> line; // ac_flag_gen_flush_2d is built on top of ac_flag_gen_flush_1d.
  ac_int<ac::nbits<AC_HEIGHT - 1>::val, false> cnt_ping, cnt_pong;
  bool incorrect_dim_change; // Did the dimension input change incorrectly?
  W_TYPE frame_width; // Width of the frame currently being processed.
  H_TYPE frame_height; // Height of the frame currently being processed.
};

#endif
