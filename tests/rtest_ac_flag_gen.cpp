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
// Disable the asserts in ac_flag_gen.h so that we can test the dim_change_error() methods.
#define DISABLE_AC_FLAG_GEN_ASSERT

#include <ac_ipl/ac_flag_gen.h>

#include <iostream>
using namespace std;

template<int AC_WIDTH, int AC_BUS_WORDS>
bool test_fg_1d (
  ac_flag_gen_1d<AC_WIDTH, AC_BUS_WORDS> &fg_1d_obj,
  int width, int num_lines
) {
  // Number of parallel packed data per line.
  int num_data_per_line = width/AC_BUS_WORDS;

  // Test 1D flag generation, assuming regular operation.
  for (int i = 0; i < num_lines; i++) {
    for (int j = 0; j < num_data_per_line; j++) {
      bool sol, eol;
      fg_1d_obj.run(width, sol, eol);
      
      if (fg_1d_obj.dim_change_error()) {
        cerr << "FAILED." << endl;
        #ifdef DEBUG
        cerr << "Unexpected dimension change." << endl;
        cerr << "Function: test_fg_1d()." << endl;
        cerr << "i = " << i << endl;
        cerr << "j = " << j << endl;
        #endif
        return true;
      }
      
      if (sol != (j == 0)) {
        cerr << "FAILED." << endl;
        #ifdef DEBUG
        cerr << "Unexpected sol value encountered." << endl;
        cerr << "Function: test_fg_1d()." << endl;
        cerr << "i = " << i << endl;
        cerr << "j = " << j << endl;
        cerr << "sol = " << sol << endl;
        #endif
        return true;
      }
      
      if (eol != (j == num_data_per_line - 1)) {
        cerr << "FAILED." << endl;
        #ifdef DEBUG
        cerr << "Unexpected eol value encountered." << endl;
        cerr << "Function: test_fg_1d()." << endl;
        cerr << "i = " << i << endl;
        cerr << "j = " << j << endl;
        cerr << "eol = " << eol << endl;
        #endif
        return true;
      }
    }
  }
  
  return false;
}

template<int AC_WIDTH, int AC_HEIGHT, int AC_BUS_WORDS>
bool test_fg_2d (
  ac_flag_gen_2d<AC_WIDTH, AC_HEIGHT, AC_BUS_WORDS> &fg_2d_obj,
  int width, int height
) {
  // Number of parallel packed data per line.
  int num_data_per_line = width/AC_BUS_WORDS;
  
  // Test 2D flag generation, assuming regular operation.
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < num_data_per_line; j++) {
      bool sof, eof, sol, eol;
      
      fg_2d_obj.run(width, height, sof, eof, sol, eol);
      
      if (fg_2d_obj.dim_change_error()) {
        cerr << "FAILED." << endl;
        #ifdef DEBUG
        cerr << "Unexpected dimension change." << endl;
        cerr << "Function: test_fg_2d()." << endl;
        cerr << "i = " << i << endl;
        cerr << "j = " << j << endl;
        #endif
        return true;
      }
      
      if (sol != (j == 0)) {
        cerr << "FAILED." << endl;
        #ifdef DEBUG
        cerr << "Unexpected sol value encountered." << endl;
        cerr << "Function: test_fg_2d()." << endl;
        cerr << "i = " << i << endl;
        cerr << "j = " << j << endl;
        cerr << "sol = " << sol << endl;
        #endif
        return true;
      }
      
      if (sof != (i == 0 && sol)) {
        cerr << "FAILED." << endl;
        #ifdef DEBUG
        cerr << "Unexpected sof value encountered." << endl;
        cerr << "Function: test_fg_2d()." << endl;
        cerr << "i = " << i << endl;
        cerr << "j = " << j << endl;
        cerr << "sof = " << sof << endl;
        #endif
        return true;
      }
      
      if (eol != (j == num_data_per_line - 1)) {
        cerr << "FAILED." << endl;
        #ifdef DEBUG
        cerr << "Unexpected eol value encountered." << endl;
        cerr << "Function: test_fg_2d()." << endl;
        cerr << "i = " << i << endl;
        cerr << "j = " << j << endl;
        cerr << "eol = " << eol << endl;
        #endif
        return true;
      }
      
      if (eof != (i == height - 1 && eol)) {
        cerr << "FAILED." << endl;
        #ifdef DEBUG
        cerr << "Unexpected eof value encountered." << endl;
        cerr << "Function: test_fg_2d()." << endl;
        cerr << "i = " << i << endl;
        cerr << "j = " << j << endl;
        cerr << "eof = " << eof << endl;
        #endif
        return true;
      }
    }
  }
  
  return false;
}

