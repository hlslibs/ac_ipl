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
// $MGC_HOME/bin/c++ -std=c++11 -I$MGC_HOME/shared/include rtest_ac_shift_N.cpp -o design
// ./design

#include <ac_ipl/ac_shift_N.h>

#include <ac_math/ac_random.h>
#include <iostream>
using namespace std;

// Print elements of 1D data shifter.
template <class PIX_TYPE, int AC_WIN_WIDTH, ac_padding_method AC_PMODE, int AC_BUS_WORDS>
void print_dshift_1d(ac_shift_N_1d<PIX_TYPE, AC_WIN_WIDTH, AC_PMODE, AC_BUS_WORDS> dshift)
{
  typedef ac_shift_N_1d<PIX_TYPE, AC_WIN_WIDTH, AC_PMODE, AC_BUS_WORDS> dshift_type;
  constexpr int AC_SHIFT_WORDS = dshift_type::AC_SHIFT_WORDS;

  for (int j = 0; j < AC_SHIFT_WORDS; j++) {
    cout << dshift[j] << " ";
  }
}

// Print elements of 2D data shifter.
template <class PIX_TYPE, int AC_WIN_HEIGHT, int AC_WIN_WIDTH, ac_padding_method AC_PMODE, int AC_BUS_WORDS>
void print_dshift_2d(ac_shift_N_2d<PIX_TYPE, AC_WIN_HEIGHT, AC_WIN_WIDTH, AC_PMODE, AC_BUS_WORDS> dshift)
{
  typedef ac_shift_N_2d<PIX_TYPE, AC_WIN_HEIGHT, AC_WIN_WIDTH, AC_PMODE, AC_BUS_WORDS> dshift_type;
  constexpr int AC_SHIFT_HEIGHT = dshift_type::AC_SHIFT_HEIGHT;
  constexpr int AC_SHIFT_WORDS = dshift_type::AC_SHIFT_WORDS;

  for (int i = 0; i < AC_SHIFT_HEIGHT; i++) {
    cout << endl;
    for (int j = 0; j < AC_SHIFT_WORDS; j++) {
      cout << dshift[i][j] << " ";
    }
  }
}

// test_driver_1d tests the subscript operator overloading in ac_shift_N_1d as it is not
// executed by the rtest_ac_window_v2.cpp unit test.
template <class PIX_TYPE, int AC_WIN_WIDTH, ac_padding_method AC_PMODE, int AC_BUS_WORDS>
bool test_driver_1d(int num_iters)
{
  typedef ac_shift_N_1d<PIX_TYPE, AC_WIN_WIDTH, AC_PMODE, AC_BUS_WORDS> dshift_type;
  constexpr int AC_SHIFT_WORDS = dshift_type::AC_SHIFT_WORDS;

  cout << "TEST: ac_shift_N_1d.  PIX_TYPE: ";
  cout.width(18);
  cout << left << PIX_TYPE::type_name();
  cout << "AC_WIN_WIDTH: ";
  cout.width(3);
  cout << left << AC_WIN_WIDTH;
  cout << "AC_BUS_WORDS: ";
  cout.width(3);
  cout << left << AC_BUS_WORDS;
  cout << "AC_SHIFT_WORDS: ";
  cout.width(4);
  cout << left << AC_SHIFT_WORDS;

  // Declare data shifter objects.
  dshift_type lshift_N, rshift_N;
  lshift_N.reset();
  rshift_N.reset();

  // Declare reference arrays and initialize them to 0.
  ac_array<PIX_TYPE, AC_SHIFT_WORDS> lshift_ref, rshift_ref;
  for (int j = 0; j < AC_SHIFT_WORDS; j++) {
    lshift_ref[j] = rshift_ref[j] = 0;
  }

  PIX_TYPE pix_val = 0;

  for (int iter = 0; iter < num_iters; iter++) {
    // Initialize input to shifters.
    ac_array<PIX_TYPE, AC_BUS_WORDS> din;
    for (int k = 0; k < AC_BUS_WORDS; k++) {
      pix_val++;
      din[k] = pix_val;
    }

    // Shift input in to the actual design.
    lshift_N << din;
    rshift_N >> din;

    // Shift input into the reference shift registers.
    for (int j = AC_SHIFT_WORDS - 1; j >= 0; j--) {
      if (j >= AC_BUS_WORDS) {
        lshift_ref[j] = lshift_ref[j - AC_BUS_WORDS];
      } else {
        lshift_ref[j] = din[AC_BUS_WORDS - 1 - j];
      }
    }

    for (int j = 0; j < AC_SHIFT_WORDS; j++) {
      if (j < AC_SHIFT_WORDS - AC_BUS_WORDS) {
        rshift_ref[j] = rshift_ref[j + AC_BUS_WORDS];
      } else {
        rshift_ref[j] = din[j - (AC_SHIFT_WORDS - AC_BUS_WORDS)];
      }
    }

    #ifdef DEBUG
    cout << endl;
    cout << "rshift_ref = " << rshift_ref << endl;
    cout << "rshift_N = ";
    print_dshift_1d(rshift_N);
    cout << endl;
    cout << "lshift_ref = " << lshift_ref << endl;
    cout << "lshift_N = ";
    print_dshift_1d(lshift_N);
    cout << endl;
    #endif

    // Test the reference vs actual subscript operator output for shift
    // register reads.
    for (int j = 0; j < AC_SHIFT_WORDS; j++) {
      if (lshift_ref[j] != lshift_N[j]) {
        #ifdef DEBUG
        AC_ASSERT(false, "lshift_N output does not match reference.");
        #endif
        cout << "FAILED." << endl;
        return false;
      }

      if (rshift_ref[j] != rshift_N[j]) {
        #ifdef DEBUG
        AC_ASSERT(false, "rshift_N output does not match reference.");
        #endif
        cout << "FAILED." << endl;
        return false;
      }
    }
  }

  // Write a randomized set of input values to the shift registers using
  // the subscript operator.
  for (int j = 0; j < AC_SHIFT_WORDS; j++) {
    PIX_TYPE rand_pix_in;
    ac_random(rand_pix_in);
    lshift_N[j] = rand_pix_in;
    rshift_N[j] = rand_pix_in;
    lshift_ref[j] = rand_pix_in;
    rshift_ref[j] = rand_pix_in;
  }

  #ifdef DEBUG
  cout << "rshift_ref = " << rshift_ref << endl;
  cout << "rshift_N = ";
  print_dshift_1d(rshift_N);
  cout << endl;
  cout << "lshift_ref = " << lshift_ref << endl;
  cout << "lshift_N = ";
  print_dshift_1d(lshift_N);
  cout << endl;
  #endif

  // Test the reference vs actual subscript operator output for shift
  // register reads.
  for (int j = 0; j < AC_SHIFT_WORDS; j++) {
    if (lshift_ref[j] != lshift_N[j]) {
      #ifdef DEBUG
      AC_ASSERT(false, "lshift_N output does not match reference.");
      #endif
      cout << "FAILED." << endl;
      return false;
    }

    if (rshift_ref[j] != rshift_N[j]) {
      #ifdef DEBUG
      AC_ASSERT(false, "rshift_N output does not match reference.");
      #endif
      cout << "FAILED." << endl;
      return false;
    }
  }

  cout << "PASSED." << endl;
  return true;
}

