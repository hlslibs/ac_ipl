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
#ifndef _INCLUDED_AC_LINEBUFFER_FLUSH_H_
#define _INCLUDED_AC_LINEBUFFER_FLUSH_H_

#include <ac_ipl/ac_packed_vector.h>

#include "ac_linebuffer_1r1w_flush.h"
#include "ac_linebuffer_spcircular_flush.h"
#include "ac_linebuffer_spwrmask_flush.h"

// Enums defining the Buffer architectures.
enum ac_buff_arch_flush {
  AC_1R1W_FLUSH,
  AC_SPWRMASK_FLUSH,
  AC_SPCIR_FLUSH
};

// Top level linebuffer class.
template<typename INPUT_TYPE, typename OUTPUT_TYPE, int AC_WIDTH, int AC_NUM_LINES, ac_buff_arch_flush AC_BUFF_ARCH, bool AC_REPEAT = false>
class ac_linebuffer_flush
{
public:
  enum {
    AC_BUS_WORDS = INPUT_TYPE::packed_words,
    PACKED_WIDTH = AC_WIDTH/AC_BUS_WORDS,
  };
  typedef typename INPUT_TYPE::base_type BASE_TYPE;
  typedef ac_int<ac::nbits<PACKED_WIDTH - 1>::val, false> ADDR_TYPE;
  static_assert(AC_WIDTH > 0, "AC_WIDTH must be positive.");
  static_assert(AC_WIDTH%AC_BUS_WORDS == 0, "AC_WIDTH should be divisible by AC_BUS_WORDS.");

  void run(
    INPUT_TYPE din, ADDR_TYPE write_addr,
    ADDR_TYPE read_addr,
    bool write_enabled,
    OUTPUT_TYPE &dout,
    // If repeat_line = true, din is not written to memory and the linebuffer does not shift vertically.
    // dout is written purely based on stored linebuffer data.
    bool repeat_line = false
  ) {
    AC_ASSERT(AC_REPEAT || !repeat_line, "repeat_line cannot be set to true if AC_REPEAT == false.");
    
    ac_array<INPUT_TYPE,AC_NUM_LINES> packed_output;
    lb.run(din, write_addr, read_addr, write_enabled, packed_output, repeat_line);
    linebuffer_access(packed_output, dout);
  }
  
  void reset() {
    lb.reset();
  }
  
  void linebuffer_access(ac_array<INPUT_TYPE, AC_NUM_LINES> packed_input, ac_array<BASE_TYPE, AC_NUM_LINES, AC_BUS_WORDS> &output) {
#pragma hls_unroll yes
    COPY_INPUT_TO_ARR_I: for (int i=0; i<AC_NUM_LINES; i++) {
#pragma hls_unroll yes
      COPY_INPUT_TO_ARR_J: for (int j=0; j<AC_BUS_WORDS; j++) {
        output[i][j]=packed_input[i][j];
      }
    }
  }
  
  void linebuffer_access(ac_array<INPUT_TYPE, AC_NUM_LINES> packed_input, ac_array<INPUT_TYPE, AC_NUM_LINES> &output) {
#pragma hls_unroll yes
    COPY_INPUT_TO_PV_ARR: for (int i=0; i<AC_NUM_LINES; i++) {
      output[i]=packed_input[i];
    }
  }
  
  void align_buf_cnts() {
    lb.align_buf_cnts();
  }
  
  bool misaligned() {
    return lb.misaligned();
  }

private:
  // The DUMMY template parameter ensures that buffer_type_struct is always partially specialized and
  // that the code will pass C++ compilation.
  template <bool DUMMY, ac_buff_arch_flush BUFF_ARCH>
  struct buffer_type_struct {};

  template <bool DUMMY>
  struct buffer_type_struct<DUMMY, AC_SPWRMASK_FLUSH> {
    static_assert(PACKED_WIDTH%2 == 0, "AC_WIDTH should be divisible by 2*AC_BUS_WORDS if you're using singleport buffers.");
    typedef ac_linebuffer_spwrmask_flush<INPUT_TYPE, PACKED_WIDTH, AC_NUM_LINES, AC_REPEAT> BUFF_TYPE;
  };

  template <bool DUMMY>
  struct buffer_type_struct<DUMMY, AC_SPCIR_FLUSH> {
    static_assert(PACKED_WIDTH%2 == 0, "AC_WIDTH should be divisible by 2*AC_BUS_WORDS if you're using singleport buffers.");
    typedef ac_linebuffer_spcircular_flush<INPUT_TYPE, PACKED_WIDTH, AC_NUM_LINES, AC_REPEAT> BUFF_TYPE;
  };

  template <bool DUMMY>
  struct buffer_type_struct<DUMMY, AC_1R1W_FLUSH> {
    typedef ac_linebuffer_1r1w_flush<INPUT_TYPE, PACKED_WIDTH, AC_NUM_LINES, AC_REPEAT> BUFF_TYPE;
  };

  static constexpr bool DUMMY = false;
  typedef typename buffer_type_struct<DUMMY, AC_BUFF_ARCH>::BUFF_TYPE BUFF_TYPE;
  BUFF_TYPE lb;
};
#endif
