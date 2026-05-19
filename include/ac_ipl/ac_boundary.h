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
//****************************************************************************
//  File : ac_boundary.h
//
//  Description:
//    Boundary handling for 1D and 2D AC Window 2.0 designs.
//    ac_boundary_1d takes care of horizontal boundary handling, i.e.
//    padding the left and right boundaries of the image, around the SOL and
//    EOL pixels. ac_boundary_2d uses an array of ac_boundary_1d objects to
//    handle horizontal boundary handling and then implements vertical
//    boundary handling to pad the top and bottom boundaries of the image.
//
//    ac_boundary_1d template parameters:
//    - T: Base type, i.e. type of pixels in the window.
//    - AC_WIN_WIDTH: Window width.
//    - AC_PMODE: Padding mode/method.
//    - AC_BUS_WORDS: Number of pixels passed on bus in parallel. Default
//        value is 1.
//    - HIGH_SPEED_REPLICATE: Switches to an AC_REPLICATE boundary padding
//        loop structure that can potentially let the design schedule at a
//        higher clock period without incurring a negative slack. Default
//        value is false.
//
//    ac_boundary_1d methods:
//    -   void run (
//          ac_array<T, AC_WORDS + EXTRA_WORD> din,
//          ac_int<AC_WORDS, false> sol_flags,
//          ac_int<AC_WORDS, false> eol_flags,
//          ac_array<T, AC_WORDS> &dout
//        ):
//        Class interface run function. Uses positional information supplied
//        by the *_flags registers to ascertain the image boundaries and
//        perform boundary processing on the din array. The output is written
//        to dout.
//    - void reset():
//        Resets the horizontal flag shifters to 0.
//    - void set_pval(T pval):
//        Sets the padded_val data member to the pval input.
//    - ac_boundary_1d():
//        Default constructor. Sets padded_val to zero using set_pval().
//    - ac_boundary_1d(T pval):
//        Parameterized constructor. Sets padded_val to the pval input
//        using set_pval.
//
//    ac_boundary_1d has a nested struct called bhandling_struct. This
//    struct is specialized for different padding methods and architectures.
//    Each specialized struct has the handle() method which actually handles
//    the relevant horizontal boundary processing.
//
//    What the handle() method does is different for different padding
//    methods, but the method signature is the same regardless:
//    void handle(ac_array<T, AC_WORDS + EXTRA_WORD> din, ac_int<AC_WORDS, false> sol_flags, ac_int<AC_WORDS, false> eol_flags, T padded_val, ac_array<T, AC_WORDS> &dout_temp).
//
//    The parameters for this method are the same as those used in the run()
//    function, with the exception of the "padded_val" parameter: this one
//    is added to pass over the padded pixel value for AC_CONSTANT handling.
//    If AC_CONSTANT handling is not used, the padded_val parameter is
//    ignored. Also, the output array is called dout_temp instead of dout,
//    because it temporarily stores the output window before it's written
//    to the run() function output.
//
//    ac_boundary_2d template parameters:
//    - T: Base type, i.e. type of pixels in the window.
//    - AC_WIN_HEIGHT: Window height.
//    - AC_WIN_WIDTH: Window width.
//    - AC_PMODE: Padding mode/method.
//    - AC_BUS_WORDS: Number of pixels passed on bus in parallel. Default
//        value is 1.
//    - HIGH_SPEED_REPLICATE: Switches to an AC_REPLICATE boundary padding
//        loop structure that can potentially let the design schedule at a
//        higher clock period without incurring a negative slack. Default
//        value is false.
//
//    ac_boundary_2d methods:
//    -   void run(
//          ac_array<T, AC_SHIFT_HEIGHT, AC_WORDS + EXTRA_WORD> din,
//          ac_int<AC_WIN_HEIGHT, false> sof_flags,
//          ac_int<AC_WIN_HEIGHT, false> eof_flags,
//          ac_int<AC_WORDS, false> sol_flags,
//          ac_int<AC_WORDS, false> eol_flags,
//          ac_array<T, AC_WIN_HEIGHT, AC_WORDS> &dout
//        ):
//        Class interface run function. Uses positional information supplied
//        by the *_flags registers to ascertain the image boundaries and
//        perform boundary processing on the din array. The output is written
//        to dout.
//    - void set_pval(T pval):
//        Sets the padded_val data member to the pval input.
//    - ac_boundary_2d():
//        Default constructor. Sets padded_val to zero using set_pval.
//    - ac_boundary_2d(T pval):
//        Parameterized constructor. Sets padded_val to the pval input
//        using set_pval.
//
//    ac_boundary_2d has a nested struct called bhandling_struct. Just like
//    in ac_boundary_1d, this struct is specialized for different padding
//    methods and architectures and has the boundary processing method handle().
//
//    The method signature for handle() is as follows:
//    void handle(ac_array<T, AC_SHIFT_HEIGHT, AC_WORDS> lines_out, ac_int<AC_WIN_HEIGHT, false> sof_flags, ac_int<AC_WIN_HEIGHT, false> eof_flags, T padded_val, ac_array<T, AC_WIN_HEIGHT, AC_WORDS> &dout_temp).
//
//    The "lines_out" input is the output from each of the lines of the
//    window after horizontal boundary processing has been done. The
//    sof_flags and eof_flags inputs supply the positional information that
//    lets the boundary handling know where the top and bottom of the image
//    are, respectively.
//
//****************************************************************************

