/**************************************************************************
 *                                                                        *
 *  Algorithmic C (tm) Image Processing Library                           *
 *                                                                        *
 *  Software Version: 2025.4                                              *
 *                                                                        *
 *  Release Date    : Tue Nov 11 18:01:30 PST 2025                        *
 *  Release Type    : Production Release                                  *
 *  Release Build   : 2025.4.0                                            *
 *                                                                        *
 *  Copyright 2019 Siemens                                                *
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
////////////////////////////////////////////////////////////////////////////////
// 05-26-09 - Mike Fingeroff - Structs that allow doubling of singleport width
// and user specialization to support custom classes in ac_window
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __AC_WINDOW_STRUCTS__
#define __AC_WINDOW_STRUCTS__
#include <ac_int.h>
#include <ac_fixed.h>
#if __cplusplus >= 201103L
// Only include ac_ipl/ac_pixels.h header if compiler standard is C++11 or later.
#include <ac_ipl/ac_pixels.h>
#endif

#ifndef __SYNTHESIS__
#include <stdio.h>
#endif

template<typename T, bool IS_SINGLEPORT>
struct ac_width2x {
  typedef T data;
  static void set_half(bool sel_half, T din, data &dout) {
    #ifndef __SYNTHESIS__
    printf("ERROR: A user-defined specialization of this struct ac_width2x must be created when using\n");
    printf("user-defined classes and true singleport RAM.  Refer to the ac_window section of the Catapult\n");
    printf("users guide\n");
    assert(0);
    #endif
  }
  static void get_half(bool sel_half, data  din, T &dout) {
    #ifndef __SYNTHESIS__
    printf("ERROR: A user-defined specialization of this struct ac_width2x must be created when using\n");
    printf("user-defined classes and true singleport RAM.  Refer to the ac_window section of the Catapult\n");
    printf("users guide\n");
    assert(0);
    #endif
  }
};

//ac_int singleport support
template<int AC_WIDTH, bool AC_SIGN>
struct ac_width2x<ac_int<AC_WIDTH,AC_SIGN>, true> {
  typedef ac_int<AC_WIDTH*2,false> data;
  static void set_half(bool sel_half, ac_int<AC_WIDTH,AC_SIGN> din, data &dout) {
    int tmp = sel_half?AC_WIDTH:0;
    dout.set_slc(tmp,din);
  }
  static void get_half(bool sel_half, data  din, ac_int<AC_WIDTH,AC_SIGN> &dout) {
    ac_int<AC_WIDTH, false> half_slc = sel_half ? din.template slc<AC_WIDTH>(AC_WIDTH) : din.template slc<AC_WIDTH>(0);
    dout.set_slc(0, half_slc);
  }
};

//ac_int dualport support.
template<int AC_WIDTH, bool AC_SIGN>
struct ac_width2x<ac_int<AC_WIDTH,AC_SIGN>, false> {
  typedef ac_int<AC_WIDTH, AC_SIGN> data;
  static void set_half(bool sel_half, data din, data &dout) { }
  static void get_half(bool sel_half, data din, data &dout) { }
};

//ac_fixed singleport support
template<int AC_WIDTH, int AC_INTEGER, bool AC_SIGN, ac_q_mode AC_Q, ac_o_mode AC_O>
struct ac_width2x<ac_fixed<AC_WIDTH, AC_INTEGER, AC_SIGN, AC_Q, AC_O>, true> {
  typedef ac_fixed<AC_WIDTH*2,AC_WIDTH*2,false> data;
  static void set_half(bool sel_half, ac_fixed<AC_WIDTH,AC_INTEGER,AC_SIGN,AC_Q,AC_O> din, data &dout) {
    int tmp = sel_half?AC_WIDTH:0;
    dout.set_slc(tmp, din.template slc<AC_WIDTH>(0));
  }
  static void get_half(bool sel_half, data  din, ac_fixed<AC_WIDTH,AC_INTEGER,AC_SIGN,AC_Q,AC_O> &dout) {
    ac_int<AC_WIDTH, false> half_slc = sel_half ? din.template slc<AC_WIDTH>(AC_WIDTH) : din.template slc<AC_WIDTH>(0);
    dout.set_slc(0, half_slc);
  }
};

//ac_fixed dualport support
template<int AC_WIDTH, int AC_INTEGER, bool AC_SIGN, ac_q_mode AC_Q, ac_o_mode AC_O>
struct ac_width2x<ac_fixed<AC_WIDTH, AC_INTEGER, AC_SIGN, AC_Q, AC_O>, false> {
  typedef ac_fixed<AC_WIDTH, AC_INTEGER, AC_SIGN, AC_Q, AC_O> data;
  static void set_half(bool sel_half, data din, data &dout) { }
  static void get_half(bool sel_half, data din, data &dout) { }
};

#if __cplusplus >= 201103L
// ac_pixels.h header contains the RGB_imd struct, and this header is only included if the C++ compiler
// standard is C++11 or later. Hence, the RGB_imd specializations are only compiled if this compiler
// standard is satisfied.

// RGB_imd<ac_int> singleport support.
template<int AC_WIDTH, bool AC_SIGN>
struct ac_width2x<ac_ipl::RGB_imd<ac_int<AC_WIDTH, AC_SIGN> >, true> {
  typedef ac_ipl::RGB_imd<ac_int<AC_WIDTH*2,false> > data;
  typedef ac_width2x<ac_int<AC_WIDTH, AC_SIGN>, true> ac_width2x_internal_type;
  static void set_half(bool sel_half, ac_ipl::RGB_imd<ac_int<AC_WIDTH, AC_SIGN> > din, data &dout) {
    // Call ac_int versions of set_half functions, for each color component.
    ac_width2x_internal_type::set_half(sel_half, din.R, dout.R);
    ac_width2x_internal_type::set_half(sel_half, din.G, dout.G);
    ac_width2x_internal_type::set_half(sel_half, din.B, dout.B);
  }
  static void get_half(bool sel_half, data din, ac_ipl::RGB_imd<ac_int<AC_WIDTH, AC_SIGN> > &dout) {
    // Call ac_int versions of get_half functions, for each color component.
    ac_width2x_internal_type::get_half(sel_half, din.R, dout.R);
    ac_width2x_internal_type::get_half(sel_half, din.G, dout.G);
    ac_width2x_internal_type::get_half(sel_half, din.B, dout.B);
  }
};

// RGB_imd<ac_int> dualport support.
template<int AC_WIDTH, bool AC_SIGN>
struct ac_width2x<ac_ipl::RGB_imd<ac_int<AC_WIDTH, AC_SIGN> >, false> {
  typedef ac_ipl::RGB_imd<ac_int<AC_WIDTH, AC_SIGN> > data;
  static void set_half(bool sel_half, data din, data &dout) { }
  static void get_half(bool sel_half, data din, data &dout) { }
};

// RGB_imd<ac_fixed> singleport support.
template<int AC_WIDTH, int AC_INTEGER, bool AC_SIGN, ac_q_mode AC_Q, ac_o_mode AC_O>
struct ac_width2x<ac_ipl::RGB_imd<ac_fixed<AC_WIDTH, AC_INTEGER, AC_SIGN, AC_Q, AC_O> >, true> {
  typedef ac_ipl::RGB_imd<ac_fixed<AC_WIDTH*2,AC_WIDTH*2,false> > data;
  typedef ac_width2x<ac_fixed<AC_WIDTH, AC_INTEGER, AC_SIGN, AC_Q, AC_O>, true> ac_width2x_internal_type;
  static void set_half(bool sel_half, ac_ipl::RGB_imd<ac_fixed<AC_WIDTH, AC_INTEGER, AC_SIGN, AC_Q, AC_O> > din, data &dout) {
    // Call ac_fixed versions of set_half functions, for each color component.
    ac_width2x_internal_type::set_half(sel_half, din.R, dout.R);
    ac_width2x_internal_type::set_half(sel_half, din.G, dout.G);
    ac_width2x_internal_type::set_half(sel_half, din.B, dout.B);
  }
  static void get_half(bool sel_half, data din, ac_ipl::RGB_imd<ac_fixed<AC_WIDTH, AC_INTEGER, AC_SIGN, AC_Q, AC_O> > &dout) {
    // Call ac_fixed versions of get_half functions, for each color component.
    ac_width2x_internal_type::get_half(sel_half, din.R, dout.R);
    ac_width2x_internal_type::get_half(sel_half, din.G, dout.G);
    ac_width2x_internal_type::get_half(sel_half, din.B, dout.B);
  }
};

// RGB_imd<ac_fixed> dualport support.
template<int AC_WIDTH, int AC_INTEGER, bool AC_SIGN, ac_q_mode AC_Q, ac_o_mode AC_O>
struct ac_width2x<ac_ipl::RGB_imd<ac_fixed<AC_WIDTH, AC_INTEGER, AC_SIGN, AC_Q, AC_O> >, false> {
  typedef ac_ipl::RGB_imd<ac_fixed<AC_WIDTH, AC_INTEGER, AC_SIGN, AC_Q, AC_O> > data;
  static void set_half(bool sel_half, data din, data &dout) { }
  static void get_half(bool sel_half, data din, data &dout) { }
};
#endif

template<bool IS_SINGLEPORT>
struct ac_width2x<int, IS_SINGLEPORT> {
  typedef int data;

  static void set_half(bool sel_half, int din, data &dout) {

    #ifndef __SYNTHESIS__
    printf("ERROR: Only ac_int, ac_fixed, and custom classes that implement set_half and get_half are +supported\n");
    printf("when using true singleport RAM.  Refer to the ac_window section of the Catapult users guide on how to\n");
    printf("create a custom class\n");
    assert(0);
    #endif
  }

  static void get_half(bool sel_half, data  din, int &dout) {

  }
};

template<bool IS_SINGLEPORT>
struct ac_width2x<unsigned int, IS_SINGLEPORT> {
  typedef unsigned int data;

  static void set_half(bool sel_half, unsigned int din, data &dout) {

    #ifndef __SYNTHESIS__
    printf("ERROR: Only ac_int, ac_fixed, and custom classes that implement set_half and get_half are +supported\n");
    printf("when using true singleport RAM.  Refer to the ac_window section of the Catapult users guide on how to\n");
    printf("create a custom class\n");
    assert(0);
    #endif
  }

  static void get_half(bool sel_half, data  din, unsigned int &dout) {

  }
};

template<bool IS_SINGLEPORT>
struct ac_width2x<char, IS_SINGLEPORT> {
  typedef char data;

  static void set_half(bool sel_half, char din, data &dout) {

    #ifndef __SYNTHESIS__
    printf("ERROR: Only ac_int, ac_fixed, and custom classes that implement set_half and get_half are +supported\n");
    printf("when using true singleport RAM.  Refer to the ac_window section of the Catapult users guide on how to\n");
    printf("create a custom class\n");
    assert(0);
    #endif
  }

  static void get_half(bool sel_half, data  din, char &dout) {

  }
};

template<bool IS_SINGLEPORT>
struct ac_width2x<unsigned char, IS_SINGLEPORT> {
  typedef unsigned char data;

  static void set_half(bool sel_half, unsigned char din, data &dout) {

    #ifndef __SYNTHESIS__
    printf("ERROR: Only ac_int, ac_fixed, and custom classes that implement set_half and get_half are +supported\n");
    printf("when using true singleport RAM.  Refer to the ac_window section of the Catapult users guide on how to\n");
    printf("create a custom class\n");
    assert(0);
    #endif
  }

  static void get_half(bool sel_half, data  din, unsigned char &dout) {

  }
};

#endif
