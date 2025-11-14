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
 *  Copyright  Siemens                                                *
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
// To compile stand-alone and run:
// $MGC_HOME/bin/c++ -std=c++11 -I$MGC_HOME/shared/include rtest_ac_window_v2.cpp -o design
// ./design

#include <ac_ipl/ac_window_v2.h>

#include <string>
#include <iostream>
using namespace std;

template <int FLAG_SIZE>
void print_flag_arr_1d(ac_int<FLAG_SIZE, false> flag_arr)
{
  cout << "{";

  for (int i = 0; i < FLAG_SIZE; i++) {
    cout << flag_arr[i];
    if (i != FLAG_SIZE - 1) {
      cout << " ";
    }
  }

  cout << "}";
}

int idx_flatten(int i, int j, int height, int width)
{
  AC_ASSERT(i >= 0, "i is negative.");
  AC_ASSERT(j >= 0, "j is negative.");
  AC_ASSERT(i < height, "i exceeds height.");
  AC_ASSERT(j < width, "j exceeds width.");
  return i*width + j;
}

template <ac_padding_method AC_PMODE>
int bounds(int i, int sz)
{
  if (AC_PMODE == AC_REFLECT101) {
    if (i < 0) {
      return -i;
    } else if (i >= sz) {
      return 2*(sz - 1) - i;
    } else {
      return i;
    }
  }
  if (AC_PMODE == AC_REFLECT) {
    if (i < 0) {
      return -i - 1;
    } else if (i >= sz) {
      return 2*(sz - 1) - i + 1;
    } else {
      return i;
    }
  }

  if (AC_PMODE == AC_REPLICATE) {
    if (i < 0) {
      return 0;
    } else if (i >= sz) {
      return sz - 1;
    } else {
      return i;
    }
  }
}

