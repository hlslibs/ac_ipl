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
 *  Copyright 2019 Siemens                                                *
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
#include <ac_ipl/ac_imhist.h>

#include <string>
#include <fstream>
#include <iostream>
using namespace std;
#include <bmpUtil/bmp_io.hpp>
#include <bmpUtil/bmp_io.cpp>

// This struct tells you whether the input image is an RGB image or not.
// Generalized struct.
template <class T>
struct isRGB {
  enum { val = false };
};

// Specialized struct for RGB input types.
template <unsigned CDEPTH>
struct isRGB<ac_ipl::RGB_1PPC<CDEPTH> > {
  enum { val = true };
};

// Input pixel initialization function for greyscale images: Convert RGB to grayscale input.
template <int IN_CDEPTH>
void initPixIn(
  const unsigned char* in_rarray, const unsigned char* in_garray, const unsigned char* in_barray,
  int i, int j,
  int widthInt, int heightInt,
  ac_int<IN_CDEPTH, false> &pixIn
)
{
  ac_fixed<IN_CDEPTH, IN_CDEPTH, false, AC_TRN, AC_SAT> pixInFi = 0.299*double(in_rarray[i*widthInt + j]) + 0.587*double(in_garray[i*widthInt + j]) +  0.114*double(in_barray[i*widthInt + j]);
  pixIn = pixInFi.to_int();
}

// Input pixel initialization function for RGB images: Copy RGB components in image arrays to input pixel's RGB components, while also initializing TUSER and TLAST flags.
template <unsigned IN_CDEPTH>
void initPixIn(const unsigned char* in_rarray, const unsigned char* in_garray, const unsigned char* in_barray, int i, int j, int widthInt, int heightInt, ac_ipl::RGB_1PPC<IN_CDEPTH> &pixIn)
{
  pixIn.R = int(in_rarray[i*widthInt + j]);
  pixIn.G = int(in_garray[i*widthInt + j]);
  pixIn.B = int(in_barray[i*widthInt + j]);
  pixIn.TUSER = (i == int(heightInt - 1) && j == 0);
  pixIn.TLAST = (j == int(widthInt - 1));
}

// Update reference histogram array for grayscale
template <int IN_CDEPTH>
void updateArrRef(ac_int<IN_CDEPTH, false> pixIn, int histArrRef[1 << IN_CDEPTH])
{
  histArrRef[pixIn]++;
}

// Update reference histogram array for RGB
template <unsigned IN_CDEPTH>
void updateArrRef(ac_ipl::RGB_1PPC<IN_CDEPTH> pixIn, int histArrRef[1 << IN_CDEPTH])
{
  histArrRef[pixIn.R]++;
  histArrRef[pixIn.G]++;
  histArrRef[pixIn.B]++;
}

