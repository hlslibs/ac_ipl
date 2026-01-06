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
//****************************************************************************
//  File : ac_window_v2.h
//
//  Description:
//    Top-level AC Window 2.0 design. Integrates all the AC Window 2.0
//    sub-blocks to carry out Standard windowing. ac_window_v2_1d handles 1D
//    windowing, while ac_window_v2_2d handles 2D windowing.
//
//    ac_window_v2_1d template parameters:
//    - PIX_TYPE: Pixel type.
//    - AC_IMG_WIDTH: Maximum possible image width.
//    - AC_WIN_WIDTH: Window width.
//    - AC_PMODE: Padding method/mode.
//    - AC_BUS_WORDS: Number of pixels passed on bus in parallel. Default
//        value is 1.
//    - HIGH_SPEED_REPLICATE: Template parameter used by boundary processing
//        class to switch between AC_REPLICATE architectures. Default value
//        is false.
//
//    ac_window_v2_1d methods:
//    -   void run(
//          ac_array<PIX_TYPE, AC_BUS_WORDS> din,
//          W_TYPE width,
//          bool &read_data,
//          ac_array<PIX_TYPE, AC_WORDS> &dout,
//          bool &sol_out,
//          bool &eol_out,
//          VOUT_TYPE &vld_out
//        ):
//        Class interface run function. din is the pixel input(s), width is
//        the image width, dout is the windowed output, *_out outputs are
//        the output flags.
//        VOUT_TYPE is ac_int<AC_BUS_WORDS, false>. If padding is used, vld_out
//        is either all-ones or all-zeros, depending on whether the window is valid or not.
//        If padding is not used, vld_out can be a combination of ones and zeros if the
//        overlapped window is at the end of the line and if some of the individual windows
//        aren't valid due to lack of padding.
//        read_data is an output flag which can be used to gate
//        against input reads while the window is flushing.
//    - void reset():
//        Calls reset() method of all the constituent blocks except for the
//        boundary processor (which is not instantiated as a class member but
//        instantiated local to the run() function).
//    - ac_window_v2_1d():
//        Default constructor. Calls the reset() method and sets the
//        padded_val data member to 0.
//    - ac_window_v2_1d(PIX_TYPE pval) : padded_val(pval):
//        Parameterized constructor. Calls the reset() method and sets the
//        padded_val data member to the pval input.
//
//    ac_window_v2_2d template parameters:
//    - PIX_TYPE: Pixel type.
//    - AC_IMG_HEIGHT: Maximum possible image height.
//    - AC_IMG_WIDTH: Maximum possible image width.
//    - AC_WIN_HEIGHT: Window height.
//    - AC_WIN_WIDTH: Window width.
//    - BUFF_TYPE: Buffer type.
//    - AC_PMODE: Padding method/mode.
//    - AC_BUS_WORDS: Number of pixels passed on bus in parallel. Default
//        value is 1.
//    - HIGH_SPEED_REPLICATE: Template parameter used by boundary processing
//        class to switch between AC_REPLICATE architectures. Default value
//        is false.
//    - AC_REPEAT: Enables line repetition. Setting this to true adds storage
//        for an extra line, among other architectural changes. Useful for
//        applications like image interpolation and resizing. Default value
//        is false.
//
//    ac_window_v2_2d methods:
//    - void run (
//        ac_array<PIX_TYPE, AC_BUS_WORDS> din,
//        W_TYPE width,
//        H_TYPE height,
//        bool &read_data,
//        ac_array<PIX_TYPE, AC_WIN_HEIGHT, AC_WORDS> &dout,
//        bool &sof_out,
//        bool &eof_out,
//        bool &sol_out,
//        bool &eol_out,
//        VOUT_TYPE &vld_out,
//        bool repeat_line = false
//      ):
//        Class interface run function. din is the pixel input(s), width and
//        height are the image width and height, respectively, dout is the
//        windowed output, *_out outputs are the output flags.
//        VOUT_TYPE is ac_int<AC_BUS_WORDS, false>. If padding is used, vld_out
//        is either all-ones or all-zeros, depending on whether the window is valid or not.
//        If padding is not used, vld_out can be a combination of ones and zeros if the
//        overlapped window is at the end of the line and if some of the windows aren't
//        valid due to lack of padding.
//        read_data is an output flag which can be used to gate
//        against input reads while the window is flushing.
//        repeat_line tells the window when to repeat over the same set of
//        lines. It is set to false by default (no line repetition) and can
//        only be set to true if AC_REPEAT = true.
//    - void reset_flags():
//        Resets various flags internal to the AC Window 2.0 design.
//        If AC_REPEAT = true, it is called implicitly.
//        If AC_REPEAT = false, the user must explicitly call the function
//        before they send in a new frame for processing.
//    - void reset():
//        Calls reset() method of all the constituent blocks except for the
//        boundary processor (which is not instantiated as a class member but
//        instantiated local to the run() function).
//    - ac_window_v2_2d():
//        Default constructor. Calls the reset() method and sets the
//        padded_val data member to 0.
//    - ac_window_v2_2d(PIX_TYPE pval) : padded_val(pval):
//        Parameterized constructor. Calls the reset() method and sets the
//        padded_val data member to the pval input.
//
//****************************************************************************