#ifndef _INCLUDED_AC_BOUNDARY_H_
#define _INCLUDED_AC_BOUNDARY_H_

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

template <typename T, int AC_WIN_WIDTH, ac_padding_method AC_PMODE, int AC_BUS_WORDS = 1, bool HIGH_SPEED_REPLICATE = false>
class ac_boundary_1d
{
public:
  static_assert(AC_WIN_WIDTH > 0, "AC_WIN_WIDTH must be positive.");
  static_assert(AC_BUS_WORDS > 0, "AC_BUS_WORDS must be positive.");

  typedef calc_shifter_words<AC_WIN_WIDTH, AC_PMODE, AC_BUS_WORDS> cswords_type;

  enum {
    AC_WORDS = cswords_type::AC_WORDS,
    EXTRA_WORD = cswords_type::EXTRA_WORD,
    LPAD_WORDS = cswords_type::LPAD_WORDS,
    RPAD_WORDS = cswords_type::RPAD_WORDS,
    upp_mid_idx = AC_WORDS - RPAD_WORDS - 1,
  };

  void run (
    ac_array<T, AC_WORDS + EXTRA_WORD> din,
    ac_int<AC_WORDS, false> sol_flags,
    ac_int<AC_WORDS, false> eol_flags,
    ac_array<T, AC_WORDS> &dout
  ) {
    ac_array<T, AC_WORDS> dout_temp;
    constexpr bool DUMMY = false;
    bhandling_struct<DUMMY, HIGH_SPEED_REPLICATE, AC_PMODE> bhandling;
    bhandling.handle(din, sol_flags, eol_flags, padded_val, dout_temp);
    dout = dout_temp;
  }

  void set_pval(T pval) {
    // Set value to be padded at boundary regions, for when AC_CONSTANT is the padding method.
    padded_val = pval;
  }

  ac_boundary_1d() {
    set_pval(0);
  }

  ac_boundary_1d(T pval) {
    set_pval(pval);
  }

private:
  // Generic struct, will be specialized later.
  // The DUMMY template parameter is added to make sure that any specialized struct is always partially
  // specialized. Since the boundary handling struct is nested inside the ac_boundary class, fully
  // specializing it will result in an error.
  template <bool DUMMY, bool HIGH_SPEED_REPLICATE_, ac_padding_method AC_PMODE_>
  struct bhandling_struct { };
  
  // No boundary padding.
  template <bool DUMMY, bool HIGH_SPEED_REPLICATE_>
  struct bhandling_struct <DUMMY, HIGH_SPEED_REPLICATE_, AC_NO_PADDING> {
    void handle(ac_array<T, AC_WORDS + EXTRA_WORD> din, ac_int<AC_WORDS, false> sol_flags, ac_int<AC_WORDS, false> eol_flags, T padded_val, ac_array<T, AC_WORDS> &dout_temp) {
      dout_temp = din; // Note that EXTRA_WORD = 0.
    }
  };

  // AC_CONSTANT boundary padding.
  template <bool DUMMY, bool HIGH_SPEED_REPLICATE_>
  struct bhandling_struct <DUMMY, HIGH_SPEED_REPLICATE_, AC_CONSTANT> {
    void handle(ac_array<T, AC_WORDS + EXTRA_WORD> din, ac_int<AC_WORDS, false> sol_flags, ac_int<AC_WORDS, false> eol_flags, T padded_val, ac_array<T, AC_WORDS> &dout_temp) {
      // Middle of window: No boundary handling.
      #pragma hls_unroll yes
      MIDDLE_WINDOW_1D: for (int i = LPAD_WORDS; i <= upp_mid_idx; i++) {
        dout_temp[i] = din[i];
      }

      sol_handle(din, sol_flags, padded_val, dout_temp);
      eol_handle(din, eol_flags, padded_val, dout_temp);
    }

