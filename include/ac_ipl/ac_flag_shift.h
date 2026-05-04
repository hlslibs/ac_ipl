/**************************************************************************
 *                                                                        *
 *  Algorithmic C (tm) Image Processing Library                           *
 *                                                                        *
 *  Software Version: 2026.1                                              *
 *                                                                        *
 *  Release Date    : Wed Mar 11 20:46:14 PDT 2026                        *
 *  Release Type    : Production Release                                  *
 *  Release Build   : 2026.1.1                                            *
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
//  File : ac_flag_shift.h
//
//  Description:
//    Flag shifters for 1D and 2D AC Window 2.0 designs. ac_flag_shift_1d
//    handles shifting in one dimension such that the shifted flags align
//    with the outputs from the data shifters.
//    ac_flag_shift_2d instantiates an ac_flag_gen_1d object to serve as the
//    horizontal flag shifter. It also uses the output from the horizontal
//    shifter to vertically shift the SOF and EOF flag arrays and align those
//    with the 2D data shifter output.
//
//    ac_flag_shift_1d template parameters:
//    - AC_WIN_WIDTH: Window width.
//    - AC_PMODE: Padding mode/method.
//    - AC_BUS_WORDS: Number of pixels passed on bus in parallel.
//        Default value = 1.
//
//    ac_flag_shift_1d methods:
//    - void run(bool sol, bool eol, ac_int<AC_WORDS, false> &sol_flags, ac_int<AC_WORDS, false> &eol_flags):
//        Class interface run function. Right-shifts sol and eol inputs into
//        the horizontal flag shifters. Also slices the first AC_WORDS bits
//        from the flag shifters to the sol_flags and eol_flags arrays.
//    - void reset():
//        Resets the horizontal flag shifters to 0.
//    - ac_flag_shift_1d():
//        Default constructor. Calls the reset() function.
//
//    ac_flag_shift_2d template parameters:
//    - AC_WIN_HEIGHT: Window height.
//    - AC_WIN_WIDTH: Window width.
//    - AC_PMODE: Padding mode/method.
//    - AC_BUS_WORDS: Number of pixels passed on bus in parallel.
//        Default value = 1.
//    - AC_REPEAT: Set to true if the window is expected to repeat over the
//        same set of lines for algorithms like interpolation.
//        Default value = false.
//
//    ac_flag_shift_2d methods:
//    - bool run(
//        bool sof, bool eof, bool sol, bool eol,
//        ac_int<AC_WIN_HEIGHT, false> &sof_flags,
//        ac_int<AC_WIN_HEIGHT, false> &eof_flags,
//        ac_int<AC_WORDS, false> &sol_flags,
//        ac_int<AC_WORDS, false> &eol_flags,
//        bool repeat_line = false
//      ):
//        Class interface run function. Accepts input flags and shifts them
//        into the horizontal and vertical flag shifters. If repeat_line is
//        set to true, it is assumed that the window will repeat over the same
//        lines, and the vertical flag registers (sof_regs and eof_regs) will
//        not accept new inputs. repeat_line can only be set to true if the
//        AC_REPEAT template parameter is true. It is set to false by default.
//    - void reset():
//        Resets all flag shifters to 0.
//    - ac_flag_shift_2d():
//        Default constructor. Calls the reset() function.
//
//****************************************************************************

#ifndef _INCLUDED_AC_FLAG_SHIFT_H_
#define _INCLUDED_AC_FLAG_SHIFT_H_

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

template<int AC_WIN_WIDTH, ac_padding_method AC_PMODE, int AC_BUS_WORDS = 1>
class ac_flag_shift_1d
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
  
  #ifdef AC_WIN_V2_REVERSE_PADDING
  static_assert(AC_PMODE != AC_REFLECT101, "AC_REFLECT101 must not be used if you intend to reverse image padding for even window sizes.");
  #endif

  void run(bool sol, bool eol, ac_int<AC_WORDS, false> &sol_flags, ac_int<AC_WORDS, false> &eol_flags) {
    // Right-shift both the SOL and EOL flag registers. Store the sol and eol flags in the appropriate positions.
    sol_regs >>= AC_BUS_WORDS;
    eol_regs >>= AC_BUS_WORDS;
    sol_regs[AC_SHIFT_WORDS - AC_BUS_WORDS] = sol;
    eol_regs[AC_SHIFT_WORDS - 1] = eol;

    // One can also use a direct assignment to slice out the required number of flags, but the slc()
    // method is explicitly called here to remind the user that we're slicing AC_WORDS LSBs out of the
    // flag registers.
    sol_flags = sol_regs.template slc<AC_WORDS>(0);
    eol_flags = eol_regs.template slc<AC_WORDS>(0);
  }

  void reset() {
    // Reset SOL and EOL shift registers.
    sol_regs = eol_regs = 0;
  }

  ac_flag_shift_1d() {
    reset();
  }

private:
  ac_int<AC_SHIFT_WORDS, false> sol_regs, eol_regs;
};

template <int AC_WIN_HEIGHT, int AC_WIN_WIDTH, ac_padding_method AC_PMODE, int AC_BUS_WORDS = 1, bool AC_REPEAT = false>
class ac_flag_shift_2d
{
public:
  static_assert(AC_WIN_HEIGHT > 0, "AC_WIN_HEIGHT must be positive.");
  static_assert(AC_WIN_WIDTH > 0, "AC_WIN_WIDTH must be positive.");
  static_assert(AC_BUS_WORDS > 0, "AC_BUS_WORDS must be positive.");

  typedef calc_shifter_height<AC_WIN_HEIGHT, AC_PMODE> csheight_type;
  typedef ac_flag_shift_1d<AC_WIN_WIDTH, AC_PMODE, AC_BUS_WORDS> LINE_TYPE;

  enum {
    EXTRA_LINE = csheight_type::EXTRA_LINE,
    AC_SHIFT_HEIGHT = csheight_type::AC_SHIFT_HEIGHT,
    AC_WORDS = LINE_TYPE::AC_WORDS,
    EXTRA_WORD = LINE_TYPE::EXTRA_WORD,
    NUM_SHIFTS_FILL = LINE_TYPE::NUM_SHIFTS_FILL,
    AC_SHIFT_WORDS = LINE_TYPE::AC_SHIFT_WORDS // For external use, if needed.
  };

  bool run(
    bool sof, bool eof, bool sol, bool eol,
    ac_int<AC_WIN_HEIGHT, false> &sof_flags,
    ac_int<AC_WIN_HEIGHT, false> &eof_flags,
    ac_int<AC_WORDS, false> &sol_flags,
    ac_int<AC_WORDS, false> &eol_flags,
    bool repeat_line = false
  ) {
    AC_ASSERT(AC_REPEAT || !repeat_line, "repeat_line cannot be set to true if AC_REPEAT == false.");

    // Call line object to perform 1D flag shifting on sol and eol flag arrays.
    line.run(sol, eol, sol_flags, eol_flags);

    constexpr int LAST_COL_IDX = NUM_SHIFTS_FILL + EXTRA_WORD - 1;

    // CCOV_EXCL_START
    // Shift all columns of sof_regs and eof_regs except for the last one.
    #pragma hls_waive CCC
    #pragma hls_unroll yes
    FSHIFT_2D_I: for (int j = 0; j < LAST_COL_IDX; j++) {
      #pragma hls_unroll yes
      FSHIFT_2D_J: for (int i = 0; i < AC_SHIFT_HEIGHT; i++) {
        sof_regs[i][j] = sof_regs[i][j + 1];
        eof_regs[i][j] = eof_regs[i][j + 1];
      }
    }
    // CCOV_EXCL_STOP

    // dont_repeat is set to false if we're iterating over the same set of lines,
    // and it's set to true if we're not doing that.
    bool dont_repeat = !(AC_REPEAT && repeat_line);

    // Shift all rows of the last column of the sof_regs and eof_regs arrays
    // except for the last row, if we're at the sol/eol pixel, respectively.
    #pragma hls_unroll yes
    FSHIFT_2D_LAST_COL: for (int i = 0; i < AC_SHIFT_HEIGHT - 1; i++) {
      if (sol && dont_repeat) {
        sof_regs[i][LAST_COL_IDX] = sof_regs[i + 1][LAST_COL_IDX];
      }
      // Since we do the EOF shifting before the boundary processing, we have
      // to make sure that the EOF shifting lags by one clock cycle, by using
      // flag data from the previous iteration.
      if (eol_old && dont_repeat) {
        eof_regs[i][LAST_COL_IDX] = eof_regs[i + 1][LAST_COL_IDX];
      }
    }

    // Shift in the sof and eof flags into the last row of the
    // last column of the sof_regs and eof_regs flag, respectively,
    // provided that we're at the SOL/EOL pixel.
    if (sol && dont_repeat) {
      sof_regs[AC_SHIFT_HEIGHT - 1][LAST_COL_IDX] = sof;
    }

    // Since we do the EOF shifting before the boundary processing, we have
    // to make sure that the EOF shifting lags by one clock cycle, by using
    // flag data from the previous iteration.
    if (eol_old && dont_repeat) {
      eof_regs[AC_SHIFT_HEIGHT - 1][LAST_COL_IDX] = eof_old;
    }

    // Write out the first column of the sof_regs and eof_regs shifters to
    // the sof_flags and eof_flags outputs, respectively. If we've added an
    // extra row to the shifters to enable correct AC_REFLECT101 padding, we
    // ignore the extra row in the final output.
    #pragma hls_unroll yes
    FSHIFT_2D_COPY_TO_OUT: for (int i = 0; i < AC_WIN_HEIGHT; i++) {
      #pragma hls_waive UMR
      sof_flags[i] = sof_regs[i][0];
      #pragma hls_waive UMR
      eof_flags[i] = eof_regs[i][0];
    }

    bool eof_old_returned = eof_old;

    // If AC Window is not set to repeat over the same set of lines, update eof_old
    // with the eof value. If it is set to repeat over the same set of lines, retain
    // the eof value from the end of the last input line until the repetition is over.
    eof_old = (dont_repeat && eof) || (!dont_repeat && eof_old);
    eol_old = eol;

    return eof_old_returned;
  }

  void reset() {
    // Reset the 1D shift registers to zero.
    line.reset();

    // Reset all SOF and EOF shift registers to zero.
    #pragma hls_unroll yes
    RESET_FSHIFT_2D_I: for (int i = 0; i < AC_SHIFT_HEIGHT; i++) {
      #pragma hls_unroll yes
      RESET_FSHIFT_2D_J: for (int j = 0; j < NUM_SHIFTS_FILL + EXTRA_WORD; j++) {
        sof_regs[i][j] = eof_regs[i][j] = false;
      }
    }

    eof_old = eol_old = false;
  }

  ac_flag_shift_2d() {
    reset();
  }

private:
  LINE_TYPE line;
  // Add an extra column to both sof_regs and eof_regs if we need an extra cycle to rampup
  // while using REFLECT101 boundary padding.
  ac_array<bool, AC_SHIFT_HEIGHT, NUM_SHIFTS_FILL + EXTRA_WORD> sof_regs, eof_regs;
  bool eof_old, eol_old;
};

#endif