#ifndef _INCLUDED_AC_WINDOW_V2_H_
#define _INCLUDED_AC_WINDOW_V2_H_

#include "ac_linebuffer.h"
#include "ac_flag_gen.h"
#include "ac_flag_shift.h"
#include "ac_shift_N.h"
#include "ac_boundary.h"
#include "ac_flag_gen_out.h"

template <class PIX_TYPE, int AC_IMG_WIDTH, int AC_WIN_WIDTH, ac_padding_method AC_PMODE, int AC_BUS_WORDS = 1, bool HIGH_SPEED_REPLICATE = false>
class ac_window_v2_1d
{
public:
  typedef ac_int<ac::nbits<AC_IMG_WIDTH>::val, false> W_TYPE;
  typedef ac_int<AC_BUS_WORDS, false> VOUT_TYPE;
  typedef ac_boundary_1d<PIX_TYPE, AC_WIN_WIDTH, AC_PMODE, AC_BUS_WORDS, HIGH_SPEED_REPLICATE> BOUND_1D_TYPE;

  enum {
    AC_WORDS = BOUND_1D_TYPE::AC_WORDS,
    EXTRA_WORD = BOUND_1D_TYPE::EXTRA_WORD,
    USING_PADDING = AC_PMODE != AC_NO_PADDING
  };

  void run(
    ac_array<PIX_TYPE, AC_BUS_WORDS> din,
    W_TYPE width,
    bool &read_data,
    ac_array<PIX_TYPE, AC_WORDS> &dout,
    bool &sol_out,
    bool &eol_out,
    VOUT_TYPE &vld_out
  ) {
    // Call input flag generator.
    bool sol, eol;
    fgen_1d.run(width, sol, eol);

    // Call flag shifter.
    ac_int<AC_WORDS, false> sol_flags, eol_flags;
    fshift_1d.run(sol, eol, sol_flags, eol_flags);

    // Call data shifter.
    ac_array<PIX_TYPE, AC_WORDS + EXTRA_WORD> dshift_out;
    dshift_1d.rshift(din, dshift_out);

    // Call boundary processing.
    BOUND_1D_TYPE bound_1d(padded_val);
    bound_1d.run(dshift_out, sol_flags, eol_flags, dout);

    // Call output flag generator.
    bool eol_out_temp;
    fgen_out_1d.run(sol_flags, eol_flags, sol_out, eol_out_temp, vld_out);

    if (eol) {
      outside_image = true;
    }
    
    if (eol_out_temp) {
      outside_image = false;
      fgen_1d.reset();
      fshift_1d.reset();
    }
    
    read_data = !outside_image;
    
    eol_out = eol_out_temp;
  }

  void reset() {
    fgen_1d.reset();
    fshift_1d.reset();
    dshift_1d.reset();
    fgen_out_1d.reset();
    outside_image = false;
  }

