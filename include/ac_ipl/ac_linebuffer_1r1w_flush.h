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
#ifndef _INCLUDED_AC_LINEBUFFER_1R1W_FLUSH_H_
#define _INCLUDED_AC_LINEBUFFER_1R1W_FLUSH_H_

#include <ac_int.h>
#include <ac_array.h>
#include <ac_ipl/ac_packed_vector.h>

template<typename INPUT_TYPE, int PACKED_WIDTH, int AC_NUM_LINES, bool AC_REPEAT = false>
class ac_linebuffer_1r1w_flush
{
public:
  enum {
    BUFF_WIDTH = PACKED_WIDTH,
    ADDR_BITS = ac::log2_ceil<BUFF_WIDTH>::val,
    TOTAL_LINES = AC_NUM_LINES-(AC_REPEAT?0:1)
  };
  
  typedef ac_int<ac::nbits<BUFF_WIDTH - 1>::val, false> ADDR_TYPE;
  typedef ac_int<ac::nbits<TOTAL_LINES>::val, false> BUF_CNT_TYPE;

  void run (
    INPUT_TYPE din, ADDR_TYPE write_addr,
    ADDR_TYPE read_addr,
    bool write_enabled,
    ac_array<INPUT_TYPE, AC_NUM_LINES> &dout,
    // If repeat_line = true, din is not written to memory and the linebuffer does not shift vertically.
    // dout is written purely based on stored linebuffer data.
    bool repeat_line = false
  ) {
    bool dont_repeat = !(AC_REPEAT && repeat_line);
    
    if ((read_addr == 0) && dont_repeat) {
      read_buf_cnt++;
      if (read_buf_cnt == TOTAL_LINES) {
        read_buf_cnt = 0;
      }
    }
    
    if ((write_addr == 0) && dont_repeat && write_enabled) {
      write_buf_cnt++;
      if (write_buf_cnt == TOTAL_LINES) {
        write_buf_cnt = 0;
      }
    }
    
    INPUT_TYPE buf_reads[TOTAL_LINES];
    #pragma hls_unroll yes
    SET_BUF_READS_1R1W: for (int i = 0; i < TOTAL_LINES; i++) {
      #pragma hls_waive <abr>
      buf_reads[i] = line_buffer[i][read_addr];
    }

    if (dont_repeat && write_enabled) {
      line_buffer[write_buf_cnt][write_addr] = din;
    }
    
    INPUT_TYPE buf_mux[TOTAL_LINES][TOTAL_LINES];

    #pragma hls_unroll yes
    REARRANGE_BUF_1R1W_I: for (int i=0; i<TOTAL_LINES; i++) {
      #pragma hls_unroll yes
      REARRANGE_BUF_1R1W_J: for (int j=0; j<TOTAL_LINES; j++) {
        int idx = i - j - int(dont_repeat);
        if (idx < 0) {
          idx = idx + TOTAL_LINES;
        }
        buf_mux[i][TOTAL_LINES-1-j] = buf_reads[idx];
      }
    }

    #pragma hls_unroll yes
    OUTPUT_1R1W: for (int i=0; i<AC_NUM_LINES; i++) {
      if (i == AC_NUM_LINES - 1) {
        // If we're not using line repetition, dont_repeat is always true and the if statement
        // below is always skipped, resulting in an expected coverage hole.
        #pragma hls_waive <cns>
        if (!dont_repeat) {
          dout[i] = buf_mux[read_buf_cnt][i];
        } else {
          dout[i] = din;
        }
      } else {
        // If we're not using line repetition, dont_repeat is always true and the else statement
        // below is always skipped, resulting in an expected coverage hole.
        if (dont_repeat) {
          dout[i] = buf_mux[read_buf_cnt][i + 1 - (AC_REPEAT?0:1)];
        } else {
          dout[i] = buf_mux[read_buf_cnt][i];
        }
      }
    }
  }

  void align_buf_cnts() {
    read_buf_cnt = write_buf_cnt;
  }
  
  bool misaligned() {
    return false;
  }

  void reset() {
    BUFF_RESET_I: for (int i = 0; i < TOTAL_LINES; i++) {
      BUFF_RESET_J: for (int j = 0; j < BUFF_WIDTH; j++) {
        line_buffer[i][j].reset();
      }
    }
  }
  
  ac_linebuffer_1r1w_flush() : read_buf_cnt(TOTAL_LINES - 1), write_buf_cnt(TOTAL_LINES - 1) {}
  
  #ifndef __SYNTHESIS__
  void print() {
    std::cout << "TOTAL_LINES = " << TOTAL_LINES << std::endl;
    std::cout << "BUFF_WIDTH = " << BUFF_WIDTH << std::endl;
   
    for (int i = 0; i < TOTAL_LINES; i++) {
      for (int j = 0; j < BUFF_WIDTH; j++) {
        std::cout << line_buffer[i][j] << " ";
      }
      std::cout << std::endl;
    }
  }
  #endif

private:
  INPUT_TYPE line_buffer[TOTAL_LINES][BUFF_WIDTH];
  BUF_CNT_TYPE read_buf_cnt, write_buf_cnt;
};
#endif
