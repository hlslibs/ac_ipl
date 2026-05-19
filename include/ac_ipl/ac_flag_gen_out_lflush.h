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
//*******************************************************************************
//  File : ac_flag_gen_out_lflush.h
//
//  Description:
//    Output flag generators for the AC Window 2.0 Line Flushing design.
//    ac_flag_gen_out_lflush_1d accepts the horizontal flag shifter output and
//    generates valid and position (SOL and EOL) flags for the horizontal
//    windows.
//    ac_flag_gen_out_lflush_2d accepts the 2D flag shifter outputs. It
//    instantiates an ac_flag_gen_out_lflush_1d object to generate output SOL
//    and EOL flags, while also generating the output SOF, EOF and valid flags
//    by itself.
//
//    ac_flag_gen_out_lflush_1d template parameters:
//    - AC_WIN_WIDTH: Window width.
//    - AC_BUS_WORDS: Number of pixels passed on bus in parallel. Default
//        value is 1.
//
//    ac_flag_gen_out_lflush_1d methods:
//    - void run (ac_int<AC_WORDS, false> sol_flags, ac_int<AC_WORDS, false> eol_flags, ac_window_v2_lflush_state window_hstate, bool &sol_out, bool &eol_out, bool &vld_out):
//        Class interface run function.
//        *_flags are the outputs of the horizontal flag shifter.
//        window_hstate input specifies the state the window is in with regards
//        to its horizontal position.
//        sol_out, eol_out and vld_out are the output SOL, EOL and valid flags,
//        respectively.
//    - ac_flag_gen_out_lflush_1d():
//        Default constructor. Calls the reset() function.
//
//    ac_flag_gen_out_lflush_2d template parameters:
//    - AC_WIN_HEIGHT: Window height. This must be greater than or equal to
//        three, i.e. the window must be at least three rows tall.
//    - AC_WIN_WIDTH: Window width.
//    - AC_BUS_WORDS: Number of pixels passed on bus in parallel. Default
//        value is 1.
//
//    ac_flag_gen_out_lflush_2d methods:
//    - void run (ac_int<AC_WIN_HEIGHT, false> sof_flags, ac_int<AC_WIN_HEIGHT, false> eof_flags, ac_int<AC_WORDS, false> sol_flags, ac_int<AC_WORDS, false> eol_flags, ac_window_v2_lflush_state window_hstate, ac_window_v2_lflush_state window_vstate, bool &sof_out, bool &eof_out, bool &sol_out, bool &eol_out, bool &vld_out):
//        Class interface run function.
//        *_flags are the outputs of the horizontal flag shifter.
//        window_hstate and window_vstate inputs specify the state the window is
//        in with regards to its horizontal and vertical positions, respectively.
//        sof_out, eof_out, sol_out, eol_out and vld_out are the output SOL, EOL
//        and valid flags, respectively.
//    - ac_flag_gen_out_lflush_2d():
//        Default constructor. Calls the reset() function.
//
//*******************************************************************************

#ifndef _INCLUDED_AC_FLAG_GEN_OUT_LFLUSH_H_
#define _INCLUDED_AC_FLAG_GEN_OUT_LFLUSH_H_

#include <ac_int.h>
#include <ac_array.h>
#include "ac_boundary_enums.h"
#include "ac_window_v2_lflush_enums.h"

// The code uses static assertions, which are only supported by C++11 or later compiler
// standards. Hence, the user should be informed if they are not using those standards.
#if (defined(__GNUC__) && (__cplusplus < 201103L))
#error Please use C++11 or a later standard for compilation.
#endif
#if (defined(_MSC_VER) && (_MSC_VER < 1920) && !defined(__EDG__))
#error Please use Microsoft VS 2019 or a later standard for compilation.
#endif

template<int AC_WIN_WIDTH, int AC_BUS_WORDS = 1>
class ac_flag_gen_out_lflush_1d
{
public:
  static_assert(AC_WIN_WIDTH > 0, "AC_WIN_WIDTH must be positive.");
  static_assert(AC_BUS_WORDS > 0, "AC_BUS_WORDS must be positive.");
  // Since we're only using this to calculate the padded words on either side, it doesn't
  // matter whether it's AC_CONSTANT or any other padding mode.
  typedef calc_shifter_words<AC_WIN_WIDTH, AC_CONSTANT, AC_BUS_WORDS> calc_pwords_type;

