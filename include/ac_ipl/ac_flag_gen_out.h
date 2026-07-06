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
//********************************************************************************
//  File : ac_flag_gen_out.h
//
//  Description:
//    Output flag generators for 1D and 2D AC Window 2.0 Standard Windowing
//    designs.
//    ac_flag_gen_out_1d accepts the horizontal flag shifter output and generates
//    valid and position (SOL and EOL) flags for the horizontal windows.
//    ac_flag_gen_out_2d accepts the 2D flag shifter outputs. It instantiates
//    an ac_flag_gen_out_1d object to generate output SOL and EOL flags, while
//    also generating the output SOF, EOF and valid flags by itself.
//
//    ac_flag_gen_out_1d template parameters:
//    - AC_WIN_WIDTH: Window width.
//    - AC_BUS_WORDS: Number of pixels passed on bus in parallel. Default
//        value is 1.
//
//    ac_flag_gen_out_1d methods:
//    -   void run (
//          ac_int<AC_WORDS, false> sol_flags,
//          ac_int<AC_WORDS, false> eol_flags,
//          bool &sol_out,
//          bool &eol_out,
//          bool &vld_out
//        ):
//        Class interface run function. *_flags are the outputs of the
//        horizontal flag shifter, while sol_out, eol_out and vld_out are
//        the output SOL, EOL and valid flags, respectively.
//    - void reset():
//        Resets rampup flag to 0/false.
//    - ac_flag_gen_out_1d():
//        Default constructor. Calls the reset() function.
//
//    ac_flag_gen_out_2d template parameters:
//    - AC_WIN_HEIGHT: Window height. This must be greater than or equal to
//        three, i.e. the window must be at least three rows tall.
//    - AC_WIN_WIDTH: Window width.
//    - AC_BUS_WORDS: Number of pixels passed on bus in parallel. Default
//        value is 1.
//
//    ac_flag_gen_out_2d methods:
//    - void run (
//        ac_int<AC_WIN_HEIGHT, false> &sof_flags,
//        ac_int<AC_WIN_HEIGHT, false> &eof_flags,
//        ac_int<AC_WORDS, false> sol_flags,
//        ac_int<AC_WORDS, false> eol_flags,
//        bool &sof_out, bool &eof_out, bool &sol_out, bool &eol_out,
//        bool &vld_out
//      ):
//        Class interface run function. *_flags are the outputs of the
//        2D flag shifter, while sof_out, eof_out, sol_out, eol_out
//        and vld_out are the output SOF, EOF, SOL, EOL and valid flags,
//        respectively.
//    - void reset():
//        Resets rampup flag to 0/false.
//    - ac_flag_gen_out_2d():
//        Default constructor. Calls the reset() function.
//
//********************************************************************************

#ifndef _INCLUDED_AC_FLAG_GEN_OUT_H_
#define _INCLUDED_AC_FLAG_GEN_OUT_H_

#include <ac_int.h>
#include <ac_array.h>
#include "ac_boundary_enums.h"

// The code uses static assertions, which are only supported by C++11 or later compiler
// standards. Hence, the user should be informed if they are not using those standards.
#if (defined(__GNUC__) && (__cplusplus < 201103L))
#error Please use C++11 or a later standard for compilation.
#endif
#if (defined(_MSC_VER) && (_MSC_VER < 1920) && !defined(__EDG__))
#error Please use Microsoft VS 2019 or a later standard for compilation.
#endif

template<int AC_WIN_WIDTH, int AC_BUS_WORDS = 1, bool USING_PADDING = true, bool IS_STATEFUL = false>
class ac_flag_gen_out_1d
{
public:
  static_assert(AC_WIN_WIDTH > 0, "AC_WIN_WIDTH must be positive.");
  static_assert(AC_BUS_WORDS > 0, "AC_BUS_WORDS must be positive.");
  typedef ac_int<AC_BUS_WORDS, false> VOUT_TYPE;

  typedef calc_shifter_words<AC_WIN_WIDTH, AC_NO_PADDING, AC_BUS_WORDS> cswords_np_type;
  // Since we're only using this to calculate the padded words on either side, it doesn't
  // matter whether it's AC_CONSTANT or any other padding mode.
  typedef calc_shifter_words<AC_WIN_WIDTH, AC_CONSTANT, AC_BUS_WORDS> calc_pwords_type;

  enum {
    AC_WORDS = cswords_np_type::AC_WORDS, // This is the same regardless of whether padding is used or not.
    NSF_NO_PAD = cswords_np_type::NUM_SHIFTS_FILL,
    ASW_NO_PAD = cswords_np_type::AC_SHIFT_WORDS,
    ALIGNED_NO_PAD = (ASW_NO_PAD == AC_WORDS),
    LPAD_WORDS = calc_pwords_type::LPAD_WORDS,
    RPAD_WORDS = calc_pwords_type::RPAD_WORDS
  };

  void run (
    ac_int<AC_WORDS, false> sol_flags,
    ac_int<AC_WORDS, false> eol_flags,
    bool &sol_out,
    bool &eol_out,
    VOUT_TYPE &vld_out
  ) {
    constexpr int solf_idx = USING_PADDING ? LPAD_WORDS : 0;
    bool sol_out_temp = sol_flags[solf_idx];
    bool eol_out_temp = eol_flags[eolf_idx];
    sol_out = sol_out_temp;
    eol_out = eol_out_temp;

    if (IS_STATEFUL) {
      if (sol_out_temp) {
        ramped_up = true; // Window is ramped up when the output line starts.
      }
      vld_out = -int(ramped_up);
    } else {
      // If the flag generator is not stateful, we don't need the "ramped_up" flag.
      vld_out = -1;
    }

    if (!USING_PADDING && !ALIGNED_NO_PAD && eol_out_temp) {
      vld_out = eol_vout_np;
    }

    if (IS_STATEFUL && eol_out_temp) {
      #pragma hls_waive NCO
      ramped_up = false; // Window is no longer ramped up once the output line ends.
    }
  }