    private:
    #pragma CCOV_EXCL yes
    void sol_handle(ac_array<T, AC_WORDS + EXTRA_WORD> din, ac_int<AC_WORDS, false> sol_flags, T padded_val, ac_array<T, AC_WORDS> &dout_temp) {
      ac_int<1, false> sol_seen = 0;
      // Left side of window: SOL boundary handling.
      #pragma hls_waive CCC
      #pragma hls_unroll yes
      SOL_BOUND_HANDLING: for (int i = LPAD_WORDS - 1; i >= 0; i--) {
        sol_seen |= sol_flags[i + 1];
        dout_temp[i] = sol_seen ? padded_val : din[i];
      }
    }

    #pragma CCOV_EXCL yes
    void eol_handle(ac_array<T, AC_WORDS + EXTRA_WORD> din, ac_int<AC_WORDS, false> eol_flags, T padded_val, ac_array<T, AC_WORDS> &dout_temp) {
      ac_int<1, false> eol_seen = 0;
      // Right side of window: EOL boundary handling.
      #pragma hls_waive CCC
      #pragma hls_unroll yes
      EOL_BOUND_HANDLING: for (int i = upp_mid_idx + 1; i < AC_WORDS; i++) {
        eol_seen |= eol_flags[i - 1];
        dout_temp[i] = eol_seen ? padded_val : din[i];
      }
    }
  };

  // AC_REPLICATE boundary padding, HIGH_SPEED_REPLICATE = false.
  template <bool DUMMY>
  struct bhandling_struct<DUMMY, false, AC_REPLICATE> {
    void handle(ac_array<T, AC_WORDS + EXTRA_WORD> din, ac_int<AC_WORDS, false> sol_flags, ac_int<AC_WORDS, false> eol_flags, T padded_val, ac_array<T, AC_WORDS> &dout_temp) {
      // Middle of window: No boundary handling.
      #pragma hls_unroll yes
      MIDDLE_WINDOW_1D: for (int i = LPAD_WORDS; i <= upp_mid_idx; i++) {
        dout_temp[i] = din[i];
      }
      sol_handle(din, sol_flags, dout_temp);
      eol_handle(din, eol_flags, dout_temp);
    }

    private:
    #pragma CCOV_EXCL yes
    void sol_handle(ac_array<T, AC_WORDS + EXTRA_WORD> din, ac_int<AC_WORDS, false> sol_flags, ac_array<T, AC_WORDS> &dout_temp) {
      ac_int<1, false> sol_seen = 0;
      // Left side of window: SOL boundary handling.
      #pragma hls_waive CCC
      #pragma hls_unroll yes
      SOL_BOUND_HANDLING: for (int i = LPAD_WORDS - 1; i >= 0; i--) {
        int diff_idx = LPAD_WORDS - (i + 1);
        // The flag shifter shifts by AC_BUS_WORDS each clock cycle. As a result, we
        // can skip checking for SOL flags at certain indices of the flag shifter
        // and thereby save on hardware.
        sol_seen |= (diff_idx%AC_BUS_WORDS == 0) && sol_flags[i + 1];
        dout_temp[i] = sol_seen ? dout_temp[i + 1] : din[i];
      }
    }

    #pragma CCOV_EXCL yes
    void eol_handle(ac_array<T, AC_WORDS + EXTRA_WORD> din, ac_int<AC_WORDS, false> eol_flags, ac_array<T, AC_WORDS> &dout_temp) {
      ac_int<1, false> eol_seen = 0;
      // Right side of window: EOL boundary handling.
      #pragma hls_waive CCC
      #pragma hls_unroll yes
      EOL_BOUND_HANDLING: for (int i = upp_mid_idx + 1; i < AC_WORDS; i++) {
        int diff_idx = (i - 1) - upp_mid_idx;
        // The flag shifter shifts by AC_BUS_WORDS each clock cycle. As a result, we
        // can skip checking for EOL flags at certain indices of the flag shifter
        // and thereby save on hardware.
        eol_seen |= (diff_idx%AC_BUS_WORDS == 0) && eol_flags[i - 1];
        dout_temp[i] = eol_seen ? dout_temp[i - 1] : din[i];
      }
    }
  };

  // AC_REPLICATE boundary padding, HIGH_SPEED_REPLICATE = true.
  template <bool DUMMY>
  struct bhandling_struct<DUMMY, true, AC_REPLICATE> {
    void handle(ac_array<T, AC_WORDS + EXTRA_WORD> din, ac_int<AC_WORDS, false> sol_flags, ac_int<AC_WORDS, false> eol_flags, T padded_val, ac_array<T, AC_WORDS> &dout_temp) {
      // Middle of window: No boundary handling.
      #pragma hls_unroll yes
      MIDDLE_WINDOW_TWO_LOOP_1D: for (int i = LPAD_WORDS; i <= upp_mid_idx; i++) {
        dout_temp[i] = din[i];
      }

      T boundary_val = 0;
      sol_handle(din, sol_flags, boundary_val, dout_temp);
      eol_handle(din, eol_flags, boundary_val, dout_temp);
    }

