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
//****************************************************************************
//  File : ac_flag_gen.h
//
//  Description:
//    Classes which generate synchronization flags for images in one and two
//    dimensions. ac_flag_gen_1d handles flag generation for 1D lines of
//    pixels and produces sol and eol flags, while ac_flag_gen_2d handles
//    flag generation for 2D frames. ac_flag_gen_2d is built on top of
//    ac_flag_gen_1d, with the latter computing sol and eol flags while
//    the former computes sof and eof flags on top of that.
//
//    ac_flag_gen_1d template parameters:
//    - AC_WIDTH: Maximum line width.
//    - AC_BUS_WORDS: Number of pixels passed on bus in parallel. Default
//        value is 1.
//
//    ac_flag_gen_1d methods:
//    - void run (W_TYPE width, bool &sol, bool &eol):
//        Class interface run function. Accepts line width as inputs. Produces
//        sol and eol flags as outputs.
//    - void reset():
//        Resets all class member variables.
//    - bool dim_change_error():
//        Returns true if line width was changed mid-line.
//    - ac_flag_gen_1d():
//        Default constructor. Calls the reset() function.
//
//    ac_flag_gen_2d template parameters:
//    - AC_WIDTH: Maximum frame width.
//    - AC_HEIGHT: Maximum frame height.
//    - AC_BUS_WORDS: Number of pixels passed on bus in parallel. Default
//        value is 1.
//
//    ac_flag_gen_2d methods:
//    - void run (W_TYPE width, H_TYPE height, bool &sof, bool &eof, bool &sol, bool &eol, bool only_update_line = false):
//        Class interface run function. Accepts frame width and
//        height and an optional only_update_line flag as inputs. Produces
//        sof, eof, sol and eol flags as outputs.
//    - void reset():
//        Resets all class member variables.
//    - bool dim_change_error():
//        Returns true if frame width and/or height were changed mid-frame.
//    - ac_flag_gen_2d():
//        Default constructor. Sets cnt, frame_width and frame_height member
//        variables to zero and incorrect_dim_change member variable to false.
//
//****************************************************************************

#ifndef _INCLUDED_AC_FLAG_GEN_H_
#define _INCLUDED_AC_FLAG_GEN_H_

#include <ac_int.h>

// The code uses static assertions, which are only supported by C++11 or later compiler
// standards. Hence, the user should be informed if they are not using those standards.
#if (defined(__GNUC__) && (__cplusplus < 201103L))
#error Please use C++11 or a later standard for compilation.
#endif
#if (defined(_MSC_VER) && (_MSC_VER < 1920) && !defined(__EDG__))
#error Please use Microsoft VS 2019 or a later standard for compilation.
#endif

template<int AC_WIDTH, int AC_BUS_WORDS = 1>
class ac_flag_gen_1d
{
public:
  static_assert(AC_WIDTH > 0, "AC_WIDTH must be positive.");
  static_assert(AC_BUS_WORDS > 0, "AC_BUS_WORDS must be positive.");
  static_assert(AC_WIDTH%AC_BUS_WORDS == 0, "AC_WIDTH must be perfectly divisible by AC_BUS_WORDS.");

  typedef ac_int<ac::nbits<AC_WIDTH>::val, false> W_TYPE;

  void run (
    W_TYPE width, // Dimension input: width.
    bool &sol, bool &eol
  ) {
    AC_ASSERT(width%AC_BUS_WORDS == 0, "width must be divisible by AC_BUS_WORDS.");
    bool sol_temp = (cnt == 0);

    if (sol_temp) {
      // Store the width of the line to be processed.
      line_width = width;
    } else {
      incorrect_dim_change = (width != line_width);
      #ifndef DISABLE_AC_FLAG_GEN_ASSERT
      // The "width" input should only change when the SOL pixel is received. If the input changes on
      // any other pixel and if the DISABLE_AC_FLAG_GEN_ASSERT macro is not defined, the assert below
      // will be triggered.
      AC_ASSERT(!incorrect_dim_change, "Dimension input changed unexpectedly. Make sure width only changes when SOL pixel is received.");
      #endif
    }
    // EOL flag is set to true if cnt is at the maximum value possible, i.e. width - AC_BUS_WORDS.
    bool eol_temp = (cnt == width - AC_BUS_WORDS);

    if (eol_temp) {
      cnt = 0; // cnt is reset after eol pixel is received.
    } else {
      cnt += AC_BUS_WORDS; // Increase count if we aren't at EOL.
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

  ac_flag_gen_1d() {
    reset();
  }

private:
  ac_int<ac::nbits<AC_WIDTH - AC_BUS_WORDS>::val, false> cnt; // Counter for number of pixels processed.
  W_TYPE line_width; // Width of the line being currently processed.
  bool incorrect_dim_change; // Did the dimension input change incorrectly?
};

template<int AC_WIDTH, int AC_HEIGHT, int AC_BUS_WORDS = 1>
class ac_flag_gen_2d
{
public:
  static_assert(AC_WIDTH > 0, "AC_WIDTH must be positive.");
  static_assert(AC_HEIGHT > 0, "AC_HEIGHT must be positive.");
  static_assert(AC_BUS_WORDS > 0, "AC_BUS_WORDS must be positive.");

  typedef ac_int<ac::nbits<AC_WIDTH>::val, false> W_TYPE;
  typedef ac_int<ac::nbits<AC_HEIGHT>::val, false> H_TYPE;

  void run (
    W_TYPE width, H_TYPE height, // Dimension inputs: width and height.
    bool &sof, bool &eof, bool &sol, bool &eol,
    bool only_update_line = false // Only update line flags, i.e. SOL and EOL.
  ) {
    bool sol_temp, eol_temp;
    line.run(width, sol_temp, eol_temp);
    bool sof_temp = !only_update_line && (cnt == 0) && sol_temp;
    
    if (sof_temp) {
      // Store the width and height of the frame to be processed.
      frame_width = width;
      frame_height = height;
    } else {
      incorrect_dim_change = (width != frame_width) || (height != frame_height);
      #ifndef DISABLE_AC_FLAG_GEN_ASSERT
      // The dimension inputs should only change when the SOF pixel is received. If a dimension input
      // changes on any other pixel and if the DISABLE_AC_FLAG_GEN_ASSERT macro is not defined, the
      // assert below will be triggered.
      AC_ASSERT(!incorrect_dim_change, "Dimension input(s) changed unexpectedly. Make sure the dimensions only change when SOF pixel is received.");
      #endif
    }
    // EOF flag is set to true if we're at EOL and if cnt is at the maximum value possible, i.e.
    // height - 1.
    bool eof_temp = !only_update_line && (cnt == height - 1) && eol_temp;

    if (eof_temp) {
      cnt = 0; // cnt is reset after EOF pixel is received.
    } else {
      cnt += int(eol_temp && !only_update_line);
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

  ac_flag_gen_2d() : cnt(0), incorrect_dim_change(false), frame_width(0), frame_height(0) { }

private:
  ac_flag_gen_1d<AC_WIDTH, AC_BUS_WORDS> line; // ac_flag_gen_2d is built on top of ac_flag_gen_1d.
  ac_int<ac::nbits<AC_HEIGHT - 1>::val, false> cnt;
  bool incorrect_dim_change; // Did the dimension input(s) change incorrectly?
  W_TYPE frame_width; // Width of the frame currently being processed.
  H_TYPE frame_height; // Height of the frame currently being processed.
};

#endif
