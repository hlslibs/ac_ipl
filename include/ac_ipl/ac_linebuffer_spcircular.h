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
#ifndef _INCLUDED_AC_LINEBUFFER_SPCIRCULAR_H_
#define _INCLUDED_AC_LINEBUFFER_SPCIRCULAR_H_

/*---------------------------------------------------------------------------------------------
Templatatized line buffer class configured to use 2x wide singleport memory that acts like a dualport. Singleport memories are prefered for ASIC
This expects the line width to be equally divisible by 2.Reads are on even addresses and writes are on odd and Buffer operates in circular fashion
-----------------------------------------------------------------------------------------------*/

template<typename IN_TYPE, int AC_WIDTH, int AC_NUM_LINES, bool AC_REPEAT=0>
class ac_line_buffer_spcircular
{
  enum {
    ADDR_BITS = ac::nbits<AC_WIDTH - 1>::val,//Size of the address type.
    TOTAL_LINES_ = AC_NUM_LINES-(AC_REPEAT?0:1),
    TOTAL_LINES = AC_MAX(TOTAL_LINES_, 1), // AC_MAX is used to prevent potential compiler errors.
    BUF_CNT_BITS = ac::nbits<AC_NUM_LINES>::val// Size of the buffer count variable
  };

public:
  ac_line_buffer_spcircular():buf_cnt(TOTAL_LINES-1) {
    #ifndef DONT_UNINITIALIZE_LINEBUFFER
    ac::init_array<AC_VAL_DC>(&line_buffer[0][0], TOTAL_LINES*AC_WIDTH/2);
    ac::init_array<AC_VAL_DC>(&read_cache[0], TOTAL_LINES);
    din_cache.template set_val<AC_VAL_DC>();
    #endif
  }
  typedef ac_int<ADDR_BITS,false> addrType; //Address type

  void buffer(bool we, IN_TYPE din, addrType addr, ac_array<IN_TYPE,AC_NUM_LINES> &dout) {
    AC_ASSERT(AC_NUM_LINES != 1, "This linebuffer implementation cannot be used if AC_NUM_LINES = 1.");

    //Check if the address you are writing to an reading to is even or odd ?
    // buf_cnt determines active write buffer, it begins to increment when write_enabled
    // After it hits the last line of the buffer, re iterates from the first buffer.
    bool even = !(addr&1);//read on even
    //buf_cnt determines active write buffer
    if ((addr==0) & we) {
      buf_cnt++;
      if (buf_cnt==TOTAL_LINES) {
        buf_cnt = 0;
      }
    }

    //Based on the address passed in, if even Write the input pixel into the first half cache temporarily,
    // During odd iterations, write the value into the second half of cache which then combined written into the Line buffer.
    // When odd, we read the data out from the line buffer into teh read cache.
    if (even) {
      din_cache.set_slc(0, din.get_data());
      #pragma hls_unroll yes
      BUFF_RD_LOOP: for (int i=0; i<TOTAL_LINES; i++) {
        #pragma hls_waive <abr> // Here the addr provided by the user is expected to be in the bounds of AC_WIDTH.
        read_cache[i] = line_buffer[i][addr/2];
      }
    } else {//Write on odd into active buffer
      din_cache.set_slc(IN_TYPE::width,din.get_data());
      if (we) {
        #pragma hls_waive <nco>	<abw>
        line_buffer[buf_cnt][addr/2] = din_cache;
      }
    }

    IN_TYPE buf_reads[TOTAL_LINES];
    //Read from buffer caches based on even or odd into the buff reads.
    #pragma hls_unroll yes
    BUFF_REARR_LOOP: for (int i=0; i<TOTAL_LINES; i++) {
      buf_reads[i].set_data(read_cache[i].template slc<IN_TYPE::width>(even ? 0:IN_TYPE::width));
    }

    //Write output using either AC_NUM_LINES-1 of the buffers and din, or all the buffers if repeating
    IN_TYPE buf_mux[TOTAL_LINES][TOTAL_LINES];
    #pragma hls_unroll yes
    BUFF_MUX_LOOP: for (int i=0; i<TOTAL_LINES; i++) {
      #pragma hls_unroll yes
      for (int j=0; j<TOTAL_LINES; j++) {
        int idx = i-j-(we?1:0);//Adjust the index when writing
        if (idx < 0) {
          idx = idx + TOTAL_LINES ;
        }
        buf_mux[i][TOTAL_LINES-1-j] = buf_reads[idx];
      }
    }

    #pragma hls_unroll yes
    BUFF_OUT_LOOP: for (int i=0; i<AC_NUM_LINES; i++) {
      if (i==AC_NUM_LINES-1) {
        #pragma hls_waive <cns> // Here the CDC is checking for AC_REPEAT which is constatnt comiple time template  which is a expected behaviour.
        if (!we && AC_REPEAT) {//If not writing and AC_REPEAT set just read all the buffers
          dout[i] = buf_mux[buf_cnt][i];
        } else {
          dout[i] = din;
        }
      } else {
        if (we) {
          dout[i] = buf_mux[buf_cnt][i+1-(AC_REPEAT?0:1)];//Adjust index if AC_REPEAT set
        } else {
          dout[i] = buf_mux[buf_cnt][i];
        }
      }
    }
  }

  //Clear function to reset all buffer data
  void clear() {
    BUFF_CLR_LOOP: for (int i=0; i<TOTAL_LINES; i++) {
      for (int j=0; j<AC_WIDTH/2; j++) {
        line_buffer[i][j] = 0;
      }
    }
  }

private:
  ac_int<IN_TYPE::width*2,false> line_buffer[TOTAL_LINES][AC_WIDTH/2];
  ac_int<IN_TYPE::width*2,false> read_cache[TOTAL_LINES];
  ac_int<IN_TYPE::width*2,false> din_cache;
  ac_int<BUF_CNT_BITS,false> buf_cnt;
};

#endif
