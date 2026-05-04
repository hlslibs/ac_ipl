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
#ifndef _INCLUDED_AC_LINEBUFFER_H_
#define _INCLUDED_AC_LINEBUFFER_H_


#include <ac_ipl/ac_packed_vector.h> //ac_packed vector declaration.

// Individual line buffer declarations.
#include "ac_linebuffer_1r1w.h"
#include "ac_linebuffer_spshift.h"
#include "ac_linebuffer_spwrmask.h"
#include "ac_linebuffer_spcircular.h"

//Enums defining the Buffer types.
enum ac_buff_type {AC_DUAL,
                   AC_SPSHIFT,
                   AC_SPWRMASK,
                   AC_SPCIR
                  };

// Top level ac_linebuffer class.
template<typename INPUT_TYPE, typename OUTPUT_TYPE, int AC_WIDTH, int AC_NUM_LINES, ac_buff_type BUFF_TYPE, bool AC_REPEAT=0>
class ac_linebuffer
{

public:
  typedef ac_int<ac::nbits<(AC_WIDTH/INPUT_TYPE::packed_words) - 1>::val,false> addrType; //address type.
  static_assert(AC_NUM_LINES > 0, "AC_NUM_LINES must be positive.");
  static_assert(!AC_REPEAT || (AC_NUM_LINES > 1), "AC_NUM_LINES must be greater than 1 if line repetition is to be used.");
  static_assert(AC_WIDTH > 0, "AC_WIDTH must be positive.");
  static_assert(INPUT_TYPE::packed_words > 0, "Number of data blocks packed in the input (INPUT_TYPE::packed_words) must be positive.");
  static_assert(AC_WIDTH%INPUT_TYPE::packed_words == 0, "Buffer Width (AC_WIDTH) should be a multiple of Number of data blocks packed in the input (INPUT_TYPE::packed_words)");

  void run(bool we, INPUT_TYPE din, addrType addr, OUTPUT_TYPE &dout) {
    if (AC_NUM_LINES == 1) {
      packed_output[0] = din; // Linebuffer is not needed if there's just one line in the window.
    } else {
      lb.buffer(we, din, addr, packed_output);
    }
    linebuffer_access(packed_output,dout);
  }

//Clear method with line buffer choice.
  void clear() {
    lb.clear();
  }

// Line buffer access type that would return the data in ac_array type, where the size of the array return is Filter_HEIGHT, where each elemet is packed data value.
  template <typename PV_TYPE>
  void linebuffer_access(ac_array<PV_TYPE,AC_NUM_LINES> packed_input,  ac_array<typename PV_TYPE::base_type, AC_NUM_LINES, PV_TYPE::packed_words>  &output) {
#pragma hls_unroll yes
    LINE_ACCESS_LOOP_1:for (int i=0; i<AC_NUM_LINES; i++) {
#pragma hls_unroll yes
    LINE_ACCESS_LOOP_2:for (int j=0; j<PV_TYPE::packed_words; j++) {
        output[i][j]=packed_input[i][j];
      }
    }
  }

// Line buffer access type that would return the data in array type whoes each element is packed, where the size of the array return is Filter_HEIGHT, where each elemet can be accessed individually in packed version.
  template <typename PV_TYPE>
  void linebuffer_access(ac_array<PV_TYPE,AC_NUM_LINES> packed_input,  ac_array<PV_TYPE,AC_NUM_LINES> &output) {
#pragma hls_unroll yes
    LINE_ACCESS_LOOP:for (int i=0; i<AC_NUM_LINES; i++) {
      output[i]=packed_input[i];
    }
  }

private:

  //Generic struct declaration to later be specialized for defining the class type.
  template <typename IN_TY, int BUFF_WIDTH, int NUM_LINES,ac_buff_type buff_typ,  bool ac_rep>
  struct buffer_type_struct {};

  //Template specialized struct to declare the class instance of type where mem is mapped to a dual port 1Read and 1 Write memeory resource.
  template <typename IN_TY, int BUFF_WIDTH, int NUM_LINES, bool ac_rep>
  struct buffer_type_struct<IN_TY, BUFF_WIDTH, NUM_LINES, AC_DUAL, ac_rep> {
    typedef ac_linebuffer_1r1w<IN_TY, BUFF_WIDTH, NUM_LINES, ac_rep> Buff;
  };

  //Template specialized struct to declare the class instance of type where mem is mapped to a Single memeory resource and the buffer architecture is shift based.
  template <typename IN_TY, int BUFF_WIDTH, int NUM_LINES, bool ac_rep>
  struct buffer_type_struct<IN_TY, BUFF_WIDTH, NUM_LINES, AC_SPSHIFT, ac_rep> {
    typedef ac_linebuffer_spshift<IN_TY, BUFF_WIDTH, NUM_LINES, ac_rep> Buff;
    static_assert(BUFF_WIDTH%2 == 0, "For single port memory mapped Buffer, effective Buffer Width (AC_BUFFER_WIDTH/INPUT_TYPE::packed_words) should be a multiple of 2");
  };

  //Template specialized struct to declare the class instance of type where mem is mapped to a Single memeory resource and the buffer architecture is circular based.
  template <typename IN_TY, int BUFF_WIDTH, int NUM_LINES, bool ac_rep>
  struct buffer_type_struct<IN_TY, BUFF_WIDTH, NUM_LINES, AC_SPCIR, ac_rep> {
    typedef ac_line_buffer_spcircular<IN_TY, BUFF_WIDTH, NUM_LINES, ac_rep> Buff;
    static_assert(BUFF_WIDTH%2 == 0, "For single port memory mapped Buffer, effective Buffer Width (AC_BUFFER_WIDTH/INPUT_TYPE::packed_words) should be a multiple of 2");
  };

  //Template specialized struct to declare the class instance of type where mem is mapped to a Single Write mask memeory resource.
  template <typename IN_TY, int BUFF_WIDTH, int NUM_LINES, bool ac_rep>
  struct buffer_type_struct<IN_TY, BUFF_WIDTH, NUM_LINES, AC_SPWRMASK, ac_rep> {
    typedef ac_linebuffer_spwrmask<IN_TY, BUFF_WIDTH, NUM_LINES, ac_rep> Buff;
    static_assert(BUFF_WIDTH%2 == 0, "For single port memory mapped Buffer, effective Buffer Width (AC_BUFFER_WIDTH/INPUT_TYPE::packed_words) should be a multiple of 2");
  };

  ac_array<INPUT_TYPE,AC_NUM_LINES> packed_output; //Packed output from the individual LB.
  typedef typename buffer_type_struct<INPUT_TYPE,AC_WIDTH/INPUT_TYPE::packed_words,AC_NUM_LINES,BUFF_TYPE,AC_REPEAT>::Buff buff_type;
  buff_type lb;

};
#endif