  ac_window_v2_1d() : padded_val(0) {
    reset();
  }

  ac_window_v2_1d(PIX_TYPE pval) : padded_val(pval) {
    reset();
  }

private:
  ac_flag_gen_1d<AC_IMG_WIDTH, AC_BUS_WORDS> fgen_1d;
  ac_flag_shift_1d<AC_WIN_WIDTH, AC_PMODE, AC_BUS_WORDS> fshift_1d;
  ac_shift_N_1d<PIX_TYPE, AC_WIN_WIDTH, AC_PMODE, AC_BUS_WORDS> dshift_1d;
  ac_flag_gen_out_1d<AC_WIN_WIDTH, AC_BUS_WORDS, USING_PADDING, true> fgen_out_1d;

  PIX_TYPE padded_val;
  bool outside_image;
};

template <class PIX_TYPE, int AC_IMG_HEIGHT, int AC_IMG_WIDTH, int AC_WIN_HEIGHT, int AC_WIN_WIDTH, ac_buff_type BUFF_TYPE, ac_padding_method AC_PMODE, int AC_BUS_WORDS = 1, bool HIGH_SPEED_REPLICATE = false, bool AC_REPEAT = false>
class ac_window_v2_2d
{
public:
  typedef ac_int<ac::nbits<AC_IMG_WIDTH>::val, false> W_TYPE;
  typedef ac_int<ac::nbits<AC_IMG_HEIGHT>::val, false> H_TYPE;
  typedef ac_int<AC_BUS_WORDS, false> VOUT_TYPE;

  typedef ac_boundary_2d<PIX_TYPE, AC_WIN_HEIGHT, AC_WIN_WIDTH, AC_PMODE, AC_BUS_WORDS, HIGH_SPEED_REPLICATE> BOUND_2D_TYPE;
  
  enum {
    AC_WORDS = BOUND_2D_TYPE::AC_WORDS,
    EXTRA_WORD = BOUND_2D_TYPE::EXTRA_WORD,
    AC_SHIFT_HEIGHT = BOUND_2D_TYPE::AC_SHIFT_HEIGHT,
    LPAD_WORDS = BOUND_2D_TYPE::LPAD_WORDS, // For external use, if needed.
    RPAD_WORDS = BOUND_2D_TYPE::RPAD_WORDS, // For external use, if needed.
    TPAD_LINES = BOUND_2D_TYPE::TPAD_LINES, // For external use, if needed.
    BPAD_LINES = BOUND_2D_TYPE::BPAD_LINES, // For external use, if needed.
    USING_PADDING = AC_PMODE != AC_NO_PADDING
  };

  typedef ac_packed_vector<PIX_TYPE, AC_BUS_WORDS> LB_IN_TYPE;
  typedef ac_array<PIX_TYPE, AC_SHIFT_HEIGHT, AC_BUS_WORDS> LB_OUT_TYPE;
  typedef ac_linebuffer<LB_IN_TYPE, LB_OUT_TYPE, AC_IMG_WIDTH, AC_SHIFT_HEIGHT, BUFF_TYPE, AC_REPEAT> LB_TYPE;