// test_driver_2d tests the subscript operator overloading in ac_shift_N_2d as it is not
// executed by the rtest_ac_window_v2.cpp unit test.
template<class PIX_TYPE, int AC_WIN_HEIGHT, int AC_WIN_WIDTH, ac_padding_method AC_PMODE, int AC_BUS_WORDS = 1>
bool test_driver_2d(int num_iters)
{
  typedef ac_shift_N_2d<PIX_TYPE, AC_WIN_HEIGHT, AC_WIN_WIDTH, AC_PMODE, AC_BUS_WORDS> dshift_type;
  constexpr int AC_SHIFT_WORDS = dshift_type::AC_SHIFT_WORDS;
  constexpr int AC_SHIFT_HEIGHT = dshift_type::AC_SHIFT_HEIGHT;

  cout << "TEST: ac_shift_N_2d.  PIX_TYPE: ";
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
  cout << "AC_SHIFT_WORDS: ";
  cout.width(4);
  cout << left << AC_SHIFT_WORDS;
  cout << "AC_SHIFT_HEIGHT: ";
  cout.width(3);
  cout << left << AC_SHIFT_HEIGHT;

  // Declare reference arrays and initialize them to 0.
  ac_array<PIX_TYPE, AC_SHIFT_HEIGHT, AC_SHIFT_WORDS> lshift_ref, rshift_ref;

  for (int i = 0; i < AC_SHIFT_HEIGHT; i++) {
    for (int j = 0; j < AC_SHIFT_WORDS; j++) {
      lshift_ref[i][j] = rshift_ref[i][j] = 0;
    }
  }
  
  PIX_TYPE col_offset = 0;
  int width = num_iters*AC_BUS_WORDS;
  
  // Declare 2D data shifters.
  dshift_type lshift_N, rshift_N;
  lshift_N.reset();
  rshift_N.reset();

  for (int iter = 0; iter < num_iters; iter++) {
    ac_array<PIX_TYPE, AC_SHIFT_HEIGHT, AC_BUS_WORDS> din;

    for (int i = 0; i < AC_SHIFT_HEIGHT; i++) {
      for (int k = 0; k < AC_BUS_WORDS; k++) {
        if (i == 0) {
          col_offset++;
        }
        din[i][k] = i*width + col_offset;
      }
    }

    // Shift input in to the actual design.
    lshift_N << din;
    rshift_N >> din;

    // Shift input into the reference shift registers.
    for (int i = 0; i < AC_SHIFT_HEIGHT; i++) {
      for (int j = AC_SHIFT_WORDS - 1; j >= 0; j--) {
        if (j >= AC_BUS_WORDS) {
          lshift_ref[i][j] = lshift_ref[i][j - AC_BUS_WORDS];
        } else {
          lshift_ref[i][j] = din[i][AC_BUS_WORDS - 1 - j];
        }
      }

      for (int j = 0; j < AC_SHIFT_WORDS; j++) {
        if (j < AC_SHIFT_WORDS - AC_BUS_WORDS) {
          rshift_ref[i][j] = rshift_ref[i][j + AC_BUS_WORDS];
        } else {
          rshift_ref[i][j] = din[i][j - (AC_SHIFT_WORDS - AC_BUS_WORDS)];
        }
      }
    }

    // Test the reference vs actual subscript operator output for shift
    // register reads.
    for (int i = 0; i < AC_SHIFT_HEIGHT; i++) {
      for (int j = 0; j < AC_SHIFT_WORDS; j++) {
        if (lshift_ref[i][j] != lshift_N[i][j]) {
          #ifdef DEBUG
          AC_ASSERT(false, "lshift_N output does not match reference.");
          #endif
          cout << "FAILED." << endl;
          return false;
        }

        if (rshift_ref[i][j] != rshift_N[i][j]) {
          #ifdef DEBUG
          AC_ASSERT(false, "rshift_N output does not match reference.");
          #endif
          cout << "FAILED." << endl;
          return false;
        }
      }
    }

    #ifdef DEBUG
    cout << endl;
    cout << "rshift_ref = " << rshift_ref << endl;
    cout << "rshift_N = ";
    print_dshift_2d(rshift_N);
    cout << endl;
    cout << "lshift_ref = " << lshift_ref << endl;
    cout << "lshift_N = ";
    print_dshift_2d(lshift_N);
    cout << endl;
    #endif
  }

  // Write a randomized set of input values to the shift registers using
  // the subscript operator.
  for (int i = 0; i < AC_SHIFT_HEIGHT; i++) {
    for (int j = 0; j < AC_SHIFT_WORDS; j++) {
      PIX_TYPE rand_pix_in;
      ac_random(rand_pix_in);
      lshift_N[i][j] = rand_pix_in;
      rshift_N[i][j] = rand_pix_in;
      lshift_ref[i][j] = rand_pix_in;
      rshift_ref[i][j] = rand_pix_in;
    }
  }

  // Test the reference vs actual subscript operator output for shift
  // register reads.
  for (int i = 0; i < AC_SHIFT_HEIGHT; i++) {
    for (int j = 0; j < AC_SHIFT_WORDS; j++) {
      if (lshift_ref[i][j] != lshift_N[i][j]) {
        #ifdef DEBUG
        AC_ASSERT(false, "lshift_N output does not match reference.");
        #endif
        cout << "FAILED." << endl;
        return false;
      }

      if (rshift_ref[i][j] != rshift_N[i][j]) {
        #ifdef DEBUG
        AC_ASSERT(false, "rshift_N output does not match reference.");
        #endif
        cout << "FAILED." << endl;
        return false;
      }
    }
  }

  #ifdef DEBUG
  cout << endl;
  cout << "rshift_ref = " << rshift_ref << endl;
  cout << "rshift_N = ";
  print_dshift_2d(rshift_N);
  cout << endl;
  cout << "lshift_ref = " << lshift_ref << endl;
  cout << "lshift_N = ";
  print_dshift_2d(lshift_N);
  cout << endl;
  #endif

  cout << "PASSED." << endl;
  return true;
}

