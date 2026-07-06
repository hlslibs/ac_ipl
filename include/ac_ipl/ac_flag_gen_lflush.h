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
//*******************************************************************************
//  File : ac_flag_gen_lflush.h
//
//  Description:
//    Synchronization flag generator for the for the AC Window 2.0 Line
//    Flushing design. ac_flag_gen_lflush_1d handles flag generation for 1D lines
//    of pixels and produces sol and eol flags, while ac_flag_gen_lflush_2d
//    handles flag generation for 2D frames. ac_flag_gen_lflush_2d is built on
//    top of ac_flag_gen_lflush_1d, with the latter computing sol and eol flags
//    while the former computes sof and eof flags on top of that.
//
//    ac_flag_gen_lflush_1d template parameters:
//    - AC_WIDTH: Maximum line width.
//    - AC_BUS_WORDS: Number of pixels passed on bus in parallel. Default
//        value is 1.
//
//    ac_flag_gen_lflush_1d methods:
//    - void run (W_TYPE width, ac_window_v2_lflush_state window_hstate, bool &sol, bool &eol):
//        Class interface run function.
//        width is the line width.
//        window_hstate input specifies the state the window is in with regards
//        to its horizontal position.
//        sol and eol are the output flags.
//    - bool dim_change_error():
//        Returns true if line width was changed mid-line.
//    - void reset():
//        Resets member variables.
//    - ac_flag_gen_lflush_1d():
//        Default constructor. Calls the reset() function.
//
//    ac_flag_gen_lflush_2d template parameters:
//    - AC_WIN_HEIGHT: Window height.
//    - AC_WIDTH: Maximum line width.
//    - AC_BUS_WORDS: Number of pixels passed on bus in parallel. Default
//        value is 1.
//    - AC_REPEAT: Set to true if the window is expected to repeat over the
//        same set of lines for algorithms like interpolation.
//        Default value = false.
//
//    ac_flag_gen_lflush_2d methods:
//    - void run (W_TYPE width, H_TYPE height, ac_window_v2_lflush_state window_hstate, ac_window_v2_lflush_state window_vstate, bool &sof, bool &eof, bool &sol, bool &eol, bool repeat_line = false):
//        Class interface run function.
//        width and height are the frame width and height, respectively.
//        window_hstate and window_vstate inputs specify the state the window is
//        in with regards to its horizontal and vertical positions, respectively.
//        sof, eof, sol and eol are the output flags.
//        If repeat_line is set to true, it is assumed that the window is
//        repeating over the same set of lines. In such a case, the SOF+EOF flags
//        as well as the associated counter is not updated.
//    - bool dim_change_error():
//        Returns true if frame width and/or height were changed mid-frame.
//    - void reset():
//        Resets member variables.
//    - ac_flag_gen_lflush_2d():
//        Default constructor. Calls the reset() function.
//
//*******************************************************************************

#ifndef _INCLUDED_AC_FLAG_GEN_LFLUSH_H_
#define _INCLUDED_AC_FLAG_GEN_LFLUSH_H_

#include <ac_int.h>
#include "ac_window_v2_lflush_enums.h"

// The code uses static assertions, which are only supported by C++11 or later compiler
// standards. Hence, the user should be informed if they are not using those standards.
#if (defined(__GNUC__) && (__cplusplus < 201103L))
#error Please use C++11 or a later standard for compilation.
#endif
#if (defined(_MSC_VER) && (_MSC_VER < 1920) && !defined(__EDG__))
#error Please use Microsoft VS 2019 or a later standard for compilation.
#endif

template<int AC_WIDTH, int AC_BUS_WORDS = 1>
class ac_flag_gen_lflush_1d
{
public:
  static_assert(AC_WIDTH > 0, "AC_WIDTH must be positive.");
  static_assert(AC_BUS_WORDS > 0, "AC_BUS_WORDS must be positive.");
  static_assert(AC_WIDTH%AC_BUS_WORDS == 0, "AC_WIDTH must be perfectly divisible by AC_BUS_WORDS.");

  typedef ac_int<ac::nbits<AC_WIDTH>::val, false> W_TYPE;

