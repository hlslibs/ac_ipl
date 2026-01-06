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
#ifndef _INCLUDE_AC_BOUNDARY_ENUMS_H_
#define _INCLUDE_AC_BOUNDARY_ENUMS_H_

enum ac_padding_method {
  AC_NO_PADDING,
  AC_REPLICATE,
  AC_REFLECT101,
  AC_CONSTANT,
  AC_REFLECT
};

template <int AC_WIN_WIDTH, ac_padding_method AC_PMODE, int AC_BUS_WORDS>
struct calc_shifter_words {
  enum {
    USING_PADDING = AC_PMODE != AC_NO_PADDING,
    USING_REFLECT101 = AC_PMODE == AC_REFLECT101,
    AC_WORDS = AC_WIN_WIDTH + AC_BUS_WORDS - 1,
    // LPAD_WORDS and RPAD_WORDS: Number of words padded on the left and right
    #ifdef AC_WIN_V2_REVERSE_PADDING
    LPAD_WORDS = USING_PADDING ? AC_WIN_WIDTH/2 - int(AC_WIN_WIDTH%2 == 0) : 0,
    RPAD_WORDS = USING_PADDING ? AC_WIN_WIDTH/2 : 0,
    #else
    LPAD_WORDS = USING_PADDING ? AC_WIN_WIDTH/2 : 0,
    RPAD_WORDS = USING_PADDING ? AC_WIN_WIDTH/2 - int(AC_WIN_WIDTH%2 == 0) : 0,
    #endif
    // AC_WORDS_INSIDE = Number of words that are inside the image and not inside the boundary region at the moment when the window ramps up horizontally.
    AC_WORDS_INSIDE = AC_WORDS - LPAD_WORDS,
    // Number of shifts required to fill up the shift register for horizontal rampup = ceil(AC_WORDS_INSIDE/AC_BUS_WORDS)
    NUM_SHIFTS_FILL = AC_WORDS_INSIDE/AC_BUS_WORDS + int(AC_WORDS_INSIDE%AC_BUS_WORDS != 0),
    // If the window has an even width, you're using REFLECT101 and if you're passing a pixel at time, you'll need an extra word in the shift register
    // to ensure correct boundary processing later.
    EXTRA_WORD = USING_REFLECT101 && AC_WIN_WIDTH%2 == 0 && AC_BUS_WORDS == 1,
    AC_SHIFT_WORDS = NUM_SHIFTS_FILL*AC_BUS_WORDS + LPAD_WORDS + EXTRA_WORD
  };
};

template <int AC_WIN_HEIGHT, ac_padding_method AC_PMODE>
struct calc_shifter_height {
  enum {
    USING_PADDING = AC_PMODE != AC_NO_PADDING,
    USING_REFLECT101 = AC_PMODE == AC_REFLECT101,
    EXTRA_LINE = USING_REFLECT101 ? int(AC_WIN_HEIGHT%2 == 0) : 0,
    // Add an extra line to the shift registers if using REFLECT101 and if the window height is even.
    AC_SHIFT_HEIGHT = AC_WIN_HEIGHT + EXTRA_LINE,
    #ifdef AC_WIN_V2_REVERSE_PADDING
    TPAD_LINES = USING_PADDING ? AC_WIN_HEIGHT/2 - int(AC_WIN_HEIGHT%2 == 0) : 0,
    BPAD_LINES = USING_PADDING ? AC_WIN_HEIGHT/2 : 0,
    #else
    TPAD_LINES = USING_PADDING ? AC_WIN_HEIGHT/2 : 0,
    BPAD_LINES = USING_PADDING ? AC_WIN_HEIGHT/2 - int(AC_WIN_HEIGHT%2 == 0) : 0,
    #endif
  };
}; 

#endif
