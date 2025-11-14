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
 *  Copyright 2021 Siemens                                                *
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
//******************************************************************************************
// Description:
// Usage:
//   $MGC_HOME/bin/c++ -std=c++11 -I$MGC_HOME/shared/include ac_ctc_lutgen.cpp -o ac_ctc_lutgen
//   ./ac_ctc_lutgen
//  results in a text file ac_ctc_lut_values.txt which can be pasted into
//  a locally modified version of ac_ctc.h.
// Notes:
// Revision History:
//    3.4.3  - dgb - Updated compiler checks to work with MS VS 2019
//******************************************************************************************

// The LUT generator file uses static_asserts, which are only supported by C++11 or later compiler standards.
// The #error directive below informs the user if they're not using those standards.
#if (defined(__GNUC__) && (__cplusplus < 201103L))
#error Please use C++11 or a later standard for compilation.
#endif
#if (defined(_MSC_VER) && (_MSC_VER < 1920) && !defined(__EDG__))
#error Please use Microsoft VS 2019 or a later standard for compilation.
#endif

#include<ac_fixed.h>
#include<ac_ipl/ac_pixels.h>
#include<stdio.h>
#include<string>
#include<sstream>
#include<fstream>
#include<cmath>
#include<math.h>
#include<iostream>
using namespace std;

double sat_val(double in)
{
  // Saturate and round double input for storage in an 8-bit unsigned ac_int.
  return ((ac_fixed<8, 8, false, AC_RND, AC_SAT>)in).to_double();
}

int main()
{
  // Define constant enum parameters here
  enum {
    tempLimitLow = 1000, // Lower temperature limit supported by LUTs
    tempLimitUpp = 13700, // Upper temperature limit supported by LUTs
    tempInc = 100, // Increment temperature from one LUT value to the next
    sizeTable = (tempLimitUpp - tempLimitLow)/tempInc + 1, // Size of LUT
  };

  // The LUT generator only supports temperature ranges that are perfectly divisible by the increment.
  // This makes for easier indexing later.
  static_assert((tempLimitUpp - tempLimitLow)%tempInc == 0, "Please choose perfectly divisible increment");

  //The output LUT values will be printed to a file in c++ syntax.
  //Define the filename below. The new file will be in the same folder
  //as this .cpp file
  const char filename[] = "ac_ctc_lut_values.txt";

  ofstream outfile(filename);

  // Define datatype of LUT
  typedef ac_ipl::RGB_imd<ac_fixed<8, 8, false> > refValsType;
  
  refValsType refVals[sizeTable];
  int temp_table[sizeTable]; // Store temperature values corresponding to each LUT value here.
  
  int table_ind = -1;

  for (double temp = tempLimitLow; temp <= tempLimitUpp; temp += tempInc) {
    table_ind++;
    temp_table[table_ind] = temp;

    double temp_norm = temp/100;

    // Calculate red reference value
    if (temp_norm <= 66) {
      refVals[table_ind].R = 255;
    } else {
      double red_temp = temp_norm - 60;
      red_temp = 329.698727446*pow(red_temp, -0.1332047592);
      refVals[table_ind].R = sat_val(red_temp);
    }

    // Calculate green referenace value.
    if (temp_norm <= 66) {
      double green_temp = temp_norm;
      green_temp = 99.4708025861*log(green_temp) - 161.1195681661;
      refVals[table_ind].G = sat_val(green_temp);
    } else {
      double green_temp = temp_norm - 60;
      green_temp = 288.1221695283*pow(green_temp, -0.0755148492);
      refVals[table_ind].G = sat_val(green_temp);
    }

    // Calculate blue reference value.
    if (temp_norm >= 66) {
      refVals[table_ind].B = 255;
    } else if (temp_norm <= 19) {
      refVals[table_ind].B = 0;
    } else {
      double blue_temp = temp_norm - 10;
      blue_temp = 138.5177312231*log(blue_temp) - 305.0447927307;
      refVals[table_ind].B = sat_val(blue_temp);
    }
  }

  outfile << "enum {" << endl;
  outfile << "  tempLimitLow = " << tempLimitLow << "," << endl;
  outfile << "  tempLimitUpp = " << tempLimitUpp << "," << endl;
  outfile << "  tempInc = " << tempInc << "," << endl;
  outfile << "  sizeTable = (tempLimitUpp - tempLimitLow)/tempInc + 1," << endl;
  outfile << "};" << endl << endl;

  outfile << "typedef ac_fixed<8, 8, false> refValsType;" << endl << endl;

  // Print out array of red lut values.
  outfile << "const refValsType refValsR[sizeTable] = {" << endl;

  for (int i = 0; i < sizeTable; i++) {
    outfile << "  ";
    outfile.width(6);
    if (i != sizeTable - 1) {
      string str_val_concat = refVals[i].R.to_string(AC_DEC) + ".0,";
      outfile << left << str_val_concat;
    } else {
      string str_val_concat = refVals[i].R.to_string(AC_DEC) + ".0 ";
      outfile << left << str_val_concat;
    }
    outfile << " // T = " << temp_table[i] << " K" << endl;
  }

  outfile << "};" << endl << endl;

  // Print out array of green lut values.
  outfile << "const refValsType refValsG[sizeTable] = {" << endl;

  for (int i = 0; i < sizeTable; i++) {
    outfile << "  ";
    outfile.width(6);
    if (i != sizeTable - 1) {
      string str_val_concat = refVals[i].G.to_string(AC_DEC) + ".0,";
      outfile << left << str_val_concat;
    } else {
      string str_val_concat = refVals[i].G.to_string(AC_DEC) + ".0 ";
      outfile << left << str_val_concat;
    }
    outfile << " // T = " << temp_table[i] << " K" << endl;
  }

  outfile << "};" << endl << endl;

  // Print out array of blue lut values.
  outfile << "const refValsType refValsB[sizeTable] = {" << endl;

  for (int i = 0; i < sizeTable; i++) {
    outfile << "  ";
    outfile.width(6);
    if (i != sizeTable - 1) {
      string str_val_concat = refVals[i].B.to_string(AC_DEC) + ".0,";
      outfile << left << str_val_concat;
    } else {
      string str_val_concat = refVals[i].B.to_string(AC_DEC) + ".0 ";
      outfile << left << str_val_concat;
    }
    outfile << " // T = " << temp_table[i] << " K" << endl;
  }

  outfile << "};";

  outfile.close();

  cout << "Values are written, check \"" << filename << "\" for the required LUT values." << endl;

  return 0;
}