template <class IN_TYPE>
bool test_driver(
  string outf_name
)
{
  enum {
    W_MAX = 1024,
    H_MAX = 1024,  
  };

  typedef imhistTypeInfo<W_MAX, H_MAX, IN_TYPE> stDef;

  // Having a static_assert here in addition to the one in ac_imhist.h ensures that it's the first to show up in the list of compiler errors.
  static_assert(stDef::match, "Input must either be unsigned ac_int or RGB_1PPC datatype.");

  enum {
    IN_CDEPTH = stDef::IN_CDEPTH,
    // The maximum count that can be stored in a histogram bin varies depending on whether the input is RGB or not.
    // If the input is a greyscale input, the most you will have to store in a bin is the max. possible number of pixels
    // in the image (W_MAX*H_MAX)
    // For an RGB input, this max number is tripled due to the fact that the histogram counts are accumulated across three color components.
    // The output bitwidth must be large enough to account for this max. possible output value.
    OUT_SIZE  = 1 << IN_CDEPTH,
    OUT_MAX_VAL = isRGB<IN_TYPE>::val ? 3*W_MAX*H_MAX : W_MAX*H_MAX,
    OUT_BW = ac::nbits<OUT_MAX_VAL>::val,
  };

  typedef ac_int<OUT_BW, false> OUT_TYPE;
  typedef ac_int<ac::nbits<W_MAX>::val, false> widthInType;
  typedef ac_int<ac::nbits<H_MAX>::val, false> heightInType;

  cout << "TEST: ac_imhist INPUT: ";
  cout.width(16);
  cout << left << IN_TYPE::type_name();
  cout << "OUTPUT: ";
  cout.width(17);
  cout << left << OUT_TYPE::type_name();
  cout << "RESULT: ";

  unsigned long width;
  long height;
  // Declare input pixel arrays, one for each color component.
  unsigned char *in_rarray = new unsigned char[W_MAX*H_MAX];
  unsigned char *in_garray = new unsigned char[W_MAX*H_MAX];
  unsigned char *in_barray = new unsigned char[W_MAX*H_MAX];

  string inf_name = "in_image.bmp";

  bool read_fail = bmp_read((char *)inf_name.c_str(), &width, &height, &in_rarray, &in_garray, &in_barray);
  if (read_fail) {
    return false;
  }

  // Declare input/output channels.
  ac_channel<IN_TYPE>  streamIn;
  ac_channel<OUT_TYPE> streamOut;

  int histArrRef[OUT_SIZE];

  // Initialize reference array for histogram output
  for (int i = 0; i < OUT_SIZE; i++) {
    histArrRef[i] = 0;
  }

  // Read input image in reverse row order; bmp files store the images in an inverted format.
  for (int i = int(height) - 1; i >= 0; i--) {
    for (int j = 0; j < int(width); j++) {
      IN_TYPE pixIn;
      initPixIn(in_rarray, in_garray, in_barray, i, j, int(width), int(height), pixIn);
      streamIn.write(pixIn);
      // Update the C++ reference array. The output stored in this array is compared to the design output.
      updateArrRef(pixIn, histArrRef);
    }
  }

  widthInType  widthIn  = width;
  heightInType heightIn = height;
  // Instantiate object of top level class.
  ac_imhist<IN_TYPE, OUT_BW, W_MAX, H_MAX> imhistObj;
  // Call top-level function
  imhistObj.run(streamIn, streamOut, widthIn, heightIn);

  if (streamIn.debug_size() != 0) {
    cout << "FAILED. Image input not completely consumed by design." << endl;
    return false;
  }

  if (streamOut.debug_size() != OUT_SIZE) {
    cout << "FAILED. Incorrect output size." << endl;
    return false;
  }

  bool matchesRef = true;

  // Read output channel.
  for (int i = 0; i < OUT_SIZE; i++) {
    OUT_TYPE histOut = streamOut.read();
    matchesRef = matchesRef && histOut == histArrRef[i];
    if (!matchesRef) { break; }
  }

  if (!matchesRef) {
    cout << "FAILED. Design output does not match reference output." << endl;
    return false;
  }

  cout << "PASSED." << endl;

  delete[] in_rarray;
  delete[] in_garray;
  delete[] in_barray;
  
  return true;
}

int main(int argc, char *argv[])
{
  cout << "==================================================================================" << endl;
  cout << "------------------------- Running rtest_ac_imhist.cpp ----------------------------" << endl;
  cout << "==================================================================================" << endl;

  bool all_tests_pass = true;

  all_tests_pass = test_driver<ac_int<8, false>    >("out_imhist_ac_int.txt")   && all_tests_pass;
  all_tests_pass = test_driver<ac_ipl::RGB_1PPC<8> >("out_imhist_RGB_1PPC.txt") && all_tests_pass;

  cout << "  Testbench finished." << endl;

  // Notify the user whether or not the test was a failure.
  if (!all_tests_pass) {
    cout << "  ac_imhist - FAILED" << endl;
    return -1;
  }

  cout << "  ac_imhist - PASSED" << endl;

  return 0;
}
