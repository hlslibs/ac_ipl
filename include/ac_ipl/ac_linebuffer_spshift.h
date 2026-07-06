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
#ifndef _INCLUDED_AC_LINEBUFFER_SPSHIFT_H_
#define _INCLUDED_AC_LINEBUFFER_SPSHIFT_H_

/*---------------------------------------------------------------------------------------------
Templatatized line buffer class configured to use 2x wide singleport memory that acts like a dualport
Singleport memories are prefered for ASIC, This expects the line width to be equally divisible by 2
Reads are on even addresses and writes are on odd, Buffers here operates in shifting fashion
-----------------------------------------------------------------------------------------------*/



template<typename IN_TYPE, int AC_WIDTH, int AC_NUM_LINES, bool AC_REPEAT=0>
class ac_linebuffer_spshift
{
  enum {
    ADDR_BITS = ac::nbits<AC_WIDTH - 1>::val,//Size of the address type.
    TOTAL_LINES_ = AC_NUM_LINES-(AC_REPEAT?0:1),
    TOTAL_LINES = AC_MAX(TOTAL_LINES_, 1) // AC_MAX is used to prevent potential compiler errors.
  };

public:
  ac_linebuffer_spshift() {
    #ifndef DONT_UNINITIALIZE_LINEBUFFER
    ac::init_array<AC_VAL_DC>(&line_buffer[0][0], TOTAL_LINES*AC_WIDTH/2);
    ac::init_array<AC_VAL_DC>(&read_cache[0], TOTAL_LINES);
    ac::init_array<AC_VAL_DC>(&write_cache[0], TOTAL_LINES);
    din_cache.template set_val<AC_VAL_DC>();
    #endif
  }
  typedef ac_int<ADDR_BITS,false> addrType;
  void buffer(bool we, IN_TYPE din, addrType addr, ac_array<IN_TYPE,AC_NUM_LINES> &dout) {
    AC_ASSERT(AC_NUM_LINES != 1, "This linebuffer implementation cannot be used if AC_NUM_LINES = 1.");

    //Check if the address you are writing to an reading to is even or odd ?
    bool even = !(addr&1);//read on even

    //Based on the address passed in, if even Write the input pixel into the first half cache temporarily,
    // During odd iterations, write the value into the second half of cache which then combined written into the Line buffer.
    // When odd, we read the data out from the line buffer into teh read cache.
    if (even) {
      din_cache.set_slc(0, din.get_data());
      #pragma hls_unroll yes
      BUFF_SHIFT_LOOP:for (int i=0; i<TOTAL_LINES; i++) {
        #pragma hls_waive <abr> // Here the addr provided by the user is expected to be in the bounds of AC_WIDTH.
        read_cache[i] = line_buffer[i][addr/2];
      }
    } else {
      din_cache.set_slc(IN_TYPE::width,din.get_data());
      if (we) {
        #pragma hls_unroll yes
        BUFF_WR_LOOP:for (int i=0; i<TOTAL_LINES; i++) {
          if (i==TOTAL_LINES-1) {
            #pragma hls_waive <abw> // Here the addr provided by the user is expected to be in the bounds of AC_WIDTH.
            line_buffer[i][addr/2] = din_cache;
          } else {
            #pragma hls_waive <nco>
            line_buffer[i][addr/2] = read_cache[i+1];
          }
        }
      }
    }

    //Write output using either AC_NUM_LINES-1 of the buffers and din, or all the buffers if repeating
    #pragma hls_unroll yes
    BUFF_RD_LOOP: for (int i=0; i<AC_NUM_LINES; i++) {
      #pragma hls_waive <cns> // Here the CDC is checking for AC_NUM_LINES which is constatnt comiple time template  which is a expected behaviour.
      if (i==AC_NUM_LINES-1) {
        #pragma hls_waive <cns> // Here the CDC is checking for AC_REPEAT which is constatnt comiple time template  which is a expected behaviour.
        if (!we && AC_REPEAT) {
          dout[i].set_data(read_cache[i].template slc<IN_TYPE::width>(even ? 0:IN_TYPE::width));
        } else {
          dout[i] = din;
        }
      } else {
        if (we) {
          dout[i].set_data(read_cache[i+1-(AC_REPEAT?0:1)].template slc<IN_TYPE::width>(even ? 0:IN_TYPE::width));
        } else {
          dout[i].set_data(read_cache[i].template slc<IN_TYPE::width>(even ? 0:IN_TYPE::width));
        }
      }
    }
  }

  //Clear function to reset all buffer data
  void clear() {
    BUFF_CLR_LOOP:for (int i=0; i<TOTAL_LINES; i++) {
      for (int j=0; j<AC_WIDTH/2; j++) {
        line_buffer[i][j] = 0;
      }
    }
  }
  
private: 
  ac_int<IN_TYPE::width*2,false> line_buffer[TOTAL_LINES][AC_WIDTH/2];
  ac_int<IN_TYPE::width*2,false> write_cache[TOTAL_LINES],read_cache[TOTAL_LINES];
  ac_int<IN_TYPE::width*2,false> din_cache;
};

#endif