  void reset() {
    ramped_up = false;
  }

  ac_flag_gen_out_1d() : eol_vout_np(set_eol_vout_for_np()) {
    reset();
  }

private:
  enum {
    pad_idx = AC_WORDS - 1 - RPAD_WORDS,
    no_pad_idx = ALIGNED_NO_PAD ? AC_WORDS - 1 : ASW_NO_PAD - AC_BUS_WORDS - 1,
    eolf_idx = USING_PADDING ? pad_idx : no_pad_idx
  };

  static VOUT_TYPE set_eol_vout_for_np() {
    // Minimum set to 0 to prevent compiler warnings in some cases.
    constexpr int ones_upto_idx = AC_MAX(eolf_idx - (AC_WORDS - AC_BUS_WORDS), 0);
    ac_int<AC_BUS_WORDS, false> out = -1;
    if (!USING_PADDING && !ALIGNED_NO_PAD) {
      out = 0;
      out[ones_upto_idx + 1] = 1;
      --out;
    }
    return out;
  }

  const VOUT_TYPE eol_vout_np;
  bool ramped_up; // Has window ramped up?
};

template<int AC_WIN_HEIGHT, int AC_WIN_WIDTH, int AC_BUS_WORDS = 1, bool USING_PADDING = true>
class ac_flag_gen_out_2d
{
public:
  static_assert(AC_WIN_HEIGHT > 0, "AC_WIN_HEIGHT must be positive.");
  static_assert(AC_WIN_WIDTH > 0, "AC_WIN_WIDTH must be positive.");
  static_assert(AC_BUS_WORDS > 0, "AC_BUS_WORDS must be positive.");
  #ifdef AC_WIN_V2_REVERSE_PADDING
  static_assert(!USING_PADDING || AC_WIN_HEIGHT >= 2, "AC_WIN_HEIGHT must not be less than 2, if boundary padding is used.");
  #else
  static_assert(!USING_PADDING || AC_WIN_HEIGHT >= 3, "AC_WIN_HEIGHT must not be less than 3, if boundary padding is used.");
  #endif

  // Since we're only using this to calculate the padded lines on either side, it doesn't
  // matter whether it's AC_CONSTANT or any other padding mode.
  typedef calc_shifter_height<AC_WIN_HEIGHT, AC_CONSTANT> calc_plines_type;

  enum {
    AC_WORDS = AC_WIN_WIDTH + AC_BUS_WORDS - 1,
    TPAD_LINES = calc_plines_type::TPAD_LINES,
    BPAD_LINES = calc_plines_type::BPAD_LINES,
    // Under certain conditions, the 1D output flag generator is always valid
    // for the entire frame, once sof_out = 1. In such a case, the 1D flag
    // generator does not need to be stateful as the validity of the output
    // doesn't change inside the frame.
    LINE_IS_STATEFUL = !USING_PADDING && ((AC_WIN_WIDTH - 1) >= AC_BUS_WORDS)
  };

  typedef ac_flag_gen_out_1d<AC_WIN_WIDTH, AC_BUS_WORDS, USING_PADDING, LINE_IS_STATEFUL> LINE_TYPE;
  typedef typename LINE_TYPE::VOUT_TYPE VOUT_TYPE;

  enum {
    ALIGNED_NO_PAD = LINE_TYPE::ALIGNED_NO_PAD,
  };

  void run (
    ac_int<AC_WIN_HEIGHT, false> sof_flags,
    ac_int<AC_WIN_HEIGHT, false> eof_flags,
    ac_int<AC_WORDS, false> sol_flags,
    ac_int<AC_WORDS, false> eol_flags,
    bool &sof_out, bool &eof_out, bool &sol_out, bool &eol_out,
    VOUT_TYPE &vld_out,
    // eof_vals is needed if we're not using padding.
    // eof_vals[0] = eof, eof_vals[1] = eof_old (from flag shifter)
    ac_int<2, false> eof_vals = 0
  ) {
    bool sof_out_temp, eof_out_temp, sol_out_temp, eol_out_temp;
    VOUT_TYPE line_ramped_up;
    line.run(sol_flags, eol_flags, sol_out_temp, eol_out_temp, line_ramped_up);
    
    if (USING_PADDING) {
      sof_out_temp = sol_out_temp && sof_flags[TPAD_LINES];
      eof_out_temp = eol_out_temp && eof_flags[AC_WIN_HEIGHT - BPAD_LINES];
    } else {
      sof_out_temp = sol_out_temp && sof_flags[0];
      eof_out_temp = eol_out_temp && (ALIGNED_NO_PAD ? eof_vals[0] : eof_vals[1]);
    }

    if (sof_out_temp) {
      ramped_up_v = true; // Window is ramped up vertically when the output frame starts.
    }

    VOUT_TYPE vld_out_temp = 0;
    if (ramped_up_v) { vld_out_temp = line_ramped_up; }
    vld_out = vld_out_temp;

    sof_out = sof_out_temp;
    eof_out = eof_out_temp;
    sol_out = sol_out_temp;
    eol_out = eol_out_temp;
  }

  void reset() {
    line.reset();
    ramped_up_v = false;
  }

  ac_flag_gen_out_2d() {
    reset();
  }

private:
  LINE_TYPE line;
  bool ramped_up_v;
};

#endif
