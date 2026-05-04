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
#ifndef _INCLUDED_AC_LINEBUFFER_1R1W_H_
#define _INCLUDED_AC_LINEBUFFER_1R1W_H_


/*---------------------------------------------------------------------------------------------
Templatatized line buffer class configured to use separate R/W memory
Buffer operates in shifting fashion, Likely only used for FPGA
AC_REPEAT is used if the buffers are read without writing them, this would be the case for something like interpolation
-----------------------------------------------------------------------------------------------*/

template<typename IN_TYPE, int AC_WIDTH, int AC_NUM_LINES, bool AC_REPEAT=0>
class ac_linebuffer_1r1w
{

  enum {
    ADDR_BITS = ac::nbits<AC_WIDTH - 1>::val,//Size of the address type.
    TOTAL_LINES_ = AC_NUM_LINES-(AC_REPEAT?0:1),
    TOTAL_LINES = AC_MAX(TOTAL_LINES_, 1) // AC_MAX is used to prevent potential compiler errors.
  };

public:
  ac_linebuffer_1r1w() {
    #ifndef DONT_UNINITIALIZE_LINEBUFFER
    ac_int<IN_TYPE::width, false> uninit_val;
    uninit_val.template set_val<AC_VAL_DC>();
    
    BUFF_UNINIT_LOOP_I: for (int i=0; i<TOTAL_LINES; i++) {
      BUFF_UNINIT_LOOP_J: for (int j=0; j<AC_WIDTH; j++) {
        line_buffer[i][j].set_data(uninit_val);
      }
    }
    #endif
  }
  
  typedef ac_int<ADDR_BITS,false> addrType;

  void buffer(bool we, IN_TYPE din, addrType addr, ac_array<IN_TYPE,AC_NUM_LINES> &dout) {
    AC_ASSERT(AC_NUM_LINES != 1, "This linebuffer implementation cannot be used if AC_NUM_LINES = 1.");
    
    IN_TYPE buf_reads[TOTAL_LINES];
    // Reading the memory loop, from line buffer to buff_Reads
    #pragma hls_unroll yes
    BUFF_RD_LOOP: for (int i=0; i<TOTAL_LINES; i++) {
      #pragma hls_waive <abr> // Here the addr provided by the user is expected to be in the bounds of AC_WIDTH.
      buf_reads[i] = line_buffer[i][addr];
    }

    //Write output using either AC_NUM_LINES-1 of the buffers and din, or all the buffers if repeating
    #pragma hls_unroll yes
    BUFF_OUT_LOOP: for (int i=0; i<AC_NUM_LINES; i++) {
      if (i==AC_NUM_LINES-1) {
        #pragma hls_waive <cns> // Here the CDC is checking for AC_REPEATe which is a expected behaviour.
        if (!we && AC_REPEAT) {
          dout[i] =  buf_reads[i];
        } else {
          dout[i] = din;
        }
      } else {
        if (we) {
          dout[i] = buf_reads[i+1-(AC_REPEAT?0:1)];//Adjust index if AC_REPEAT set
        } else {
          dout[i] = buf_reads[i];
        }
      }
    }

    //Writing into the buffer and shifting the values using the buf reads.
    if (we) {
      #pragma hls_unroll yes
      BUFF_WR_LOOP: for (int i=0; i<TOTAL_LINES; i++) {
        if (i==TOTAL_LINES-1) {
          #pragma hls_waive <abw> // Here the addr provided by the user is expected to be in the bounds of AC_WIDTH.
          line_buffer[i][addr] = din;
        } else {
          #pragma hls_waive <nco>
          line_buffer[i][addr] = buf_reads[i+1];
        }
      }
    }
  }

  //Clear method to reset the buffer data.
  void clear() {
    BUFF_CLR_LOOP: for (int i=0; i<TOTAL_LINES; i++) {
      for (int j=0; j<AC_WIDTH; j++) {
        for (int k=0; k<IN_TYPE::packed_words; k++) {
          line_buffer[i][j][k] = (typename IN_TYPE::base_type)0;
        }
      }
    }
  }

private: 
  IN_TYPE line_buffer[TOTAL_LINES][AC_WIDTH];
};
#endif