    private:
    #pragma CCOV_EXCL yes
    void sol_handle(ac_array<T, AC_WORDS + EXTRA_WORD> din, ac_int<AC_WORDS, false> sol_flags, T &boundary_val, ac_array<T, AC_WORDS> &dout_temp) {
      // Left side of window: SOL boundary handling.
      #pragma hls_waive CCC
      #pragma hls_unroll yes
      SOL_BOUND_HANDLING_I: for (int i = 0; i < LPAD_WORDS; i++) {
        ac_int<1, false> sol_seen = 0;
        // Since the flag shifter shifts by AC_BUS_WORDS each clock cycle, we decrement the
        // counter "j" by AC_BUS_WORDS instead of decrementing by 1, thereby only checking
        // indices where the flag value can be set to 1 and saving on hardware.
        #pragma hls_waive CCC
        #pragma hls_unroll yes
        SOL_BOUND_HANDLING_J: for (int j = LPAD_WORDS; j >= i + 1; j -= AC_BUS_WORDS) {
          sol_seen |= sol_flags[j];
          if (sol_flags[j]) {
            boundary_val = din[j];
          }
        }
        dout_temp[i] = sol_seen ? boundary_val : din[i];
      }
    }

    #pragma CCOV_EXCL yes
    void eol_handle(ac_array<T, AC_WORDS + EXTRA_WORD> din, ac_int<AC_WORDS, false> eol_flags, T &boundary_val, ac_array<T, AC_WORDS> &dout_temp) {
      // Right side of window: EOL boundary handling.
      #pragma hls_waive CCC
      #pragma hls_unroll yes
      EOL_BOUND_HANDLING_I: for (int i = upp_mid_idx + 1; i < AC_WORDS; i++) {
        ac_int<1, false> eol_seen = 0;
        // Since the flag shifter shifts by AC_BUS_WORDS each clock cycle, we increment the
        // counter "j" by AC_BUS_WORDS instead of incrementing by 1, thereby only checking
        // indices where the flag value can be set to 1 and saving on hardware.
        #pragma hls_waive CCC
        #pragma hls_unroll yes
        EOL_BOUND_HANDLING_J: for (int j = upp_mid_idx; j < i; j += AC_BUS_WORDS) {
          eol_seen |= eol_flags[j];
          if (eol_flags[j]) {
            boundary_val = din[j];
          }
        }
        dout_temp[i] = eol_seen ? boundary_val : din[i];
      }
    }
  };

  // AC_REFLECT101 boundary padding.
  template <bool DUMMY, bool HIGH_SPEED_REPLICATE_>
  struct bhandling_struct<DUMMY, HIGH_SPEED_REPLICATE_, AC_REFLECT101> {
    void handle(ac_array<T, AC_WORDS + EXTRA_WORD> din, ac_int<AC_WORDS, false> sol_flags, ac_int<AC_WORDS, false> eol_flags, T padded_val, ac_array<T, AC_WORDS> &dout_temp) {
      // Middle of window: No boundary handling.
      #pragma hls_unroll yes
      MIDDLE_WINDOW_TWO_LOOP_1D: for (int i = LPAD_WORDS; i <= upp_mid_idx; i++) {
        dout_temp[i] = din[i];
      }

      T boundary_val = 0;
      sol_handle(din, sol_flags, boundary_val, dout_temp);
      eol_handle(din, eol_flags, boundary_val, dout_temp);
    }
    
    private:
    #pragma CCOV_EXCL yes
    void sol_handle(ac_array<T, AC_WORDS + EXTRA_WORD> din, ac_int<AC_WORDS, false> sol_flags, T &boundary_val, ac_array<T, AC_WORDS> &dout_temp) {
      // Left side of window: SOL boundary handling.
      #pragma hls_waive CCC
      #pragma hls_unroll yes
      SOL_BOUND_HANDLING_I: for (int i = 0; i < LPAD_WORDS; i++) {
        ac_int<1, false> sol_seen = 0;
        // Since the flag shifter shifts by AC_BUS_WORDS each clock cycle, we decrement the
        // counter "j" by AC_BUS_WORDS instead of decrementing by 1, thereby only checking
        // indices where the flag value can be set to 1 and saving on hardware.
        #pragma hls_waive CCC
        #pragma hls_unroll yes
        SOL_BOUND_HANDLING_J: for (int j = LPAD_WORDS; j >= i + 1; j -= AC_BUS_WORDS) {
          sol_seen |= sol_flags[j];
          if (sol_flags[j]) {
            boundary_val = din[j*2 - i];
          }
        }
        dout_temp[i] = sol_seen ? boundary_val : din[i];
      }
    }

