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
//  File : ac_shift_N.h
//
//  Description:
//    Data shifters for 1D and 2D AC Window 2.0 designs. ac_shift_N_1d handles
//    shifting in one dimension, by one or more words per clock cycle.
//    ac_shift_N_2d instantiates multiple ac_shift_N_1d objects to serve as
//    horizontal shift registers. Together, these ac_shift_N_1D objects make
//    up all the lines of the 2D shift registers.
//
//    ac_shift_N_1d template parameters:
//    - T: Base type, i.e. type of each element of the shift register.
//    - AC_WIN_WIDTH: Window width.
//    - AC_PMODE: Padding mode/method.
//    - AC_BUS_WORDS: Number of pixels passed on bus in parallel. Default
//        value is 1.
//
//    ac_shift_N_1d methods:
//    - void operator >> (ac_array<T, AC_BUS_WORDS> din):
//        Right-shift operator overloading. Right-Shifts din into the
//        horizontal shift register.
//    - void rshift(ac_array<T, AC_BUS_WORDS> din, ac_array<T, AC_WORDS + EXTRA_WORD> &dout):
//        Uses right-shift operator overloading to shift din into the shift
//        register. Also writes the first (AC_WORDS + EXTRA_WORD) elements
//        of thie shift register to dout. This function is used by the
//        AC Window 2.0 design.
//    - void operator << (ac_array<T, AC_BUS_WORDS> din):
//        Left-shift operator overloading. Left-shifts din into the
//        horizontal shift register.
//    - void lshift(ac_array<T,AC_BUS_WORDS> din, ac_array<T, AC_WORDS + EXTRA_WORD> &dout):
//        Uses left-shift operator overloading to shift din into the shift
//        register. Also writes the last (AC_WORDS + EXTRA_WORD) elements
//        of thie shift register to dout.
//    - T &operator[] (unsigned idx):
//        Subscript operator overloading. Returns a reference of the
//        shift-register element at index "idx"
//    - void reset():
//        Resets all elements of the shift register to zero.
//    - ac_shift_N_1d():
//        Default constructor. Calls the reset() function.
//
//    ac_shift_N_2d template parameters:
//    - T: Base type, i.e. type of each element of the shift register.
//    - AC_WIN_HEIGHT: Window height.
//    - AC_WIN_WIDTH: Window width.
//    - AC_PMODE: Padding mode/method.
//    - AC_BUS_WORDS: Number of pixels passed on bus in parallel. Default
//        value is 1.
//
//    ac_shift_N_2d methods:
//    - void operator >> (ac_array<T, AC_SHIFT_HEIGHT, AC_BUS_WORDS> din):
//        Iteratively right-shifts all lines of din into the horizontal shift
//        registers.
//    - void rshift(ac_array<T, AC_SHIFT_HEIGHT, AC_BUS_WORDS> din, ac_array<T, AC_SHIFT_HEIGHT, AC_WORDS + EXTRA_WORD>& dout):
//        Iteratively right-shifts all lines of din into the horizontal shift
//        registers and writes the first (AC_WORDS + EXTRA_WORD) elements of
//        each horizontal register to the corresponding line of dout.
//    - void operator << (ac_array<T, AC_SHIFT_HEIGHT, AC_BUS_WORDS> din):
//        Iteratively left-shifts all lines of din into the horizontal shift
//        registers.
//    - void lshift(ac_array<T, AC_SHIFT_HEIGHT, AC_BUS_WORDS> din, ac_array<T, AC_SHIFT_HEIGHT, AC_WORDS + EXTRA_WORD>& dout):
//        Iteratively left-shifts all lines of din into the horizontal shift
//        registers and writes the last (AC_WORDS + EXTA_WORDS) elements of
//        each horizontal shift register to the corresponding line of dout.
//    - LINE_TYPE &operator[] (unsigned idx):
//        Returns a reference of the horizontal shift register object at
//        index "idx". Can be used in combination with the ac_shift_N_1d
//        operator to access individual elements of the 2D shift register.
//    - void reset():
//        Iteratively calls the reset function on all of the horizontal shift
//        registers, and sets all shifter elements to zero.
//    - ac_shift_N_2d():
//        Default constructor. Calls the reset() function.
//
//****************************************************************************