int main(int argc, char *argv[])
{
  cout << "==============================================================================" << endl;
  cout << "------------------------ Running rtest_ac_shift_N.cpp ------------------------" << endl;
  cout << "==============================================================================" << endl;

  bool all_tests_pass = true;

  all_tests_pass = test_driver_1d<ac_int<16, false>, 8, AC_REFLECT101, 1>(10) && all_tests_pass;
  all_tests_pass = test_driver_1d<ac_int<16, false>, 7, AC_REFLECT101, 1>(10) && all_tests_pass;
  all_tests_pass = test_driver_1d<ac_int<16, false>, 8, AC_REFLECT101, 4>(10) && all_tests_pass;
  all_tests_pass = test_driver_1d<ac_int<16, false>, 7, AC_REFLECT101, 3>(10) && all_tests_pass;

  all_tests_pass = test_driver_2d<ac_int<16, false>, 6, 8, AC_REFLECT101, 1>(10) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 5, 7, AC_REFLECT101, 1>(10) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 6, 8, AC_REFLECT101, 4>(10) && all_tests_pass;
  all_tests_pass = test_driver_2d<ac_int<16, false>, 5, 7, AC_REFLECT101, 3>(10) && all_tests_pass;


  if (!all_tests_pass) {
    cout << "  ac_shift_N - FAILED." << endl;
    return -1;
  }

  cout << "  ac_shift_N - PASSED." << endl;
  return 0;
}