    #pragma CCOV_EXCL yes
    void eol_handle(ac_array<T, AC_WORDS + EXTRA_WORD> din, ac_int<AC_WORDS, false> eol_flags, T &boundary_val, ac_array<T, AC_WORDS> &dout_temp) {
      // Right side of window: EOL boundary handling.
      #pragma hls_waive CCC
      #pragma hls_unroll yes
      EOL_BOUND_HANDLING_I: for (int i = upp_mid_idx + 1; i < AC_WORDS; i++) {
        ac_int<1, false> eol_seen = 0;
        // Since the flag shifter shifts by AC_BUS_WORDS each clock cycle, we increment the
        // counter "j" by AC_BUS_WORDS instead of incrementing by 1, thereby only checking
        // indices where the flag value can be set to 1 and saving on hardware.
        #pragma hls_waive CCC
        #pragma hls_unroll yes
        EOL_BOUND_HANDLING_J: for (int j = upp_mid_idx; j < i; j += AC_BUS_WORDS) {
          eol_seen |= eol_flags[j];
          if (eol_flags[j]) {
            boundary_val = din[j*2 - i];
          }
        }
        dout_temp[i] = eol_seen ? boundary_val : din[i];
      }
    }
  };

  // AC_REFLECT boundary padding.
  template <bool DUMMY, bool HIGH_SPEED_REPLICATE_>
  struct bhandling_struct<DUMMY, HIGH_SPEED_REPLICATE_, AC_REFLECT> {
    void handle(ac_array<T, AC_WORDS + EXTRA_WORD> din, ac_int<AC_WORDS, false> sol_flags, ac_int<AC_WORDS, false> eol_flags, T padded_val, ac_array<T, AC_WORDS> &dout_temp) {
      // Middle of window: No boundary handling.
      #pragma hls_unroll yes
      MIDDLE_WINDOW_TWO_LOOP_1D: for (int i = LPAD_WORDS; i <= upp_mid_idx; i++) {
        dout_temp[i] = din[i];
      }

      T boundary_val = 0;
      sol_handle(din, sol_flags, boundary_val, dout_temp);
      eol_handle(din, eol_flags, boundary_val, dout_temp);
    }

    #pragma CCOV_EXCL yes
    void sol_handle(ac_array<T, AC_WORDS + EXTRA_WORD> din, ac_int<AC_WORDS, false> sol_flags, T &boundary_val, ac_array<T, AC_WORDS> &dout_temp) {
      // Left side of window: SOL boundary handling.
      #pragma hls_waive CCC
      #pragma hls_unroll yes
      SOL_BOUND_HANDLING_I: for (int i = 0; i < LPAD_WORDS; i++) {
        ac_int<1, false> sol_seen = 0;
        // Since the flag shifter shifts by AC_BUS_WORDS each clock cycle, we decrement the
        // counter "j" by AC_BUS_WORDS instead of decrementing by 1, thereby only checking
        // indices where the flag value can be set to 1 and saving on hardware.
        #pragma hls_waive CCC
        #pragma hls_unroll yes
        SOL_BOUND_HANDLING_J: for (int j = LPAD_WORDS; j >= i + 1; j -= AC_BUS_WORDS) {
          sol_seen |= sol_flags[j];
          if (sol_flags[j]) {
            boundary_val = din[j*2 - i - 1];
          }
        }
        dout_temp[i] = sol_seen ? boundary_val : din[i];
      }
    }

    #pragma CCOV_EXCL yes
    void eol_handle(ac_array<T, AC_WORDS + EXTRA_WORD> din, ac_int<AC_WORDS, false> eol_flags, T &boundary_val, ac_array<T, AC_WORDS> &dout_temp) {
      // Right side of window: EOL boundary handling.
      #pragma hls_waive CCC
      #pragma hls_unroll yes
      EOL_BOUND_HANDLING_I: for (int i = upp_mid_idx + 1; i < AC_WORDS; i++) {
        ac_int<1, false> eol_seen = 0;
        // Since the flag shifter shifts by AC_BUS_WORDS each clock cycle, we increment the
        // counter "j" by AC_BUS_WORDS instead of incrementing by 1, thereby only checking
        // indices where the flag value can be set to 1 and saving on hardware.
        #pragma hls_waive CCC
        #pragma hls_unroll yes
        EOL_BOUND_HANDLING_J: for (int j = upp_mid_idx; j < i; j += AC_BUS_WORDS) {
          eol_seen |= eol_flags[j];
          if (eol_flags[j]) {
            boundary_val = din[j*2 - i + 1];
          }
        }
        dout_temp[i] = eol_seen ? boundary_val : din[i];
      }
    }
  };

  T padded_val; // Padded pixel value for AC_CONSTANT boundary processing.
};