  void run (
    ac_array<PIX_TYPE, AC_BUS_WORDS> din,
    W_TYPE width,
    H_TYPE height,
    bool &read_data,
    ac_array<PIX_TYPE, AC_WIN_HEIGHT, AC_WORDS> &dout,
    bool &sof_out,
    bool &eof_out,
    bool &sol_out,
    bool &eol_out,
    VOUT_TYPE &vld_out,
    bool repeat_line = false
  ) {
    AC_ASSERT(AC_REPEAT || !repeat_line, "repeat_line cannot be set to true if AC_REPEAT == false.");

    // Call input flag generator.
    bool sof, eof, sol, eol;
    bool only_update_line = outside_image || (AC_REPEAT && repeat_line);
    fgen_2d.run(width, height, sof, eof, sol, eol, only_update_line);

    // Call flag shifter.
    ac_int<AC_WIN_HEIGHT, false> sof_flags, eof_flags;
    ac_int<AC_WORDS, false> sol_flags, eol_flags;
    bool eof_old = fshift_2d.run(sof, eof, sol, eol, sof_flags, eof_flags, sol_flags, eol_flags, repeat_line);

    // Call linebuffer.
    #ifndef __SYNTHESIS__
    // The if condition for the branch below will result in an expected coverage hole. This is because the
    // BUFF_TYPE is a constant. It should have no bearing on the actual algorithm or the synthesis, as
    // it's merely used to guard a software assert.
    if (BUFF_TYPE != AC_DUAL) {
      AC_ASSERT(width%(2*AC_BUS_WORDS) == 0, "Image width must be divisible by 2*AC_BUS_WORDS if using singleport linebuffers.");
    }
    #endif
    if (sol) {
      linebuf_addr = 0;
    }
    bool linebuf_we = !(AC_REPEAT && repeat_line);
    LB_IN_TYPE linebuf_in;
    linebuf_in.pack_data(din);
    LB_OUT_TYPE linebuf_out;
    linebuf_2d.run(linebuf_we, linebuf_in, linebuf_addr, linebuf_out);
    linebuf_addr++;

    // Call data shifter.
    ac_array<PIX_TYPE, AC_SHIFT_HEIGHT, AC_WORDS + EXTRA_WORD> dshift_out;
    dshift_2d.rshift(linebuf_out, dshift_out);

    // Call boundary processing.
    BOUND_2D_TYPE bound_2d(padded_val);
    bound_2d.run(dshift_out, sof_flags, eof_flags, sol_flags, eol_flags, dout);

    // Call output flag generator.
    bool eof_out_temp;
    ac_int<2, false> eof_vals;
    eof_vals[0] = eof;
    eof_vals[1] = eof_old;
    fgen_out_2d.run(sof_flags, eof_flags, sol_flags, eol_flags, sof_out, eof_out_temp, sol_out, eol_out, vld_out, eof_vals);

    if (eof) {
      outside_image = true;
    }
    
    // If output EOF was encountered and AC_REPEAT == false, we reset various AC Window 2.0 flags to
    // prepare for the next frame.
    // However, if AC_REPEAT == true, we're probably resizing the image and the output EOF might not be
    // a reliable indicator of when to reset these flags. The user should instead explicitly call the
    // reset_flags() function before they send in a frame for windowing.
    if (!AC_REPEAT && eof_out_temp) {
      reset_flags();
    }
    
    read_data = !outside_image;

    eof_out = eof_out_temp;
  }
  
  void reset_flags() {
    outside_image = false;
    fgen_2d.reset();
    fshift_2d.reset();
    fgen_out_2d.reset();
  }

  void reset() {
    fgen_2d.reset();
    fshift_2d.reset();
    dshift_2d.reset();
    fgen_out_2d.reset();
    linebuf_addr = 0;
    outside_image = false;
  }

  ac_window_v2_2d() : padded_val(0) {
    reset();
  }

  ac_window_v2_2d(PIX_TYPE pval) : padded_val(pval) {
    reset();
  }

private:
  ac_flag_gen_2d<AC_IMG_WIDTH, AC_IMG_HEIGHT, AC_BUS_WORDS> fgen_2d;
  LB_TYPE linebuf_2d;
  ac_flag_shift_2d<AC_WIN_HEIGHT, AC_WIN_WIDTH, AC_PMODE, AC_BUS_WORDS, AC_REPEAT> fshift_2d;
  ac_shift_N_2d<PIX_TYPE, AC_WIN_HEIGHT, AC_WIN_WIDTH, AC_PMODE, AC_BUS_WORDS> dshift_2d;
  ac_flag_gen_out_2d<AC_WIN_HEIGHT, AC_WIN_WIDTH, AC_BUS_WORDS, USING_PADDING> fgen_out_2d;

  typename LB_TYPE::addrType linebuf_addr;
  PIX_TYPE padded_val;
  bool outside_image;
};

#endif
