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
#ifndef _INCLUDED_MEDIAN2D_HLS_H_
#define _INCLUDED_MEDIAN2D_HLS_H_

#if (defined(__GNUC__) && (__cplusplus < 201103L))
#error Please use C++17 or a later standard for compilation.
#endif
#if (defined(_MSC_VER) && (_MSC_VER < 1920) && !defined(__EDG__))
#error Please use Microsoft VS 2019 or a later standard for compilation.
#endif

#include <ac_int.h>
#include <ac_channel.h>
#include <ac_ipl/ac_window_v2_lflush.h>
#include <ac_math/ac_bitonic_sort.h>


#include <mc_scverify.h>

template <class Pix_type>
struct input_pixel_packet { // Renamed from ac_input_pixel_packet
  Pix_type data;
  bool valid;
  // Default constructors
  input_pixel_packet() : data(0), valid(false) {}
  input_pixel_packet(Pix_type d, bool v) : data(d), valid(v) {}
};

// --- NEW: OutputPixelPacket structure ---
template <class Pix_type>
struct output_pixel_packet { // Renamed from ac_output_pixel_packet
  Pix_type data;
  bool valid;
  // Default constructors
  output_pixel_packet() : data(0), valid(false) {}
  output_pixel_packet(Pix_type d, bool v) : data(d), valid(v) {}
};
// ----------------------------------------


template <class Pix_type, int AC_IMG_HEIGHT_C, int AC_IMG_WIDTH_C, int AC_WIN_SIZE_C, ac_buff_type BUFF_TYPE_C, ac_padding_method AC_PMODE_C>
class ac_median_2d
{
private:
  typedef ac_int<ac::nbits<AC_IMG_WIDTH_C>::val, false> width_type; // Renamed from WTYPE
  typedef ac_int<ac::nbits<AC_IMG_HEIGHT_C>::val, false> height_type; // Renamed from HTYPE
  typedef ac_window_v2_lflush_2d<Pix_type, AC_IMG_HEIGHT_C, AC_IMG_WIDTH_C, AC_WIN_SIZE_C, AC_WIN_SIZE_C, BUFF_TYPE_C, AC_PMODE_C> window_type; // Renamed from winType
  window_type window; // Updated usage

static constexpr int arraySize =
    (AC_WIN_SIZE_C == 3) ? 16 :
    (AC_WIN_SIZE_C == 5) ? 32 :
    (AC_WIN_SIZE_C == 7) ? 64 :
    (AC_WIN_SIZE_C == 9) ? 128 :
    16;
static constexpr int WN_SIZE_SQUARE = (AC_WIN_SIZE_C == 3) ? 9 :
    (AC_WIN_SIZE_C == 5) ? 25 :
    (AC_WIN_SIZE_C == 7) ? 49 :
    (AC_WIN_SIZE_C == 9) ? 81 :
    9;
 

public:
  ac_median_2d() {}
  #pragma hls_design interface
  #pragma hls_pipeline_init_interval 1
  void CCS_BLOCK(run)(ac_channel<input_pixel_packet<Pix_type>> &din_packet,
                      ac_channel<width_type> &width_ch, // Updated usage
                      ac_channel<height_type> &height_ch, // Updated usage
                      ac_channel<output_pixel_packet<Pix_type>> &dout_packet) {

    AC_ASSERT(((AC_WIN_SIZE_C == 3)||(AC_WIN_SIZE_C == 5)||(AC_WIN_SIZE_C == 7)||(AC_WIN_SIZE_C == 9)),"Valid window sizes are 3, 5, 7 and 9"); // CCOV_EXCL_LINE

    width_type width; // Updated usage
    height_type height; // Updated usage
    width = width_ch.read();
    height = height_ch.read();
    bool read_data = true, eof_out = false;
    Pix_type MedianValue;
    input_pixel_packet<Pix_type> current_input_packet;
    do {
      current_input_packet = read_data ? din_packet.read() : input_pixel_packet<Pix_type>(Pix_type(0), true);
      bool pixel_valid = current_input_packet.valid;
      Pix_type din;
      din = current_input_packet.data;

      output_pixel_packet<Pix_type> current_output_packet;

      if (pixel_valid) {

        ac_array<Pix_type, AC_WIN_SIZE_C, AC_WIN_SIZE_C> window_out;
        bool sof_out, sol_out, eol_out, vld_out;
        window.run(din, width, height, window_out, sof_out, eof_out, sol_out, eol_out, vld_out, read_data);

        bool output_valid = vld_out;;
        if (vld_out) {
        //  constexpr int arraySize = AC_WIN_SIZE_C*AC_WIN_SIZE_C;

          ac_array<Pix_type,arraySize> flatten_window;
          ac_array<Pix_type,arraySize> sorted_window;
		int i,j = 0;
        #pragma hls_unroll yes
        LOOP_Height: for(i=0;i<AC_WIN_SIZE_C;i++)
        {
			#pragma hls_unroll yes
            LOOP_Width: for(j=0;j<AC_WIN_SIZE_C;j++)
            {
                flatten_window[i*AC_WIN_SIZE_C+j] = window_out[i][j]; 
                
            
            }
        }    

        #pragma hls_unroll yes
        LOOP_Padded: for(i = WN_SIZE_SQUARE;i<arraySize;i++)
        {
            flatten_window[i] = 0;
        } 
		          ac_math::ac_bitonic_sort<Pix_type,uint32_t(arraySize),false,false> sorter;
         // ac_math::ac_brick_sort<Pix_type,uint32_t(arraySize),true> sorter;
          sorter.hardwareAcceleratedSort(flatten_window,sorted_window);
          MedianValue = sorted_window[(AC_WIN_SIZE_C*AC_WIN_SIZE_C)>>1];
        } else {
          MedianValue = Pix_type(0);
        }
        current_output_packet = output_pixel_packet<Pix_type>(MedianValue, output_valid);
        dout_packet.write(current_output_packet);
      } else {
        current_output_packet = output_pixel_packet<Pix_type>(Pix_type(0), false);
        dout_packet.write(current_output_packet);
      }
    } while (!eof_out);

  }
};
#endif // _INCLUDED_MEDIAN2D_HLS_H_


