/**************************************************************************
 *                                                                        *
 *  Algorithmic C (tm) Image Processing Library                           *
 *                                                                        *
 *  Software Version: 2026.1                                              *
 *                                                                        *
 *  Release Date    : Tue Feb 10 18:37:14 PST 2026                        *
 *  Release Type    : Production Release                                  *
 *  Release Build   : 2026.1.0                                            *
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
//**********************************************************************************
//  File : ac_flag_gen_out_flush.h
//
//  Description:
//    Output flag generators for the AC Window 2.0 Flushing Support design.
//    ac_flag_gen_out_flush_1d generates position (SOL and EOL) flags for the
//    output window.
//    ac_flag_gen_out_flush_2d uses an ac_flag_gen_out_1d object to generate
//    output SOL and EOL flags, while also generating the output SOF, EOF and
//    valud flags by itself.
//
//  ac_flag_gen_out_flush_1d template parameters:
//    - AC_WIN_WIDTH: Window width.
//    - AC_BUS_WORDS: Number of pixels passed on bus in parallel. Default
//        value is 1.
//  
//  ac_flag_gen_out_flush_1d methods:
//    - void run (ac_int<AC_WORDS, false> sol_flags, ac_int<AC_WORDS, false> eol_flags, ac_window_v2_flush_state window_state, bool write_enabled, bool &sol_out, bool &eol_out, bool &vld_out):
//        Class interface run function.
//        *_flags are the flag shift registers that correspond to the primary frame.
//        window_state tells us which state the window is in.
//        write_enabled signals when the design processes inputs.
//        *_out flags are the output flags.
//    - ac_flag_gen_out_flush_1d():
//        Default constructor. It is empty because the class has no members.
//
//  ac_flag_gen_out_flush_2d template parameters:
//    - AC_WIN_HEIGHT: Window height. This must be greater than or equal to
//        three, i.e. the window must be at least three rows tall.
//    - AC_WIN_WIDTH: Window width.
//    - AC_BUS_WORDS: Number of pixels passed on bus in parallel. Default
//        value is 1.
//  
//  ac_flag_gen_out_flush_2d methods:
//    - void run (ac_int<AC_WIN_HEIGHT, false> sof_flags, ac_int<AC_WIN_HEIGHT, false> eof_flags, ac_int<AC_WORDS, false> sol_flags, ac_int<AC_WORDS, false> eol_flags, ac_window_v2_flush_state window_state, bool write_enabled, bool &sof_out, bool &eof_out, bool &sol_out, bool &eol_out, bool &vld_out):
//        Class interface run function.
//        *_flags are the flag shift registers that correspond to the primary frame.
//        window_state tells us which state the window is in.
//        write_enabled signals when the design processes inputs.
//        *_out flags are the output flags.
//    - ac_flag_gen_out_flush_2d():
//        Default constructor. It is empty because the class has no members.
//
//**********************************************************************************

#ifndef _INCLUDED_AC_FLAG_GEN_OUT_FLUSH_H_
#define _INCLUDED_AC_FLAG_GEN_OUT_FLUSH_H_

#include <ac_int.h>
#include <ac_array.h>
#include "ac_window_v2_flush_enums.h"

// The code uses static assertions, which are only supported by C++11 or later compiler
// standards. Hence, the user should be informed if they are not using those standards.
#if (defined(__GNUC__) && (__cplusplus < 201103L))
#error Please use C++11 or a later standard for compilation.
#endif
#if (defined(_MSC_VER) && (_MSC_VER < 1920) && !defined(__EDG__))
#error Please use Microsoft VS 2019 or a later standard for compilation.
#endif

template<int AC_WIN_WIDTH, int AC_BUS_WORDS = 1>
class ac_flag_gen_out_flush_1d
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
    ac_window_v2_flush_state window_state,
    bool write_enabled,
    bool &sol_out, bool &eol_out,
    bool &vld_out
  ) {
    bool update_flags = (window_state == AC_WINDOW_FLUSH || window_state == AC_WINDOW_FLUSH_LB_ADVANCE || write_enabled);
    
    constexpr int solf_idx = LPAD_WORDS;
    constexpr int eolf_idx = AC_WORDS - 1 - RPAD_WORDS;
    bool sol_out_temp = update_flags && sol_flags[solf_idx];
    eol_out = update_flags && eol_flags[eolf_idx];
    
    switch (window_state) {
      case AC_WINDOW_IDLE:
        vld_out = sol_out_temp;
        break;
      case AC_WINDOW_ACTIVE:
        vld_out = write_enabled;
        break;
      default:
        vld_out = true;
        break;
    }

    sol_out = sol_out_temp;
  }

  ac_flag_gen_out_flush_1d() { }
};

template<int AC_WIN_HEIGHT, int AC_WIN_WIDTH, int AC_BUS_WORDS = 1>
class ac_flag_gen_out_flush_2d
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
    ac_int<AC_WIN_HEIGHT, false> sof_flags, ac_int<AC_WIN_HEIGHT, false> eof_flags,
    ac_int<AC_WORDS, false> sol_flags, ac_int<AC_WORDS, false> eol_flags,
    ac_window_v2_flush_state window_state,
    bool write_enabled,
    bool &sof_out, bool &eof_out, bool &sol_out, bool &eol_out,
    bool &vld_out
  ) {
    bool update_flags = (window_state == AC_WINDOW_FLUSH || window_state == AC_WINDOW_FLUSH_LB_ADVANCE || write_enabled);
  
    ac_flag_gen_out_flush_1d<AC_WIN_WIDTH, AC_BUS_WORDS> line;
    bool sol_out_temp, eol_out_temp;
    bool vld_out_d; // Dummy value that's not actually used in 2D output flag generation.
    line.run(sol_flags, eol_flags, window_state, write_enabled, sol_out_temp, eol_out_temp, vld_out_d);

    bool sof_out_temp = update_flags && sol_out_temp && sof_flags[TPAD_LINES];
    eof_out = update_flags && eol_out_temp && eof_flags[AC_WIN_HEIGHT - BPAD_LINES];
    
    switch (window_state) {
      case AC_WINDOW_IDLE:
        vld_out = sof_out_temp;
        break;
      case AC_WINDOW_ACTIVE:
        vld_out = write_enabled;
        break;
      default:
        vld_out = true;
        break;
    }

    sof_out = sof_out_temp;
    sol_out = sol_out_temp;
    eol_out = eol_out_temp;
  }

  ac_flag_gen_out_flush_2d() { }
};

#endif