#ifndef _INCLUDED_AC_SHIFT_N_H_
#define _INCLUDED_AC_SHIFT_N_H_

#include <ac_array.h>
#include <ac_int.h> // So that you can use AC_ASSERT.

#include "ac_boundary_enums.h"

// The code uses static assertions, which are only supported by C++11 or later compiler
// standards. Hence, the user should be informed if they are not using those standards.
#if (defined(__GNUC__) && (__cplusplus < 201103L))
#error Please use C++11 or a later standard for compilation.
#endif
#if (defined(_MSC_VER) && (_MSC_VER < 1920) && !defined(__EDG__))
#error Please use Microsoft VS 2019 or a later standard for compilation.
#endif

template<typename T, int AC_WIN_WIDTH, ac_padding_method AC_PMODE, int AC_BUS_WORDS = 1>
class ac_shift_N_1d
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

  void operator >> (ac_array<T, AC_BUS_WORDS> din) {
    // CCOV_EXCL_START
    // Shift registers by AC_BUS_WORDS every time you call the >> operator.
    #pragma hls_waive CCC
    #pragma hls_unroll yes
    OPERATOR_RSHIFT_1D: for (int j = 0; j < AC_SHIFT_WORDS - AC_BUS_WORDS; j++) {
      regs[j] = regs[j + AC_BUS_WORDS];
    }
    // CCOV_EXCL_STOP

    // Feed inputs to the last AC_BUS_WORDS elements.
    #pragma hls_unroll yes
    OPERATOR_RSHIFT_INPUT_1D: for (int j = AC_SHIFT_WORDS - AC_BUS_WORDS; j < AC_SHIFT_WORDS; j++) {
      regs[j] = din[j - (AC_SHIFT_WORDS - AC_BUS_WORDS)];
    }
  }

  void rshift(ac_array<T, AC_BUS_WORDS> din, ac_array<T, AC_WORDS + EXTRA_WORD> &dout) {
    operator >> (din);

    #pragma hls_unroll yes
    RSHIFT_CPY_1D: for (int j = 0; j < AC_WORDS + EXTRA_WORD; j++) {
      dout[j] = regs[j];
    }
  }

  void operator << (ac_array<T, AC_BUS_WORDS> din) {
    // CCOV_EXCL_START
    // Shift registers by AC_BUS_WORDS every time you call the << operator.
    #pragma hls_waive CCC
    #pragma hls_unroll yes
    OPERATOR_LSHIFT_1D: for (int j = AC_SHIFT_WORDS - 1; j >= AC_BUS_WORDS; j--) {
      regs[j] = regs[j - AC_BUS_WORDS];
    }
    // CCOV_EXCL_STOP

    // Feed inputs to the first AC_BUS_WORDS elements.
    #pragma hls_unroll yes
    OPERATOR_LSHIFT_INPUT_1D: for (int j = AC_BUS_WORDS - 1; j >= 0; j--) {
      regs[j] = din[AC_BUS_WORDS - 1 - j];
    }
  }

  void lshift(ac_array<T,AC_BUS_WORDS> din, ac_array<T, AC_WORDS + EXTRA_WORD> &dout) {
    operator << (din);

    #pragma hls_unroll yes
    LSHIFT_CPY_1D:for (int j = 0; j < AC_WORDS + EXTRA_WORD; j++) {
      dout[j] = regs[j + (AC_SHIFT_WORDS - AC_WORDS - EXTRA_WORD)];
    }
  }

  T &operator[] (unsigned idx) {
    AC_ASSERT(idx < AC_SHIFT_WORDS, "Out of bounds shift register access.");
    return regs[idx];
  }

  void reset() {
    #pragma hls_unroll yes
    RESET_1D:for (int i = 0; i < AC_SHIFT_WORDS; i++) {
      regs[i] = 0;
    }
  }

  ac_shift_N_1d() {
    UNINIT_REGS: for (int i = 0; i < AC_SHIFT_WORDS; i++) {
      // "Initialize" all elements of the shift register to an uninitialized value.
      T uninit_val;
      regs[i] = uninit_val;
    }
  }