template <typename T, int AC_WIN_HEIGHT, int AC_WIN_WIDTH, ac_padding_method AC_PMODE, int AC_BUS_WORDS = 1, bool HIGH_SPEED_REPLICATE = false>
class ac_boundary_2d
{
public:
  static_assert(AC_WIN_HEIGHT > 0, "AC_WIN_HEIGHT must be positive.");
  static_assert(AC_WIN_WIDTH > 0, "AC_WIN_WIDTH must be positive.");
  static_assert(AC_BUS_WORDS > 0, "AC_BUS_WORDS must be positive.");

  typedef ac_boundary_1d<T, AC_WIN_WIDTH, AC_PMODE, AC_BUS_WORDS, HIGH_SPEED_REPLICATE> BOUND_1D_TYPE;

  typedef calc_shifter_height<AC_WIN_HEIGHT, AC_PMODE> csheight_type;

  enum {
    EXTRA_LINE = csheight_type::EXTRA_LINE,
    AC_SHIFT_HEIGHT = csheight_type::AC_SHIFT_HEIGHT,
    AC_WORDS = BOUND_1D_TYPE::AC_WORDS,
    EXTRA_WORD = BOUND_1D_TYPE::EXTRA_WORD,
    USING_PADDING = csheight_type::USING_PADDING,
    LPAD_WORDS = BOUND_1D_TYPE::LPAD_WORDS, // For external use.
    RPAD_WORDS = BOUND_1D_TYPE::RPAD_WORDS, // For external use.
    TPAD_LINES = csheight_type::TPAD_LINES,
    BPAD_LINES = csheight_type::BPAD_LINES,
    eof_start_idx = AC_WIN_HEIGHT - BPAD_LINES
  };

  void run(
    ac_array<T, AC_SHIFT_HEIGHT, AC_WORDS + EXTRA_WORD> din,
    ac_int<AC_WIN_HEIGHT, false> sof_flags,
    ac_int<AC_WIN_HEIGHT, false> eof_flags,
    ac_int<AC_WORDS, false> sol_flags,
    ac_int<AC_WORDS, false> eol_flags,
    ac_array<T, AC_WIN_HEIGHT, AC_WORDS> &dout
  ) {
    BOUND_1D_TYPE lines[AC_SHIFT_HEIGHT];
    // lines_out: Stores the horizontal boundary processing output from all the lines.
    ac_array<T, AC_SHIFT_HEIGHT, AC_WORDS> lines_out;

    #pragma hls_unroll yes
    COPY_LINE_OUT: for (int i = 0; i < AC_SHIFT_HEIGHT; i++) {
      lines[i].set_pval(padded_val); // Set constant padding value for each line.
      // Sent data and flag information to each line to get the horizontal boundary processing output.
      lines[i].run(din[i], sol_flags, eol_flags, lines_out[i]);
    }

    ac_array<T, AC_WIN_HEIGHT, AC_WORDS> dout_temp;

    if (USING_PADDING) { // If no padding is used, the handle() method in bhandling will take care of the middle of the window too.
      // Middle of the window: No boundary handling.
      dout_temp[TPAD_LINES] = lines_out[TPAD_LINES];
    }

    constexpr bool DUMMY = false;
    bhandling_struct<DUMMY, HIGH_SPEED_REPLICATE, AC_PMODE> bhandling;
    // Carry out vertical boundary handling.
    bhandling.handle(lines_out, sof_flags, eof_flags, padded_val, dout_temp);

    dout = dout_temp;
  }

  void set_pval(T pval) {
    // Set value to be padded at boundary regions, for when AC_CONSTANT is the padding method.
    padded_val = pval;
  }

  ac_boundary_2d() {
    set_pval(0);
  }

  ac_boundary_2d(T pval) {
    set_pval(pval);
  }

private:
  // Generic struct, will be specialized later.
  // The DUMMY template parameter is added to make sure that any specialized struct is always partially
  // specialized. Since the boundary handling struct is nested inside the ac_boundary class, fully
  // specializing it will result in an error.
  template <bool DUMMY, bool HIGH_SPEED_REPLICATE_, ac_padding_method AC_PMODE_>
  struct bhandling_struct { };
  
  // No boundary padding.
  template <bool DUMMY, bool HIGH_SPEED_REPLICATE_>
  struct bhandling_struct<DUMMY, HIGH_SPEED_REPLICATE_, AC_NO_PADDING> {
    void handle(
      ac_array<T, AC_SHIFT_HEIGHT, AC_WORDS> lines_out,
      ac_int<AC_WIN_HEIGHT, false> sof_flags,
      ac_int<AC_WIN_HEIGHT, false> eof_flags,
      T padded_val,
      ac_array<T, AC_WIN_HEIGHT, AC_WORDS> &dout_temp
    ) {
      dout_temp = lines_out; // Note that EXTRA_LINE = 0 and AC_SHIFT_HEIGHT = AC_WIN_HEIGHT.
    }
  };