  void run (
    W_TYPE width, // Dimension input: width.
    ac_window_v2_lflush_state window_hstate,
    bool &sol, bool &eol
  ) {
    AC_ASSERT(width%AC_BUS_WORDS == 0, "width must be divisible by AC_BUS_WORDS.");
    bool not_in_RER = (window_hstate != AC_WINDOW_ER_LF);
    bool sol_temp = not_in_RER && (cnt == 0);

    if (sol_temp) {
      // Store the width of the line to be processed.
      line_width = width;
    } else {
      incorrect_dim_change = (width != line_width);
      #ifndef DISABLE_AC_FLAG_GEN_LFLUSH_ASSERT
      // The "width" input should only change when the SOL pixel is received. If the input changes on
      // any other pixel and if the DISABLE_AC_FLAG_GEN_LFLUSH_ASSERT macro is not defined, the assert
      // below will be triggered.
      AC_ASSERT(!incorrect_dim_change, "Dimension input changed unexpectedly. Make sure width only changes when SOL pixel is received.");
      #endif
    }
    
    bool eol_temp = not_in_RER && (cnt == width - AC_BUS_WORDS);

    if (eol_temp) {
      cnt = 0;
    } else {
      ac_int<ac::nbits<AC_BUS_WORDS>::val, false> added_val = AC_BUS_WORDS;
      ac_int<added_val.width, false> mask = -int(not_in_RER);
      added_val &= mask;
      cnt += added_val;
    }

    sol = sol_temp;
    eol = eol_temp;
  }

  void reset() {
    cnt = 0;
    incorrect_dim_change = false;
    line_width = 0;
  }

  bool dim_change_error() {
    return incorrect_dim_change;
  }

  ac_flag_gen_lflush_1d() {
    reset();
  }

private:
  ac_int<ac::nbits<AC_WIDTH - AC_BUS_WORDS>::val, false> cnt; // Counter for number of pixels processed.
  W_TYPE line_width; // Width of the line being currently processed.
  bool incorrect_dim_change; // Did the dimension input change incorrectly?
};

template<int AC_WIDTH, int AC_HEIGHT, int AC_BUS_WORDS = 1, bool AC_REPEAT = false>
class ac_flag_gen_lflush_2d
{
public:
  static_assert(AC_WIDTH > 0, "AC_WIDTH must be positive.");
  static_assert(AC_HEIGHT > 0, "AC_HEIGHT must be positive.");
  static_assert(AC_BUS_WORDS > 0, "AC_BUS_WORDS must be positive.");

  typedef ac_int<ac::nbits<AC_WIDTH>::val, false> W_TYPE;
  typedef ac_int<ac::nbits<AC_HEIGHT>::val, false> H_TYPE;

  void run (
    W_TYPE width, H_TYPE height, // Dimension inputs: width and height.
    ac_window_v2_lflush_state window_hstate,
    ac_window_v2_lflush_state window_vstate,
    bool &sof, bool &eof, bool &sol, bool &eol,
    bool repeat_line = false
  ) {
    AC_ASSERT(AC_REPEAT || !repeat_line, "repeat_line cannot be set to true if AC_REPEAT == false.");
    
    bool sol_temp, eol_temp;
    line.run(width, window_hstate, sol_temp, eol_temp);
    
    bool not_in_BER = (window_vstate != AC_WINDOW_ER_LF);
    // dont_repeat is set to false if we're iterating over the same set of lines,
    // and it's set to true if we're not doing that.
    bool dont_repeat = !(AC_REPEAT && repeat_line);
    bool update_vflags = dont_repeat && not_in_BER;
    
    bool sof_temp = update_vflags && (cnt == 0) && sol_temp;

    if (sof_temp) {
      // Store the width and height of the frame to be processed.
      frame_width = width;
      frame_height = height;
    } else {
      incorrect_dim_change = (width != frame_width) || (height != frame_height);
      #ifndef DISABLE_AC_FLAG_GEN_LFLUSH_ASSERT
      // The dimension inputs should only change when the SOF pixel is received. If a dimension input
      // changes on any other pixel and if the DISABLE_AC_FLAG_GEN_LFLUSH_ASSERT macro is not defined, the
      // assert below will be triggered.
      AC_ASSERT(!incorrect_dim_change, "Dimension input(s) changed unexpectedly. Make sure the dimensions only change when SOF pixel is received.");
      #endif
    }
    
    bool eof_temp = update_vflags && (cnt == height - 1) && eol_temp;
    
    if (eof_temp) {
      cnt = 0; // cnt is reset after EOF pixel is received.
    } else {
      cnt += int(eol_temp && update_vflags);
    }

    sof = sof_temp;
    eof = eof_temp;
    sol = sol_temp;
    eol = eol_temp;
  }

  void reset() {
    cnt = 0;
    line.reset();
    incorrect_dim_change = false;
    frame_width = 0;
    frame_height = 0;
  }

  bool dim_change_error() {
    return incorrect_dim_change;
  }

  ac_flag_gen_lflush_2d() : cnt(0), incorrect_dim_change(false), frame_width(0), frame_height(0) { }

private:
  ac_flag_gen_lflush_1d<AC_WIDTH, AC_BUS_WORDS> line;
  ac_int<ac::nbits<AC_HEIGHT - 1>::val, false> cnt;
  bool incorrect_dim_change; // Did the dimension input(s) change incorrectly?
  W_TYPE frame_width; // Width of the frame currently being processed.
  H_TYPE frame_height; // Height of the frame currently being processed.
};

#endif
