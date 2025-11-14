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
// $MGC_HOME/bin/c++ -std=c++11 -I$MGC_HOME/shared/include rtest_ac_window_v2_flush.cpp -o design
// ./design

#include <ac_fixed.h>
#include <ac_ipl/ac_window_v2_flush.h>

#include <random>
#include <string>
#include <iostream>
using namespace std;

template <class T, unsigned MAX_SZ>
void print_arr_1d(ac_array<T, MAX_SZ> arr, int sz)
{
  cout << "{";

  for (int i = 0; i < sz; i++) {
    cout << arr[i];
    if (i != sz - 1) {
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

// stall_patterns_2: Defines the pattern of stalls on the secondary inputs while flushing.
enum stall_patterns_2 {
  EVEN_STALL, // Stall on even column iterations, i.e. j = 0, 2, 4, etc.
  ODD_STALL, // Stall on odd column iterations, i.e. j = 1, 3, 5, etc.
  SECOND_ROW_STALL, // Stall on the second row of flushed outputs.
  RANDOM_STALL, // Stall randomly. Chance is defined by the stall_chance input to test_driver_2d()
  NEVER_STALL,
  ALWAYS_STALL,
  PRESET_STALL
};

template <stall_patterns_2 stall_pattern_2, bool AC_REPEAT = false>
void gen_advance_flags(
  bool inside_image_1,
  bool dont_read_data,
  bool preset_flush_stall,
  double stall_chance,
  int i_1, int j_1, int height,
  bool &advance_1, bool &advance_2,
  bool repeat_line = false
)
{
  static default_random_engine generator;
  static uniform_real_distribution<double> distribution(0.0, 100.0);

  double rand_val_1 = distribution(generator);
  double rand_val_2 = distribution(generator);
  
  advance_1 = inside_image_1 ? (!dont_read_data && rand_val_1 >= stall_chance) : true;
  advance_2 = false;

  switch (stall_pattern_2) {
    case EVEN_STALL:
      advance_2 = inside_image_1 ? false : (j_1%2 != 0);
      break;
    case ODD_STALL:
      advance_2 = inside_image_1 ? false : (j_1%2 == 0);
      break;
    case SECOND_ROW_STALL:
      advance_2 = inside_image_1 ? false : (i_1 == height + 1);
      break;
    case RANDOM_STALL:
      advance_2 = inside_image_1 ? false : (rand_val_2 >= stall_chance);
      break;
    case NEVER_STALL:
      advance_2 = inside_image_1 ? false : true;
      break;
    case PRESET_STALL:
      advance_2 = inside_image_1 ? false : !preset_flush_stall;
      break;
    default: // ALWAYS_STALL
      advance_2 = false;
      break;
  }

  if (AC_REPEAT && repeat_line) {
    advance_1 = true; // Pointers for the primary frame must advance if repeat_line = true.
    advance_2 = !inside_image_1; // Advance secondary frame pointers if we're in the flushing region and repeat_line = true.
  } else if (!inside_image_1) {
    advance_2 = advance_2 && !dont_read_data;
  }
}

// stall_chance: % chance of random stall.
template <class PIX_TYPE, int AC_WIN_HEIGHT, int AC_WIN_WIDTH, ac_buff_arch_flush AC_BUFF_ARCH, ac_padding_method AC_PMODE, int AC_BUS_WORDS = 1, stall_patterns_2 stall_pattern_2 = RANDOM_STALL, bool AC_REPEAT = false>
bool test_driver_2d(int height, int num_rep_ratio = 3, int den_rep_ratio = 5, double stall_chance = 30.0, int padded_val = 0)
{
  AC_ASSERT(stall_chance >= 0, "stall_chance must be positive.");
  AC_ASSERT(stall_chance < 100, "stall_chance must not equal or exceed 100.");

  const int width = 10*AC_BUS_WORDS;

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

  cout.width(10);

  if (AC_BUFF_ARCH == AC_1R1W_FLUSH) {
    cout << left << "1R1W";
  }

  if (AC_BUFF_ARCH == AC_SPWRMASK_FLUSH) {
    cout << left << "SPWRMASK";
  }

  if (AC_BUFF_ARCH == AC_SPCIR_FLUSH) {
    cout << left << "SPCIR";
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

  if (stall_pattern_2 == SECOND_ROW_STALL) {
    cout.width(24);
  } else {
    cout.width(20);
  }

  if (stall_pattern_2 == EVEN_STALL) {
    cout << left << "sp2 = EVEN_STALL";
  }
  if (stall_pattern_2 == ODD_STALL) {
    cout << left << "sp2 = ODD_STALL";
  }
  if (stall_pattern_2 == SECOND_ROW_STALL) {
    cout << left << "sp2 = SECOND_ROW_STALL";
  }
  if (stall_pattern_2 == RANDOM_STALL) {
    cout << left << "sp2 = RANDOM_STALL";
  }
  if (stall_pattern_2 == NEVER_STALL) {
    cout << left << "sp2 = NEVER_STALL";
  }
  if (stall_pattern_2 == ALWAYS_STALL) {
    cout << left << "sp2 = ALWAYS_STALL";
  }
  if (stall_pattern_2 == PRESET_STALL) {
    cout << left << "sp2 = PRESET_STALL";
  }

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
            if ((AC_PMODE == AC_REPLICATE) || (AC_PMODE == AC_REFLECT101) || (AC_PMODE == AC_REFLECT)) {
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

  constexpr bool HIGH_SPEED_REPLICATE = false;

  typedef ac_window_v2_flush_2d<PIX_TYPE, AC_IMG_HEIGHT, AC_IMG_WIDTH, AC_WIN_HEIGHT, AC_WIN_WIDTH, AC_BUFF_ARCH, AC_PMODE, AC_BUS_WORDS, HIGH_SPEED_REPLICATE, AC_REPEAT> ac_win_type;

  enum {
    AC_WORDS = ac_win_type::AC_WORDS,
  };

  ac_win_type ac_win_obj(padded_val);

  default_random_engine generator;
  uniform_real_distribution<double> distribution(0.0, 100.0);

  constexpr int TOTAL_FRAMES = 3;

  int i_2 = 0, j_2 = 0;

  // Set the frame_cnt, i1 and j1 values for when the window will stall while flushing with the PRESET_STALL pattern.
  int preset_flush_stall_f[] =  { 0,  0,  0,  0,  0,  1,  1,  1};
  int preset_flush_stall_i1[] = {15, 15, 15, 16, 16, 15, 15, 16};
  int preset_flush_stall_j1[] = { 0,  2,  5,  0,  3,  3,  5,  1};

  constexpr int stall_f_sz = sizeof(preset_flush_stall_f)/sizeof(int);
  constexpr int stall_i_sz = sizeof(preset_flush_stall_i1)/sizeof(int);
  constexpr int stall_j_sz = sizeof(preset_flush_stall_j1)/sizeof(int);

  static_assert(stall_f_sz == stall_i_sz, "stall_f_sz not equal to stall_i_sz");
  static_assert(stall_f_sz == stall_j_sz, "stall_f_sz not equal to stall_j_sz");

  bool dont_read_data = false;

  for (int frame_cnt = 0; frame_cnt < TOTAL_FRAMES; frame_cnt++) {
    int i_1 = i_2, j_1 = j_2;
    i_2 = j_2 = 0;

    bool eof_out = false;
    window_cnt = -1;

    // The two variables below are used only if AC_REPEAT = true.
    int i_total = 0; // Counter for all the lines, including repeated ones.
    bool repeat_line = false; // Are we repeating a line?

    if (AC_REPEAT) {
      // If AC_REPEAT == true, we have to explicitly call switch_frames() rather than relying
      // on the window to do it for us.
      dont_read_data = ac_win_obj.switch_frames();
    }

    do {
      bool inside_image_1 = i_1 >= 0 && j_1 >= 0 && i_1 < height && j_1 < num_pack_per_line;
      bool preset_flush_stall = false;

      if (stall_pattern_2 == PRESET_STALL) {
        for (int stall_idx = 0; stall_idx < stall_f_sz; stall_idx++) {
          if (preset_flush_stall_f[stall_idx] == frame_cnt && preset_flush_stall_i1[stall_idx] == i_1 && preset_flush_stall_j1[stall_idx] == j_1) {
            preset_flush_stall = true;
            break;
          }
        }
      }

      bool advance_1 = false, advance_2 = false;
      gen_advance_flags<stall_pattern_2, AC_REPEAT>(inside_image_1, dont_read_data, preset_flush_stall, stall_chance, i_1, j_1, height, advance_1, advance_2, repeat_line);

      #ifdef DEBUG
      cout << "-------------------" << endl;
      cout << "frame_cnt = " << frame_cnt << endl;
      cout << "advance_1 = " << advance_1 << endl;
      cout << "i_1 = " << i_1 << endl;
      cout << "j_1 = " << j_1 << endl;
      cout << "advance_2 = " << advance_2 << endl;
      cout << "i_2 = " << i_2 << endl;
      cout << "j_2 = " << j_2 << endl;
      cout << "inside_image_1 = " << inside_image_1 << endl;
      if (AC_REPEAT) {
        cout << "i_total = " << i_total << endl;
        cout << "repeat_line = " << repeat_line << endl;
      }
      #endif

      ac_array<PIX_TYPE, AC_BUS_WORDS> din;

      for (int k = 0; k < AC_BUS_WORDS; k++) {
        din[k] = 0;

        if (inside_image_1) {
          din[k] = advance_1 ? img[i_1*width + j_1*AC_BUS_WORDS + k] : 0;
        } else if (frame_cnt != TOTAL_FRAMES - 1) {
          din[k] = advance_2 ? img[i_2*width + j_2*AC_BUS_WORDS + k] : 0;
        }

        if (AC_REPEAT && repeat_line) {
          din[k] = 255;
        }
      }

      bool write = inside_image_1 ? advance_1 : advance_2;

      #ifdef DEBUG
      cout << "din = " << din << endl;
      cout << "write = " << write << endl;
      cout << "dont_read_data = " << dont_read_data << endl;
      #endif

      ac_array<PIX_TYPE, AC_WIN_HEIGHT, AC_WORDS> dout;
      bool sof_out, sol_out, eol_out;
      bool vld_out;
      ac_win_obj.run(din, write, width, height, dout, sof_out, eof_out, sol_out, eol_out, vld_out, dont_read_data, repeat_line);

      #ifdef DEBUG
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

      bool repeat_line_prev = repeat_line;

      if (AC_REPEAT) {
        if (advance_1 && (j_1 == num_pack_per_line - 1)) {
          i_total++;

          constexpr int EXTRA_LINE = ac_win_type::EXTRA_LINE;
          constexpr int num_rampdown_lines = AC_WIN_HEIGHT/2 - int(AC_WIN_HEIGHT%2 == 0) + EXTRA_LINE;

          if ((i_1 == height + num_rampdown_lines - 1) || (i_total%den_rep_ratio <= num_rep_ratio - 1)) {
            repeat_line = false;
          } else {
            repeat_line = true;
          }
        }
      }

      if (advance_1) {
        if (j_1 == num_pack_per_line - 1) {
          j_1 = 0;
          i_1 += int(!AC_REPEAT || !repeat_line_prev);
        } else {
          j_1++;
        }
      }

      if (advance_2 && (!AC_REPEAT || !repeat_line_prev)) {
        if (j_2 == num_pack_per_line - 1) {
          j_2 = 0;
          i_2++;
        } else {
          j_2++;
        }
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
  cout << "==============================================================================" << endl;
  cout << "-------------------- Running rtest_ac_window_v2_flush.cpp --------------------" << endl;
  cout << "==============================================================================" << endl;

  bool all_tests_pass = true;

  // 6x8 Window, AC_REPLICATE boundary handling.
  all_tests_pass = test_driver_2d<ac_int<16, false>, 6, 8, AC_1R1W_FLUSH, AC_REPLICATE, 3, PRESET_STALL>(15) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 6, 8, AC_SPCIR_FLUSH, AC_REPLICATE, 3, PRESET_STALL>(15) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 6, 8, AC_1R1W_FLUSH, AC_REPLICATE, 3, RANDOM_STALL>(15) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 6, 8, AC_SPCIR_FLUSH, AC_REPLICATE, 3, RANDOM_STALL>(15) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 6, 8, AC_SPWRMASK_FLUSH, AC_REPLICATE, 3, RANDOM_STALL>(15) && all_tests_pass;

  // 6x8 Window, AC_REFLECT101 boundary handling.
  all_tests_pass = test_driver_2d<ac_int<16, false>, 6, 8, AC_1R1W_FLUSH, AC_REFLECT101, 1, PRESET_STALL>(15) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 6, 8, AC_1R1W_FLUSH, AC_REFLECT101, 3, PRESET_STALL>(15) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 6, 8, AC_SPCIR_FLUSH, AC_REFLECT101, 3, PRESET_STALL>(15) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 6, 8, AC_1R1W_FLUSH, AC_REFLECT101, 3, RANDOM_STALL>(15) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 6, 8, AC_SPCIR_FLUSH, AC_REFLECT101, 3, RANDOM_STALL>(15) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 6, 8, AC_SPWRMASK_FLUSH, AC_REFLECT101, 3, RANDOM_STALL>(15) && all_tests_pass;

  // 5x7 Window, AC_REPLICATE boundary handling.
  all_tests_pass = test_driver_2d<ac_int<16, false>, 5, 7, AC_1R1W_FLUSH, AC_REPLICATE, 4, PRESET_STALL>(15) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 5, 7, AC_SPCIR_FLUSH, AC_REPLICATE, 4, PRESET_STALL>(15) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 5, 7, AC_1R1W_FLUSH, AC_REPLICATE, 4, RANDOM_STALL>(15) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 5, 7, AC_SPCIR_FLUSH, AC_REPLICATE, 4, RANDOM_STALL>(15) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 5, 7, AC_SPWRMASK_FLUSH, AC_REPLICATE, 4, RANDOM_STALL>(15) && all_tests_pass;

  // 5x7 Window, AC_REFLECT101 boundary handling.
  all_tests_pass = test_driver_2d<ac_int<16, false>, 5, 7, AC_1R1W_FLUSH, AC_REFLECT101, 4, PRESET_STALL>(15) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 5, 7, AC_SPCIR_FLUSH, AC_REFLECT101, 4, PRESET_STALL>(15) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 5, 7, AC_1R1W_FLUSH, AC_REFLECT101, 4, RANDOM_STALL>(15) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 5, 7, AC_SPCIR_FLUSH, AC_REFLECT101, 4, RANDOM_STALL>(15) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 5, 7, AC_SPWRMASK_FLUSH, AC_REFLECT101, 4, RANDOM_STALL>(15) && all_tests_pass;

  // 6x8 Window, AC_REPLICATE boundary handling, line repetition, rr = default (3/5).
  all_tests_pass = test_driver_2d<ac_int<16, false>, 6, 8, AC_1R1W_FLUSH, AC_REPLICATE, 3, PRESET_STALL, true>(15) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 6, 8, AC_SPCIR_FLUSH, AC_REPLICATE, 3, PRESET_STALL, true>(15) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 6, 8, AC_1R1W_FLUSH, AC_REPLICATE, 3, RANDOM_STALL, true>(15) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 6, 8, AC_SPCIR_FLUSH, AC_REPLICATE, 3, RANDOM_STALL, true>(15) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 6, 8, AC_SPWRMASK_FLUSH, AC_REPLICATE, 3, RANDOM_STALL, true>(15) && all_tests_pass;

  // 6x8 Window, AC_REFLECT101 boundary handling, line repetition, rr = default (3/5).
  all_tests_pass = test_driver_2d<ac_int<16, false>, 6, 8, AC_1R1W_FLUSH, AC_REFLECT101, 1, PRESET_STALL, true>(15) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 6, 8, AC_1R1W_FLUSH, AC_REFLECT101, 3, PRESET_STALL, true>(15) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 6, 8, AC_SPCIR_FLUSH, AC_REFLECT101, 3, PRESET_STALL, true>(15) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 6, 8, AC_1R1W_FLUSH, AC_REFLECT101, 3, RANDOM_STALL, true>(15) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 6, 8, AC_SPCIR_FLUSH, AC_REFLECT101, 3, RANDOM_STALL, true>(15) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 6, 8, AC_SPWRMASK_FLUSH, AC_REFLECT101, 3, RANDOM_STALL, true>(15) && all_tests_pass;

  // 6x8 Window, AC_REPLICATE boundary handling, line repetition, rr = 2/3.
  all_tests_pass = test_driver_2d<ac_int<16, false>, 6, 8, AC_1R1W_FLUSH, AC_REPLICATE, 3, PRESET_STALL, true>(15, 2, 3) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 6, 8, AC_SPCIR_FLUSH, AC_REPLICATE, 3, PRESET_STALL, true>(15, 2, 3) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 6, 8, AC_1R1W_FLUSH, AC_REPLICATE, 3, RANDOM_STALL, true>(15, 2, 3) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 6, 8, AC_SPCIR_FLUSH, AC_REPLICATE, 3, RANDOM_STALL, true>(15, 2, 3) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 6, 8, AC_SPWRMASK_FLUSH, AC_REPLICATE, 3, RANDOM_STALL, true>(15, 2, 3) && all_tests_pass;

  // 6x8 Window, AC_REFLECT101 boundary handling, line repetition, rr = 2/3.
  all_tests_pass = test_driver_2d<ac_int<16, false>, 6, 8, AC_1R1W_FLUSH, AC_REFLECT101, 1, PRESET_STALL, true>(15, 2, 3) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 6, 8, AC_1R1W_FLUSH, AC_REFLECT101, 3, PRESET_STALL, true>(15, 2, 3) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 6, 8, AC_SPCIR_FLUSH, AC_REFLECT101, 3, PRESET_STALL, true>(15, 2, 3) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 6, 8, AC_1R1W_FLUSH, AC_REFLECT101, 3, RANDOM_STALL, true>(15, 2, 3) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 6, 8, AC_SPCIR_FLUSH, AC_REFLECT101, 3, RANDOM_STALL, true>(15, 2, 3) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 6, 8, AC_SPWRMASK_FLUSH, AC_REFLECT101, 3, RANDOM_STALL, true>(15, 2, 3) && all_tests_pass;

  if (!all_tests_pass) {
    cout << "  Integrated Design - FAILED." << endl;
    return -1;
  }

  cout << "  Integrated Design - PASSED." << endl;
  return 0;
}