private:
  ac_array<T, AC_SHIFT_WORDS> regs;
};

template<typename T, int AC_WIN_HEIGHT, int AC_WIN_WIDTH, ac_padding_method AC_PMODE, int AC_BUS_WORDS = 1>
class ac_shift_N_2d
{
public:
  static_assert(AC_WIN_HEIGHT > 0, "AC_WIN_HEIGHT must be positive.");
  static_assert(AC_WIN_WIDTH > 0, "AC_WIN_WIDTH must be positive.");
  static_assert(AC_BUS_WORDS > 0, "AC_BUS_WORDS must be positive.");

  typedef ac_shift_N_1d<T, AC_WIN_WIDTH, AC_PMODE, AC_BUS_WORDS> LINE_TYPE;
  typedef calc_shifter_height<AC_WIN_HEIGHT, AC_PMODE> csheight_type;

  enum {
    EXTRA_LINE = csheight_type::EXTRA_LINE,
    AC_SHIFT_HEIGHT = csheight_type::AC_SHIFT_HEIGHT,
    AC_WORDS = LINE_TYPE::AC_WORDS,
    EXTRA_WORD = LINE_TYPE::EXTRA_WORD,
    NUM_SHIFTS_FILL = LINE_TYPE::NUM_SHIFTS_FILL, // For external use, if needed.
    AC_SHIFT_WORDS = LINE_TYPE::AC_SHIFT_WORDS // For external use, if needed.
  };

  void operator >> (ac_array<T, AC_SHIFT_HEIGHT, AC_BUS_WORDS> din) {
    #pragma hls_unroll yes
    OPERATOR_RSHIFT_2D: for (int i = 0; i < AC_SHIFT_HEIGHT; i++) {
      lines[i] >> din[i];
    }
  }

  void rshift(ac_array<T, AC_SHIFT_HEIGHT, AC_BUS_WORDS> din, ac_array<T, AC_SHIFT_HEIGHT, AC_WORDS + EXTRA_WORD>& dout) {
    #pragma hls_unroll yes
    RSHIFT_2D: for (int i = 0; i < AC_SHIFT_HEIGHT; i++) {
      lines[i].rshift(din[i], dout[i]);
    }
  }

  void operator << (ac_array<T, AC_SHIFT_HEIGHT, AC_BUS_WORDS> din) {
    #pragma hls_unroll yes
    OPERATOR_LSHIFT_2D: for (int i = 0; i < AC_SHIFT_HEIGHT; i++) {
      lines[i] << din[i];
    }
  }

  void lshift(ac_array<T, AC_SHIFT_HEIGHT, AC_BUS_WORDS> din, ac_array<T, AC_SHIFT_HEIGHT, AC_WORDS + EXTRA_WORD>& dout) {
    #pragma hls_unroll yes
    LSHIFT_2D: for (int i = 0; i < AC_SHIFT_HEIGHT; i++) {
      lines[i].lshift(din[i], dout[i]);
    }
  }

  LINE_TYPE &operator[] (unsigned idx) {
    AC_ASSERT(idx < AC_SHIFT_HEIGHT, "Out of bounds shift register access.");
    return lines[idx];
  }

  void reset() {
    #pragma hls_unroll yes
    RESET_2D:for (int i = 0; i < AC_SHIFT_HEIGHT; i++) {
      lines[i].reset();
    }
  }

  ac_shift_N_2d() { }

private:
  LINE_TYPE lines[AC_SHIFT_HEIGHT];
};

#endif