template<int AC_WIDTH, int AC_HEIGHT, int AC_BUS_WORDS>
bool test_driver(int width, int height, int num_lines)
{
  cout << "TEST: ac_flag_gen AC_WIDTH: ";
  cout.width(5);
  cout << left << AC_WIDTH;
  cout << "AC_HEIGHT: ";
  cout.width(5);
  cout << left << AC_HEIGHT;
  cout << "AC_BUS_WORDS: ";
  cout.width(2);
  cout << left << AC_BUS_WORDS;
  cout << "width: ";
  cout.width(5);
  cout << left << width;
  cout << "height: ";
  cout.width(5);
  cout << left << height;
  cout << "num_lines: ";
  cout.width(3);
  cout << left << num_lines;
  cout << "RESULT: ";
  
  ac_flag_gen_1d<AC_WIDTH, AC_BUS_WORDS> fg_1d_obj;
  ac_flag_gen_2d<AC_WIDTH, AC_HEIGHT, AC_BUS_WORDS> fg_2d_obj;

  // Carry out an ac_flag_gen_1d test with regular operation.
  if (test_fg_1d<AC_WIDTH, AC_BUS_WORDS>(fg_1d_obj, width, num_lines)) {
    return false;
  }
  
  #ifdef DEBUG
  cout << "Test for regular ac_flag_gen_1d operation passed." << endl;
  #endif
  
  // Number of parallel packed data per line.
  int num_data_per_line = width/AC_BUS_WORDS;
  
  // The ceil operator is there to make sure that the wrong width is a multiple of AC_BUS_WORDS.
  // It should still roughly correspond to width/3.
  int wrong_width = ceil(double(width/3)/double(AC_BUS_WORDS))*AC_BUS_WORDS;
  
  // Sending half a row full of pixels to ac_flag_gen_1d with incorrect d imension input
  // (i.e. width/3 instead of width)
  for (int j = 0; j < num_data_per_line/2; j++) {
    bool sol, eol;
    fg_1d_obj.run(wrong_width, sol, eol);
  }
  
  // If you change the dimension back to the correct one, the dim_change_error() method should
  // return "true" because, while the dimension is now correct, you also changed it mid-line
  // which is incorrect.
  bool sol_dummy, eol_dummy;
  fg_1d_obj.run(width, sol_dummy, eol_dummy);
  
  if (!fg_1d_obj.dim_change_error()) {
    cerr << "FAILED." << endl;
    return false;
  }
  
  #ifdef DEBUG
  cout << "Test for ac_flag_gen_1d operation after sending incorrect dimensions passed." << endl;
  #endif
  
  // Carry out an ac_flag_gen_2d test with regular operation.
  if (test_fg_2d<AC_WIDTH, AC_HEIGHT, AC_BUS_WORDS>(fg_2d_obj, width, height)) {
    return false;
  }
  
  #ifdef DEBUG
  cout << "Test for regular ac_flag_gen_2d operation passed." << endl;
  #endif
  
  // Send half a row full of pixels with the incorrect height input, i.e. height/3.
  for (int j = 0; j < num_data_per_line/2; j++) {
    bool sof, eof, sol, eol;
    fg_2d_obj.run(width, height/3, sof, eof, sol, eol);
  }
  
  // If you change the dimensions back to the correct one, the dim_change_error() method should
  // return "true" because, while the dimension is now correct, you also changed it mid-line
  // which is incorrect.
  bool sof_dummy, eof_dummy;
  fg_2d_obj.run(width, height, sof_dummy, eof_dummy, sol_dummy, eol_dummy);https://www.youtube.com/shorts/P3gIi9su2Rw
  
  if (!fg_2d_obj.dim_change_error()) {
    cerr << "FAILED." << endl;
    return false;
  }
  
  #ifdef DEBUG
  cout << "Test for ac_flag_gen_2d operation after sending incorrect height passed." << endl;
  #endif
  
  fg_2d_obj.reset();
  
  // Send a third of a row full of pixels with incorrect dimension inputs, i.e. width/3 and height/3.
  int num_data_per_line_incorrect = wrong_width/AC_BUS_WORDS;
  
  for (int j = 0; j < num_data_per_line_incorrect; j++) {
    bool sof, eof, sol, eol;
    fg_2d_obj.run(wrong_width, height/3, sof, eof, sol, eol);
  }
  
  // If you change the dimensions back to the correct one, the dim_change_error() method should
  // return "true" because, while the dimension is now correct, you also changed it mid-line
  // which is incorrect.
  fg_2d_obj.run(width, height, sof_dummy, eof_dummy, sol_dummy, eol_dummy);
  
  if (!fg_2d_obj.dim_change_error()) {
    cerr << "FAILED." << endl;
    return false;
  }
  
  #ifdef DEBUG
  cout << "Test for ac_flag_gen_2d operation after sending incorrect dimensions passed." << endl;
  #endif
  
  cout << "PASSED." << endl;
  
  return true;
}

int main(int argv, char **argc) {
  cout << "==============================================================================" << endl;
  cout << "--------------------- Running rtest_ac_flag_gen.cpp --------------------------" << endl;
  cout << "==============================================================================" << endl;
  
  bool all_tests_pass = true;
  
  all_tests_pass = test_driver<1024, 1024, 1>(1024, 1024, 10) && all_tests_pass;
  all_tests_pass = test_driver<1024, 1024, 1>( 512,  512, 10) && all_tests_pass;
  all_tests_pass = test_driver<1024,  512, 1>(1024,  512, 10) && all_tests_pass;
  all_tests_pass = test_driver<1024,  512, 1>( 512,  256, 10) && all_tests_pass;
  
  all_tests_pass = test_driver<1024, 1024, 4>(1024, 1024, 10) && all_tests_pass;
  all_tests_pass = test_driver<1024, 1024, 4>( 512,  512, 10) && all_tests_pass;
  all_tests_pass = test_driver<1024,  512, 4>( 512,  256, 10) && all_tests_pass;
  all_tests_pass = test_driver<1024,  512, 4>(1024,  512, 10) && all_tests_pass;
  
  all_tests_pass = test_driver<1025, 1024, 5>(1025, 1024, 10) && all_tests_pass;
  all_tests_pass = test_driver<1025, 1024, 5>( 515,  512, 10) && all_tests_pass;
  all_tests_pass = test_driver<1025,  512, 5>( 515,  256, 10) && all_tests_pass;
  all_tests_pass = test_driver<1025,  512, 5>(1025,  512, 10) && all_tests_pass;
  
  cout << "  Testbench finished." << endl;
  
  if (!all_tests_pass) {
    cerr << "  ac_flag_gen - FAILED" << endl;
    return -1;
  }
  
  return 0;
}