  enum {
    AC_WORDS = AC_WIN_WIDTH + AC_BUS_WORDS - 1,
    LPAD_WORDS = calc_pwords_type::LPAD_WORDS,
    RPAD_WORDS = calc_pwords_type::RPAD_WORDS
  };

  void run (
    ac_int<AC_WORDS, false> sol_flags, ac_int<AC_WORDS, false> eol_flags,
    ac_window_v2_lflush_state window_hstate,
    bool &sol_out,
    bool &eol_out,
    bool &vld_out
  ) {
    constexpr int solf_idx = LPAD_WORDS;
    constexpr int eolf_idx = AC_WORDS - 1 - RPAD_WORDS;
    bool sol_out_temp = sol_flags[solf_idx];
    bool eol_out_temp = eol_flags[eolf_idx];
    
    if (window_hstate == AC_WINDOW_RAMP_LF) {
      vld_out = sol_out_temp;
    } else {
      vld_out = true;
    }
    
    sol_out = sol_out_temp;
    eol_out = eol_out_temp;
  }

  ac_flag_gen_out_lflush_1d() { }
};

template<int AC_WIN_HEIGHT, int AC_WIN_WIDTH, int AC_BUS_WORDS = 1>
class ac_flag_gen_out_lflush_2d
{
public:
  static_assert(AC_WIN_HEIGHT > 0, "AC_WIN_HEIGHT must be positive.");
  static_assert(AC_WIN_WIDTH > 0, "AC_WIN_WIDTH must be positive.");
  static_assert(AC_BUS_WORDS > 0, "AC_BUS_WORDS must be positive.");
  #ifdef AC_WIN_V2_REVERSE_PADDING
  static_assert(AC_WIN_HEIGHT >= 2, "AC_WIN_HEIGHT must not be less than 2.");
  #else
  static_assert(AC_WIN_HEIGHT >= 3, "AC_WIN_HEIGHT must not be less than 3.");
  #endif

  // Since we're only using this to calculate the padded lines on either side, it doesn't
  // matter whether it's AC_CONSTANT or any other padding mode.
  typedef calc_shifter_height<AC_WIN_HEIGHT, AC_CONSTANT> calc_plines_type;

  enum {
    AC_WORDS = AC_WIN_WIDTH + AC_BUS_WORDS - 1,
    TPAD_LINES = calc_plines_type::TPAD_LINES,
    BPAD_LINES = calc_plines_type::BPAD_LINES,
  };

  void run (
    ac_int<AC_WIN_HEIGHT, false> sof_flags,
    ac_int<AC_WIN_HEIGHT, false> eof_flags,
    ac_int<AC_WORDS, false> sol_flags,
    ac_int<AC_WORDS, false> eol_flags,
    ac_window_v2_lflush_state window_hstate,
    ac_window_v2_lflush_state window_vstate,
    bool &sof_out, bool &eof_out, bool &sol_out, bool &eol_out,
    bool &vld_out
  ) {
    ac_flag_gen_out_lflush_1d<AC_WIN_WIDTH, AC_BUS_WORDS> line;
    bool sol_out_temp, eol_out_temp;
    bool vld_out_line;
    line.run(sol_flags, eol_flags, window_hstate, sol_out_temp, eol_out_temp, vld_out_line);

    bool sof_out_temp = sol_out_temp && sof_flags[TPAD_LINES];
    bool eof_out_temp = eol_out_temp && eof_flags[AC_WIN_HEIGHT - BPAD_LINES];
    
    if (window_vstate == AC_WINDOW_RAMP_LF) {
      vld_out = sof_out_temp;
    } else {
      vld_out = vld_out_line;
    }
    
    sof_out = sof_out_temp;
    eof_out = eof_out_temp;
    sol_out = sol_out_temp;
    eol_out = eol_out_temp;
  }

  ac_flag_gen_out_lflush_2d() { }
};

#endif