template <class PIX_TYPE, int AC_WIN_WIDTH, ac_padding_method AC_PMODE, int AC_BUS_WORDS = 1, bool HIGH_SPEED_REPLICATE = false>
bool test_driver_1d(int num_lines, int padded_val = 0)
{
  int width = 10*AC_BUS_WORDS;

  cout << "TEST: 1D.  PIX_TYPE: ";
  cout.width(18);
  cout << left << PIX_TYPE::type_name();
  cout << "AC_WIN_WIDTH: ";
  cout.width(3);
  cout << left << AC_WIN_WIDTH;
  cout << "AC_BUS_WORDS: ";
  cout.width(3);
  cout << left << AC_BUS_WORDS;
  cout << "HIGH_SPEED_REPLICATE: ";
  cout << left << (HIGH_SPEED_REPLICATE ? "true   " : "false  ");
  
  cout.width(26);
  
  if (AC_PMODE == AC_REPLICATE) {
    cout << left << "AC_PMODE = AC_REPLICATE";
  }
  if (AC_PMODE == AC_CONSTANT) {
    cout << left << "AC_PMODE = AC_CONSTANT";
  }
  if (AC_PMODE == AC_REFLECT101) {
    cout << left << "AC_PMODE = AC_REFLECT101";
  }
  if (AC_PMODE == AC_REFLECT) {
    cout << left << "AC_PMODE = AC_REFLECT";
  }
  
  cout << "width: ";
  cout.width(4);
  cout << left << width;
  cout << "num_lines: ";
  cout.width(3);
  cout << left << num_lines;
  cout << "RESULT: ";

  int *img = new int[width*num_lines];
  typedef ac_array<int, AC_WIN_WIDTH> ref_win_type;
  ref_win_type *windows = new ref_win_type[width*num_lines];
  
  int window_cnt = -1;
  int num_pack_per_line = width/AC_BUS_WORDS;
  
  for (int i = 0; i < num_lines; i++) {
    for (int j = 0; j < width; j++) {
      img[i*width + j] = i*width + j + 1;
    }
  }
  
  for (int i = 0; i < num_lines; i++) {
    for (int j = 0; j < num_pack_per_line; j++) {
      for (int k = 0; k < AC_BUS_WORDS; k++) {
        int center_r = i;
        int center_c = j*AC_BUS_WORDS + k;
        
        window_cnt++;
        
        for (int c = 0; c < AC_WIN_WIDTH; c++) {
          if ((AC_PMODE == AC_REPLICATE) || (AC_PMODE == AC_REFLECT101) || (AC_PMODE == AC_REFLECT)) {
            int pix_r = center_r;
            int pix_c = bounds<AC_PMODE>(center_c + c - AC_WIN_WIDTH/2, width);
            windows[window_cnt][c] = img[idx_flatten(pix_r, pix_c, num_lines, width)];
          }
        
          if (AC_PMODE == AC_CONSTANT) {
            int pix_r = center_r;
            int pix_c = center_c + c - AC_WIN_WIDTH/2;
            
            bool window_inside_img = pix_c >= 0 && pix_c < width;
            windows[window_cnt][c] = window_inside_img ? img[idx_flatten(pix_r, pix_c, num_lines, width)] : padded_val;
          }
        }
      }
    }
  }
  
  enum {
    AC_IMG_WIDTH = 1080
  };
  
  typedef ac_window_v2_1d<PIX_TYPE, AC_IMG_WIDTH, AC_WIN_WIDTH, AC_PMODE, AC_BUS_WORDS, HIGH_SPEED_REPLICATE> ac_win_type;

  enum {
    AC_WORDS = ac_win_type::AC_WORDS,
  };
  
  ac_win_type ac_win_obj(padded_val);

  window_cnt = -1;

  for (int i = 0; i < num_lines; i++) {
    int j = 0;
    bool eol_out = false, read_data = true;
    
    do {
      ac_array<PIX_TYPE, AC_BUS_WORDS> din;
      bool inside_image = (j < num_pack_per_line);

      for (int k = 0; k < AC_BUS_WORDS; k++) {
        din[k] = inside_image ? img[i*width + j*AC_BUS_WORDS + k] : 0;
      }
      
      #ifdef DEBUG
      cout << "-------------------" << endl;
      cout << "i = " << i << endl;
      cout << "j = " << j << endl;
      cout << "din = " << din << endl;
      cout << "read_data = " << read_data << endl;
      #endif
      
      if (read_data != inside_image) {
        #ifdef DEBUG
        AC_ASSERT(false, "read_data does not match expected value.");
        #endif
        cout << "FAILED." << endl;
        return false;
      }
      
      ac_array<PIX_TYPE, AC_WORDS> dout;
      bool sol_out;
      typename ac_win_type::VOUT_TYPE vld_out;
      ac_win_obj.run(din, width, read_data, dout, sol_out, eol_out, vld_out);

      #ifdef DEBUG
      cout << "dout = " << dout << endl;
      cout << "sol_out = " << sol_out << endl;
      cout << "eol_out = " << eol_out << endl;
      cout << "vld_out = " << vld_out << endl;
      #endif

      if (vld_out) {
        for (int k = 0; k < AC_BUS_WORDS; k++) {
          window_cnt++;
          #ifdef DEBUG
          cout << "k = " << k << endl;
          cout << "windows[" << window_cnt << "] = " << windows[window_cnt] << endl;
          cout << "dout = " << dout << endl;
          #endif
          
          for (int c = 0; c < AC_WIN_WIDTH; c++) {
            if (windows[window_cnt][c] != dout[k + c]) {
              #ifdef DEBUG
              AC_ASSERT(false, "Reference and design output do not match.");
              #endif
              cout << "FAILED." << endl;
              return false;
            }
          }
          
          if (k == 0) {
            if (sol_out != (window_cnt%width == 0)) {
              #ifdef DEBUG
              AC_ASSERT(false, "sol_out does not match expected value.");
              #endif
              cout << "FAILED." << endl;
              return false;
            }
          }
          
          if (k == AC_BUS_WORDS - 1) {
            if (eol_out != (window_cnt%width == width - 1)) {
              #ifdef DEBUG
              AC_ASSERT(false, "eol_out does not match expected value.");
              #endif
              cout << "FAILED." << endl;
              return false;
            }
          }
        }
      }
      
      j++;
    } while (!eol_out);
  }
  
  if (window_cnt != width*num_lines - 1) {
    #ifdef DEBUG
    AC_ASSERT(false, "All windows were not tested.");
    #endif
    cout << "FAILED." << endl;
    return false;
  }
  
  delete[] img;
  delete[] windows;

  cout << "PASSED." << endl;
  return true;
}