  // AC_CONSTANT boundary padding.
  template <bool DUMMY, bool HIGH_SPEED_REPLICATE_>
  struct bhandling_struct<DUMMY, HIGH_SPEED_REPLICATE_, AC_CONSTANT> {
    void handle(
      ac_array<T, AC_SHIFT_HEIGHT, AC_WORDS> lines_out,
      ac_int<AC_WIN_HEIGHT, false> sof_flags,
      ac_int<AC_WIN_HEIGHT, false> eof_flags,
      T padded_val,
      ac_array<T, AC_WIN_HEIGHT, AC_WORDS> &dout_temp
    ) {
      ac_array<T, AC_WORDS> padded_val_line;

      #pragma hls_unroll yes
      PAD_LINE: for (int j = 0; j < AC_WORDS; j++) {
        // Fill the entire line with the constant padding value.
        // This is done to facilitate vertical boundary processing later.
        padded_val_line[j] = padded_val;
      }

      bool sof_seen = false;
      // Upper window: SOF boundary handling.
      #pragma hls_unroll yes
      SOF_BOUND_HANDLING: for (int i = TPAD_LINES - 1; i >= 0; i--) {
        if (sof_flags[i + 1]) {
          sof_seen = true;
        }
        dout_temp[i] = sof_seen ? padded_val_line : lines_out[i];
      }

      bool eof_seen = false;
      // Lower window: EOF boundary handling.
      #pragma hls_unroll yes
      EOF_BOUND_HANDLING: for (int i = eof_start_idx; i < AC_WIN_HEIGHT; i++) {
        if (eof_flags[i]) {
          eof_seen = true;
        }
        dout_temp[i] = eof_seen ? padded_val_line : lines_out[i];
      }
    }
  };

  // AC_REPLICATE boundary padding, HIGH_SPEED_REPLICATE = false.
  template <bool DUMMY>
  struct bhandling_struct<DUMMY, false, AC_REPLICATE> {
    void handle(
      ac_array<T, AC_SHIFT_HEIGHT, AC_WORDS> lines_out,
      ac_int<AC_WIN_HEIGHT, false> sof_flags,
      ac_int<AC_WIN_HEIGHT, false> eof_flags,
      T padded_val,
      ac_array<T, AC_WIN_HEIGHT, AC_WORDS> &dout_temp
    ) {
      bool sof_seen = false;
      // Upper window: SOF boundary handling.
      #pragma hls_unroll yes
      SOF_BOUND_HANDLING: for (int i = TPAD_LINES - 1; i >= 0; i--) {
        if (sof_flags[i + 1]) {
          sof_seen = true;
        }
        dout_temp[i] = sof_seen ? dout_temp[i + 1] : lines_out[i];
      }

      bool eof_seen = false;
      // Lower window: EOF boundary handling.
      #pragma hls_unroll yes
      EOF_BOUND_HANDLING: for (int i = eof_start_idx; i < AC_WIN_HEIGHT; i++) {
        if (eof_flags[i]) {
          eof_seen = true;
        }
        dout_temp[i] = eof_seen ? dout_temp[i - 1] : lines_out[i];
      }
    }
  };

  // AC_REPLICATE boundary padding, HIGH_SPEED_REPLICATE = true.
  template <bool DUMMY>
  struct bhandling_struct<DUMMY, true, AC_REPLICATE> {
    void handle(
      ac_array<T, AC_SHIFT_HEIGHT, AC_WORDS> lines_out,
      ac_int<AC_WIN_HEIGHT, false> sof_flags,
      ac_int<AC_WIN_HEIGHT, false> eof_flags,
      T padded_val,
      ac_array<T, AC_WIN_HEIGHT, AC_WORDS> &dout_temp
    ) {
      ac_array<T, AC_WORDS> boundary_val;
      #pragma hls_unroll yes
      INIT_BOUNDARY_VAL_ARRAY: for (int i = 0; i < AC_WORDS; i++) {
        boundary_val[i] = 0;
      }

      // Upper window: SOF boundary handling.
      #pragma hls_unroll yes
      SOF_BOUND_HANDLING_I: for (int i = 0; i < TPAD_LINES; i++) {
        ac_int<1, false> sof_seen = 0;
        #pragma hls_unroll yes
        SOF_BOUND_HANDLING_J: for (int j = i + 1; j < TPAD_LINES + 1; j++) {
          sof_seen |= sof_flags[j];
          if (sof_flags[j]) {
            boundary_val = lines_out[j];
          }
        }
        dout_temp[i] = sof_seen ? boundary_val : lines_out[i];
      }

      // Lower window: EOF boundary handling.
      #pragma hls_unroll yes
      EOF_BOUND_HANDLING_I: for (int i = eof_start_idx; i < AC_WIN_HEIGHT; i++) {
        ac_int<1, false> eof_seen = 0;
        #pragma hls_unroll yes
        EOF_BOUND_HANDLING_J: for (int j = eof_start_idx; j <= i; j++) {
          eof_seen |= eof_flags[j];
          if (eof_flags[j]) {
            boundary_val = lines_out[j - 1];
          }
        }
        dout_temp[i] = eof_seen ? boundary_val : lines_out[i];
      }
    }
  };

