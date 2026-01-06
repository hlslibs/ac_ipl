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
#ifndef _INCLUDED_AC_LINEBUFFER_SPWRMASK_FLUSH_H_
#define _INCLUDED_AC_LINEBUFFER_SPWRMASK_FLUSH_H_

#include <ac_int.h>
#include <ac_array.h>
#include <ac_ipl/ac_packed_vector.h>

#ifndef MEM_SIZE_UPSCALE
#define MEM_SIZE_UPSCALE false
#endif

template<typename INPUT_TYPE, int PACKED_WIDTH, int AC_NUM_LINES, bool AC_REPEAT = false>
class ac_linebuffer_spwrmask_flush
{
public:
  enum { TOTAL_LINES = AC_NUM_LINES - (AC_REPEAT ? 0 : 1) };
  
  typedef ac_int<ac::nbits<PACKED_WIDTH - 1>::val, false> ADDR_TYPE;
  typedef ac_int<ac::nbits<TOTAL_LINES>::val, false> BUF_CNT_TYPE;

  void run(
    INPUT_TYPE din, ADDR_TYPE write_addr,
    ADDR_TYPE read_addr,
    // If write_enabled = false, the linebuffer does not accept inputs while flushing outputs.
    bool write_enabled,
    ac_array<INPUT_TYPE, AC_NUM_LINES> &dout,
    // If repeat_line = true, din is not written to memory and the linebuffer does not shift vertically.
    // dout is written purely based on stored linebuffer data.
    bool repeat_line = false
  ) {
    bool dont_repeat = !(AC_REPEAT && repeat_line);
    
    bool read_even = (read_addr%2 == 0);
    if ((read_addr==0) && dont_repeat) {
      read_buf_cnt++;
      if (read_buf_cnt==TOTAL_LINES) {
        read_buf_cnt = 0;
      }
    }
    
    bool write_even = (write_addr%2 == 0);
    if ((write_addr==0) && dont_repeat && write_enabled) {
      write_buf_cnt++;
      if (write_buf_cnt==TOTAL_LINES) {
        write_buf_cnt = 0;
      }
    }
    
    if (write_enabled) {
      aligned = (write_even == read_even);
    }

    if (read_even) {
      if (dont_repeat && write_enabled) {
        if (aligned) {
          din_cache.set_slc(0, din.get_data());
        } else {
          din_cache.set_slc(INPUT_TYPE::width, din.get_data());
          ma_addr = write_addr;
          ma_buf_cnt = write_buf_cnt;
          write_ma_val = true;
        }
      }
      #pragma hls_unroll yes
      READ_FROM_LB_SPWRMASK: for (int i = 0; i < TOTAL_LINES; i++) {
        #pragma hls_waive <abr>
        read_cache[i] = line_buffer[read_addr/2].template slc<INPUT_TYPE::width*2>(INPUT_TYPE::width*2*i);
      }
    } else if (dont_repeat) {
      if (aligned && write_enabled) {
        din_cache.set_slc(INPUT_TYPE::width, din.get_data());
      }
      
      bool write_flag = (aligned && write_enabled);
      // write_addr and write_buf_cnt are unused if write_flag == false.
      ADDR_TYPE final_addr = write_ma_val ? ma_addr : write_addr;
      BUF_CNT_TYPE final_buf_cnt = write_ma_val ? ma_buf_cnt : write_buf_cnt;
      
      #pragma hls_unroll yes
      WRITE_TO_LB_SPWRMASK: for (int i = 0; i < TOTAL_LINES; i++) {
        if (i == final_buf_cnt) {
          if (write_ma_val || write_flag) {
            #pragma hls_waive <nco> <abr>
            line_buffer[final_addr/2].set_slc(INPUT_TYPE::width*2*i, din_cache);
          }
        }
      }
      
      write_ma_val = false;
      
      if (!aligned && write_enabled) {
        din_cache.set_slc(0, din.get_data());
      }
    }
    
    INPUT_TYPE buf_reads[TOTAL_LINES];

    #pragma hls_unroll yes
    SET_BUF_READS_SPWRMASK: for (int i=0; i<TOTAL_LINES; i++) {
      buf_reads[i].set_data(read_cache[i].template slc<INPUT_TYPE::width>(read_even ? 0 : INPUT_TYPE::width));
    }

    #pragma hls_unroll yes
    REARRANGE_BUF_SPWRMASK_I: for (int i=0; i<TOTAL_LINES; i++) {
      #pragma hls_unroll yes
      REARRANGE_BUF_SPWRMASK_J: for (int j=0; j<TOTAL_LINES; j++) {
        int idx = i - j - int(dont_repeat);
        if (idx < 0) {
          idx = idx + TOTAL_LINES ;
        }
        buf_mux[i][TOTAL_LINES-1-j] = buf_reads[idx];
      }
    }

    #pragma hls_unroll yes
    OUTPUT_SPWRMASK: for (int i=0; i<AC_NUM_LINES; i++) {
      if (i==AC_NUM_LINES-1) {
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
        #pragma hls_waive <cns>
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
    return write_ma_val;
  }

  void reset() {
    BUFF_RESET_SPWRMASK: for (int j = 0; j < PACKED_WIDTH/2; j++) {
      line_buffer[j] = 0;
    }
    
    read_buf_cnt = write_buf_cnt = TOTAL_LINES - 1;
    ma_buf_cnt = ma_addr = 0;
    aligned = true;
    write_ma_val = false;
  }

  #ifndef __SYNTHESIS__
  void print() {
    std::cout << "Linebuffer contents:" << std::endl;
    for (int i = 0; i < TOTAL_LINES; i++) {
      std::cout << "  ";
      for (int j = 0; j < MEM_SIZE; j++) {
        ac_int<INPUT_TYPE::width*2, false> lb_val = line_buffer[j].template slc<INPUT_TYPE::width*2>(INPUT_TYPE::width*2*i);
        ac_int<INPUT_TYPE::width, false> lb_val1 = lb_val.template slc<INPUT_TYPE::width>(0);
        ac_int<INPUT_TYPE::width, false> lb_val2 = lb_val.template slc<INPUT_TYPE::width>(INPUT_TYPE::width);
        std::cout << lb_val1 << " " << lb_val2 << " ";
      }
      std::cout << std::endl;
    }
  }
  #endif
  
  ac_linebuffer_spwrmask_flush() : read_buf_cnt(TOTAL_LINES - 1), write_buf_cnt(TOTAL_LINES - 1), ma_buf_cnt(0), ma_addr(0), aligned(true), write_ma_val(false) { }

private:
  enum {
    ORIG_SZ = PACKED_WIDTH/2,
    SZ_NBITS = ac::nbits<ORIG_SZ>::val,
    SZ_L2C = ac::log2_ceil<ORIG_SZ>::val,
    RND_MEM_SIZE= (int)SZ_NBITS != (int)SZ_L2C ? ORIG_SZ : 1<<SZ_NBITS,
    MEM_SIZE= MEM_SIZE_UPSCALE ? RND_MEM_SIZE : ORIG_SZ
  };

  ac_int<INPUT_TYPE::width*2*TOTAL_LINES,false> line_buffer[MEM_SIZE];
  ac_int<INPUT_TYPE::width*2,false> read_cache[TOTAL_LINES];
  ac_int<INPUT_TYPE::width*2,false> din_cache;
  BUF_CNT_TYPE read_buf_cnt, write_buf_cnt, ma_buf_cnt;
  INPUT_TYPE buf_mux[TOTAL_LINES][TOTAL_LINES];
  ADDR_TYPE ma_addr; // Stores addresses for misaligned values.
  bool aligned, write_ma_val;
};

#endif