// If AC_REPEAT is turned on, the design repeats a certain number of lines. The number of lines to be repeated are determined using the repetition ratio. The repetition ratio is given by the
// integer inputs "num_rep_ratio" and "den_rep_ratio" which denote the numerator and denominator of the ratio. By default, the repetition ratio is 3/5 (i.e. num_rep_ratio = 3 and den_rep_ratio = 5).
// This means that out of every 5 lines the design processes, the first 3 lines are lines with actual image inputs, whereas the last 2 (5 - 3) lines entail a repetition over the same set of lines
// that the window was processing during the 3rd line.
template <class PIX_TYPE, int AC_WIN_HEIGHT, int AC_WIN_WIDTH, ac_buff_type BUFF_TYPE, ac_padding_method AC_PMODE, int AC_BUS_WORDS = 1, bool HIGH_SPEED_REPLICATE = false, bool AC_REPEAT = false>
bool test_driver_2d(int height, int padded_val = 0, int num_rep_ratio = 3, int den_rep_ratio = 5)
{
  int width = 10*AC_BUS_WORDS;

  if (!AC_REPEAT) {
    cout << "TEST: 2D.  PIX_TYPE: ";
  } else {
    cout << "TEST: 2D(r).  PIX_TYPE: "; // "(r)": We're using line repetition.
  }
  cout.width(18);
  cout << left << PIX_TYPE::type_name();
  cout << "AC_WIN_HEIGHT: ";
  cout.width(3);
  cout << left << AC_WIN_HEIGHT;
  cout << "AC_WIN_WIDTH: ";
  cout.width(3);
  cout << left << AC_WIN_WIDTH;
  cout << "AC_BUS_WORDS: ";
  cout.width(3);
  cout << left << AC_BUS_WORDS;
  
  // If we're using AC_REPEAT, odds are that the AC_REPLICATE architecture isn't really important and
  // we don't need to display information on the "HIGH_SPEED_REPLICATE" parameter.
  if (!AC_REPEAT) {
    cout << "HIGH_SPEED_REPLICATE: ";
    cout << left << (HIGH_SPEED_REPLICATE ? "true   " : "false  ");
  }
  
  cout.width(25);
  
  if (BUFF_TYPE == AC_DUAL) {
    cout << left << "BUFF_TYPE = AC_DUAL";
  }
  
  if (BUFF_TYPE == AC_SPSHIFT) {
    cout << left << "BUFF_TYPE = AC_SPSHIFT";
  }
  
  if (BUFF_TYPE == AC_SPWRMASK) {
    cout << left << "BUFF_TYPE = AC_SPWRMASK";
  }
  
  if (BUFF_TYPE == AC_SPCIR) {
    cout << left << "BUFF_TYPE = AC_SPCIR";
  }
  
  cout.width(26);
  
  if (AC_PMODE == AC_REPLICATE) {
    cout << left << "AC_PMODE = AC_REPLICATE";
  }
  if (AC_PMODE == AC_CONSTANT) {
    cout << left << "AC_PMODE = AC_CONSTANT";
  }
  if (AC_PMODE == AC_REFLECT101) {
    cout << left << "AC_PMODE = AC_REFLECT101";
  }
  if (AC_PMODE == AC_REFLECT) {
    cout << left << "AC_PMODE = AC_REFLECT";
  }
  
  cout << "width: ";
  cout.width(4);
  cout << left << width;
  cout << "height: ";
  cout.width(4);
  cout << left << height;
  
  if (AC_REPEAT) {
    cout << "rr: "; // rr: Repetition ratio
    string rr_str = to_string(num_rep_ratio) + "/" + to_string(den_rep_ratio);
    cout.width(5);
    cout << left << rr_str;
  }
  
  cout << "RESULT: ";

  int *img = new int[width*height];
  typedef ac_array<int, AC_WIN_HEIGHT, AC_WIN_WIDTH> ref_win_type;
  ref_win_type *windows = new ref_win_type[width*height];
  
  int window_cnt = -1;
  int num_pack_per_line = width/AC_BUS_WORDS;
  
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      img[i*width + j] = i*width + j + 1;
    }
  }
  
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < num_pack_per_line; j++) {
      for (int k = 0; k < AC_BUS_WORDS; k++) {
        int center_r = i;
        int center_c = j*AC_BUS_WORDS + k;
        window_cnt++;
        
        for (int r = 0; r < AC_WIN_HEIGHT; r++) {
          for (int c = 0; c < AC_WIN_WIDTH; c++) {
            if((AC_PMODE == AC_REPLICATE) || (AC_PMODE == AC_REFLECT101) || (AC_PMODE == AC_REFLECT)) {
              int pix_r = bounds<AC_PMODE>(center_r + r - AC_WIN_HEIGHT/2, height);
              int pix_c = bounds<AC_PMODE>(center_c + c - AC_WIN_WIDTH/2, width);
              windows[window_cnt][r][c] = img[idx_flatten(pix_r, pix_c, height, width)];
            }
        
            if (AC_PMODE == AC_CONSTANT) {
              int pix_r = center_r + r - AC_WIN_HEIGHT/2;
              int pix_c = center_c + c - AC_WIN_WIDTH/2;
              bool window_inside_img = pix_r >= 0 && pix_r < height && pix_c >= 0 && pix_c < width;
              windows[window_cnt][r][c] = window_inside_img ? img[idx_flatten(pix_r, pix_c, height, width)] : padded_val;
            }
          }
        }
      }
    }
  }
  
  enum {
    AC_IMG_WIDTH = 1080,
    AC_IMG_HEIGHT = 1024,
  };
  
  typedef ac_window_v2_2d<PIX_TYPE, AC_IMG_HEIGHT, AC_IMG_WIDTH, AC_WIN_HEIGHT, AC_WIN_WIDTH, BUFF_TYPE, AC_PMODE, AC_BUS_WORDS, HIGH_SPEED_REPLICATE, AC_REPEAT> ac_win_type;

  enum {
    AC_WORDS = ac_win_type::AC_WORDS,
  };
  
  ac_win_type ac_win_obj(padded_val);
  
  for (int frame_cnt = 0; frame_cnt < 3; frame_cnt++) {
    int i = 0, j = 0;
    bool eof_out = false;
    window_cnt = -1;
    bool read_data = true;
    // The two flags below are used only if AC_REPEAT = true.
    int i_total = 0; // Counter for all the lines, including repeated ones.
    bool repeat_line = false; // Are we repeating a line?
    
    if (AC_REPEAT) {
      // If AC_REPEAT == true, we have to explicitly call reset_flags() rather than relying
      // on the window to do it for us.
      ac_win_obj.reset_flags();
    }
  
    do {
      bool inside_image = i >= 0 && j >= 0 && i < height && j < num_pack_per_line;
      ac_array<PIX_TYPE, AC_BUS_WORDS> din;
      
      for (int k = 0; k < AC_BUS_WORDS; k++) {
        din[k] = inside_image ? img[i*width + j*AC_BUS_WORDS + k] : 0;
        
        if (AC_REPEAT && repeat_line) {
          din[k] = 255;
        }
      }
      
      #ifdef DEBUG
      cout << "-------------------" << endl;
      cout << "frame_cnt = " << frame_cnt << endl;
      cout << "i = " << i << endl;
      if (AC_REPEAT) {
        cout << "i_total = " << i_total << endl;
        cout << "repeat_line = " << repeat_line << endl;
      }
      cout << "j = " << j << endl;
      cout << "din = " << din << endl;
      cout << "read_data = " << read_data << endl;
      #endif
      
      if (read_data != inside_image && !repeat_line) {
        #ifdef DEBUG
        AC_ASSERT(false, "read_data does not match expected value.");
        #endif
        cout << "FAILED." << endl;
        return false;
      }
      
      ac_array<PIX_TYPE, AC_WIN_HEIGHT, AC_WORDS> dout;
      bool sof_out, sol_out, eol_out;
      typename ac_win_type::VOUT_TYPE vld_out;
      ac_win_obj.run(din, width, height, read_data, dout, sof_out, eof_out, sol_out, eol_out, vld_out, repeat_line);
      
      #ifdef DEBUG
      cout << "dout = " << dout << endl;
      cout << "sof_out = " << sof_out << endl;
      cout << "eof_out = " << eof_out << endl;
      cout << "sol_out = " << sol_out << endl;
      cout << "eol_out = " << eol_out << endl;
      cout << "vld_out = " << vld_out << endl;
      #endif
      
      if (vld_out) {
        for (int k = 0; k < AC_BUS_WORDS; k++) {
          window_cnt++;
          
          if (AC_REPEAT && repeat_line && (window_cnt%width == 0)) {
            window_cnt -= width;
          }
          
          #ifdef DEBUG
          cout << "k = " << k << endl;
          cout << "windows[" << window_cnt << "] = " << windows[window_cnt] << endl;
          cout << "dout = " << dout << endl;
          #endif
          
          for (int r = 0; r < AC_WIN_HEIGHT; r++) {
            for (int c = 0; c < AC_WIN_WIDTH; c++) {
              if (windows[window_cnt][r][c] != dout[r][k + c]) {
                #ifdef DEBUG
                AC_ASSERT(false, "Reference and design output do not match.");
                #endif
                cout << "FAILED." << endl;
                return false;
              }
            }
          }
          
          if (k == 0) {
            if (sol_out != (window_cnt%width == 0)) {
              #ifdef DEBUG
              AC_ASSERT(false, "sol_out does not match expected value.");
              #endif
              cout << "FAILED." << endl;
              return false;
            }
            
            if (sof_out != (window_cnt == 0)) {
              #ifdef DEBUG
              AC_ASSERT(false, "sof_out does not match expected value.");
              #endif
              cout << "FAILED." << endl;
              return false;
            }
          }
          
          if (k == AC_BUS_WORDS - 1) {
            if (eol_out != (window_cnt%width == width - 1)) {
              #ifdef DEBUG
              AC_ASSERT(false, "eol_out does not match expected value.");
              #endif
              cout << "FAILED." << endl;
              return false;
            }
            
            if (eof_out != (window_cnt == width*height - 1)) {
              #ifdef DEBUG
              AC_ASSERT(false, "eof_out does not match expected value.");
              #endif
              cout << "FAILED." << endl;
              return false;
            }
          }
        }
      }
    
      if (j == num_pack_per_line - 1) {
        j = 0;
        i++;
        
        if (AC_REPEAT) {
          i_total++;
          
          if (i_total%den_rep_ratio > num_rep_ratio - 1) {
            repeat_line = true;
          } else {
            repeat_line = false;
          }
          
          // If we're repeating the line, change the value of i back to what it was before it was incremented.
          i -= int(repeat_line);
        }
      } else {
        j++;
      }
    } while (!eof_out);
    
    #ifdef DEBUG
    cout << "window_cnt after frame no. " << (frame_cnt + 1) << " = " << window_cnt << endl;
    #endif
    
    if (window_cnt != width*height - 1) {
      #ifdef DEBUG
      AC_ASSERT(false, "Not all windows were tested.");
      #endif
      cout << "FAILED." << endl;
      return false;
    }
  }
  
  delete[] img;
  delete[] windows;
  
  cout << "PASSED." << endl;
  return true;
}