  // AC_REFLECT101 boundary padding.
  template <bool DUMMY, bool HIGH_SPEED_REPLICATE_>
  struct bhandling_struct<DUMMY, HIGH_SPEED_REPLICATE_, AC_REFLECT101> {
    void handle(
      ac_array<T, AC_SHIFT_HEIGHT, AC_WORDS> lines_out,
      ac_int<AC_WIN_HEIGHT, false> sof_flags,
      ac_int<AC_WIN_HEIGHT, false> eof_flags,
      T padded_val,
      ac_array<T, AC_WIN_HEIGHT, AC_WORDS> &dout_temp
    ) {
      ac_array<T, AC_WORDS> boundary_val;
      #pragma hls_unroll yes
      INIT_BOUNDARY_VAL_ARRAY: for (int i = 0; i < AC_WORDS; i++) {
        boundary_val[i] = 0;
      }

      // Upper window: SOF boundary handling.
      #pragma hls_unroll yes
      SOF_BOUND_HANDLING_I: for (int i = 0; i < TPAD_LINES; i++) {
        ac_int<1, false> sof_seen = 0;
        #pragma hls_unroll yes
        SOF_BOUND_HANDLING_J: for (int j = i + 1; j < TPAD_LINES + 1; j++) {
          sof_seen |= sof_flags[j];
          if (sof_flags[j]) {
            boundary_val = lines_out[j*2 - i];
          }
        }
        dout_temp[i] = sof_seen ? boundary_val : lines_out[i];
      }

      // Lower window: EOF boundary handling.
      #pragma hls_unroll yes
      EOF_BOUND_HANDLING_I: for (int i = eof_start_idx; i < AC_WIN_HEIGHT; i++) {
        ac_int<1, false> eof_seen = 0;
        #pragma hls_unroll yes
        EOF_BOUND_HANDLING_J: for (int j = eof_start_idx; j <= i; j++) {
          eof_seen |= eof_flags[j];
          if (eof_flags[j]) {
            boundary_val = lines_out[(j - 1)*2 - i];
          }
        }
        dout_temp[i] = eof_seen ? boundary_val : lines_out[i];
      }
    }
  };

  // AC_REFLECT boundary padding.
  template <bool DUMMY, bool HIGH_SPEED_REPLICATE_>
  struct bhandling_struct<DUMMY, HIGH_SPEED_REPLICATE_, AC_REFLECT> {
    void handle(
      ac_array<T, AC_SHIFT_HEIGHT, AC_WORDS> lines_out,
      ac_int<AC_WIN_HEIGHT, false> sof_flags,
      ac_int<AC_WIN_HEIGHT, false> eof_flags,
      T padded_val,
      ac_array<T, AC_WIN_HEIGHT, AC_WORDS> &dout_temp
    ) {
      ac_array<T, AC_WORDS> boundary_val;
      #pragma hls_unroll yes
      INIT_BOUNDARY_VAL_ARRAY: for (int i = 0; i < AC_WORDS; i++) {
        boundary_val[i] = 0;
      }

      // Upper window: SOF boundary handling.
      #pragma hls_unroll yes
      SOF_BOUND_HANDLING_I: for (int i = 0; i < TPAD_LINES; i++) {
        ac_int<1, false> sof_seen = 0;
        #pragma hls_unroll yes
        SOF_BOUND_HANDLING_J: for (int j = i + 1; j < TPAD_LINES + 1; j++) {
          sof_seen |= sof_flags[j];
          if (sof_flags[j]) {
            boundary_val = lines_out[j*2 - i - 1];
          }
        }
        dout_temp[i] = sof_seen ? boundary_val : lines_out[i];
      }

      // Lower window: EOF boundary handling.
      #pragma hls_unroll yes
      EOF_BOUND_HANDLING_I: for (int i = eof_start_idx; i < AC_WIN_HEIGHT; i++) {
        ac_int<1, false> eof_seen = 0;
        #pragma hls_unroll yes
        EOF_BOUND_HANDLING_J: for (int j = eof_start_idx; j <= i; j++) {
          eof_seen |= eof_flags[j];
          if (eof_flags[j]) {
            boundary_val = lines_out[(j - 1)*2 - i + 1];
          }
        }
        dout_temp[i] = eof_seen ? boundary_val : lines_out[i];
      }
    }
  };

  T padded_val;
};

#endif