int main(int argc, char *argv[])
{
  cout << "================================================================================" << endl;
  cout << "------------------------ Running rtest_ac_window_v2.cpp ------------------------" << endl;
  cout << "================================================================================" << endl;

  bool all_tests_pass = true;
  
  all_tests_pass = test_driver_1d<ac_int<16, false>, 8, AC_REFLECT101, 1>(15) && all_tests_pass;
  
  all_tests_pass = test_driver_1d<ac_int<16, false>, 8, AC_REPLICATE, 3, false>(15) && all_tests_pass;
  all_tests_pass = test_driver_1d<ac_int<16, false>, 8, AC_REPLICATE, 3, true>(15) && all_tests_pass;
  all_tests_pass = test_driver_1d<ac_int<16, false>, 8, AC_CONSTANT, 3>(15, 10) && all_tests_pass;
  all_tests_pass = test_driver_1d<ac_int<16, false>, 8, AC_REFLECT101, 3>(15) && all_tests_pass;
  all_tests_pass = test_driver_1d<ac_int<16, false>, 8, AC_REFLECT, 3>(15) && all_tests_pass;
  
  all_tests_pass = test_driver_1d<ac_int<16, false>, 7, AC_REPLICATE, 3, false>(15) && all_tests_pass;
  all_tests_pass = test_driver_1d<ac_int<16, false>, 7, AC_REPLICATE, 3, true>(15) && all_tests_pass;
  all_tests_pass = test_driver_1d<ac_int<16, false>, 7, AC_CONSTANT, 3>(15, 10) && all_tests_pass;
  all_tests_pass = test_driver_1d<ac_int<16, false>, 7, AC_REFLECT101, 3>(15) && all_tests_pass;
  all_tests_pass = test_driver_1d<ac_int<16, false>, 7, AC_REFLECT, 3>(15) && all_tests_pass;
  
  all_tests_pass = test_driver_2d<ac_int<16, false>, 6, 8, AC_SPWRMASK, AC_REFLECT101, 1>(15) && all_tests_pass;
  
  all_tests_pass = test_driver_2d<ac_int<16, false>, 6, 8, AC_SPWRMASK, AC_REPLICATE, 3, false>(15) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 6, 8, AC_SPWRMASK, AC_REPLICATE, 3, true>(15) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 6, 8, AC_SPWRMASK, AC_CONSTANT, 3>(15, 10) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 6, 8, AC_SPWRMASK, AC_REFLECT101, 3>(15) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 6, 8, AC_SPWRMASK, AC_REFLECT, 3>(15) && all_tests_pass;
  
  all_tests_pass = test_driver_2d<ac_int<16, false>, 5, 7, AC_SPWRMASK, AC_REPLICATE, 4, false>(15) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 5, 7, AC_SPWRMASK, AC_REPLICATE, 4, true>(15) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 5, 7, AC_SPWRMASK, AC_CONSTANT, 4>(15, 10) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 5, 7, AC_SPWRMASK, AC_REFLECT101, 4>(15) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 5, 7, AC_SPWRMASK, AC_REFLECT, 4>(15) && all_tests_pass;
  
  all_tests_pass = test_driver_2d<ac_int<16, false>, 5, 7, AC_DUAL, AC_REFLECT, 4>(15) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 5, 7, AC_SPSHIFT, AC_REFLECT, 4>(15) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 5, 7, AC_SPCIR, AC_REFLECT, 4>(15) && all_tests_pass;
  
  all_tests_pass = test_driver_2d<ac_int<16, false>, 5, 7, AC_SPWRMASK, AC_REFLECT, 4, false, true>(15) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 5, 7, AC_DUAL, AC_REFLECT, 4, false, true>(15) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 5, 7, AC_SPSHIFT, AC_REFLECT, 4, false, true>(15) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 5, 7, AC_SPCIR, AC_REFLECT, 4, false, true>(15) && all_tests_pass;
  
  all_tests_pass = test_driver_2d<ac_int<16, false>, 5, 7, AC_SPWRMASK, AC_REFLECT, 4, false, true>(15, 0, 3, 4) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 5, 7, AC_SPWRMASK, AC_REFLECT, 4, false, true>(15, 0, 1, 2) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 5, 7, AC_SPWRMASK, AC_REFLECT, 4, false, true>(30, 0, 5, 8) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 5, 7, AC_SPWRMASK, AC_REFLECT, 4, false, true>(31, 0, 5, 8) && all_tests_pass;

  if (!all_tests_pass) {
    cout << "  Integrated Design - FAILED." << endl;
    return -1;
  }

  cout << "  Integrated Design - PASSED." << endl;
  return 0;
}
