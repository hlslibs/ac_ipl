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
#include <ac_ipl/ac_dither.h>

#include <string>
#include <fstream>
#include <iostream>
using namespace std;
#include <bmpUtil/bmp_io.cpp>

// Input pixel initialization function for greyscale images: Convert RGB to grayscale input.
template <int IN_CDEPTH>
void initPixIn(
  const unsigned char* io_rarray, const unsigned char* io_garray, const unsigned char* io_barray,
  int i, int j,
  int widthInt, int heightInt,
  ac_int<IN_CDEPTH, false> &pixIn
)
{
  // ac_fixed values are used here instead of double values; this is done to ensure consistency between 32- and 64-bit compilations.
  ac_fixed<64, 0, false> coeff1 = 0.299, coeff2 = 0.587, coeff3 = 0.114;
  typedef ac_fixed<IN_CDEPTH, IN_CDEPTH, false> T_fi;
  pixIn = (coeff1*T_fi(io_rarray[i*widthInt + j]) + coeff2*T_fi(io_garray[i*widthInt + j]) + coeff3*T_fi(io_barray[i*widthInt + j])).to_int();
}

// Input pixel initialization function for RGB images: Copy RGB components in image arrays to input pixel's RGB components, while also initializing TUSER and TLAST flags.
template <unsigned IN_CDEPTH>
void initPixIn(const unsigned char* io_rarray, const unsigned char* io_garray, const unsigned char* io_barray, int i, int j, int widthInt, int heightInt, ac_ipl::RGB_1PPC<IN_CDEPTH> &pixIn)
{
  pixIn.R = int(io_rarray[i*widthInt + j]);
  pixIn.G = int(io_garray[i*widthInt + j]);
  pixIn.B = int(io_barray[i*widthInt + j]);
  pixIn.TUSER = (i == int(heightInt - 1) && j == 0);
  pixIn.TLAST = (j == int(widthInt - 1));
}

// copyToOutArr copies greyscale output pixel to all three output image arrays, and also checks for output image flags.
template <int IN_CDEPTH, int OUT_CDEPTH>
bool copyToOutArr(ac_int<OUT_CDEPTH, false> pixOut, int i, int j, int widthInt, int heightInt, unsigned char* io_rarray, unsigned char* io_garray, unsigned char* io_barray)
{
  const int inMaxVal  = (1 << IN_CDEPTH)  - 1;
  const int outMaxVal = (1 << OUT_CDEPTH) - 1;
  const double quantRatio = double(inMaxVal)/double(outMaxVal);

  io_rarray[i*widthInt + j] = quantRatio*(pixOut.to_int());
  io_garray[i*widthInt + j] = quantRatio*(pixOut.to_int());
  io_barray[i*widthInt + j] = quantRatio*(pixOut.to_int());

  return false;
}

// copyToOutArr copies RGB components of color output to output image arrays, and also checks for output image flags.
template <unsigned IN_CDEPTH, unsigned OUT_CDEPTH>
bool copyToOutArr(ac_ipl::RGB_1PPC<OUT_CDEPTH> pixOut, int i, int j, int widthInt, int heightInt, unsigned char* io_rarray, unsigned char* io_garray, unsigned char* io_barray)
{
  const int inMaxVal  = (1 << IN_CDEPTH)  - 1;
  const int outMaxVal = (1 << OUT_CDEPTH) - 1;
  const double quantRatio = double(inMaxVal)/double(outMaxVal);
  bool flag_unexp;

  io_rarray[i*widthInt + j] = quantRatio*(pixOut.R.to_int());
  io_garray[i*widthInt + j] = quantRatio*(pixOut.G.to_int());
  io_barray[i*widthInt + j] = quantRatio*(pixOut.B.to_int());
  if (((j == widthInt - 1) != pixOut.TLAST) || ((i == heightInt - 1 && j == 0) != pixOut.TUSER)) {
    flag_unexp = true;
  }
  return flag_unexp;
}

template <class IN_TYPE, class OUT_TYPE, bool use_sp>
bool test_driver(
  string outf_name,
  string ref_name = ""
)
{
  typedef T_info<IN_TYPE, OUT_TYPE, 64, 32, AC_TRN, AC_WRAP> stDef;

  enum {
    W_MAX = 1024,
    H_MAX = 1024,
    IN_CDEPTH = stDef::IN_CDEPTH,
  };

  // Having a static_assert here in addition to the one in ac_dither.h ensures that it's the first to show up in the list of compiler errors.
  static_assert(stDef::match, "Input and output must either be unsigned ac_int or RGB_1PPC datatypes.");

  // Define dimension types
  typedef ac_int<ac::nbits<W_MAX>::val, false> widthInType;
  typedef ac_int<ac::nbits<H_MAX>::val, false> heightInType;

  cout << "TEST: ac_dither INPUT: ";
  cout.width(16);
  cout << left << IN_TYPE::type_name();
  cout << "OUTPUT: ";
  cout.width(16);
  cout << left << OUT_TYPE::type_name();
  cout << "use_sp: ";
  cout.width(6);
  cout << left << (use_sp ? "true" : "false");
  cout << "RESULT: ";

  unsigned long width;
  long height;
  // Declare input/output pixel arrays, one for each color component.
  unsigned char *io_rarray = new unsigned char[W_MAX*H_MAX];
  unsigned char *io_garray = new unsigned char[W_MAX*H_MAX];
  unsigned char *io_barray = new unsigned char[W_MAX*H_MAX];
  // Same as above, but for reference image output.
  unsigned char *ref_rarray = new unsigned char[W_MAX*H_MAX];
  unsigned char *ref_garray = new unsigned char[W_MAX*H_MAX];
  unsigned char *ref_barray = new unsigned char[W_MAX*H_MAX];

  // Initialize input filename.
  string inf_name = "in_image.bmp";

  bool read_fail = bmp_read((char *)inf_name.c_str(), &width, &height, &io_rarray, &io_garray, &io_barray);
  if (read_fail) {
    return false;
  }

  bool use_ref_img = (ref_name != "");

  if (use_ref_img) {
    read_fail = bmp_read((char *)ref_name.c_str(), &width, &height, &ref_rarray, &ref_garray, &ref_barray);
    if (read_fail) {
      return false;
    }
  }

  // Declare input/output channels.
  ac_channel<IN_TYPE>  streamIn;
  ac_channel<OUT_TYPE> streamOut;

  // Read input image in reverse row order; bmp files store the images in an inverted format.
  for (int i = int(height) - 1; i >= 0; i--) {
    for (int j = 0; j < int(width); j++) {
      IN_TYPE pixIn;
      initPixIn(io_rarray, io_garray, io_barray, i, j, int(width), int(height), pixIn);
      streamIn.write(pixIn);
    }
  }

  widthInType  widthIn  = width;
  heightInType heightIn = height;

  // Instantiate object of top level class.
  ac_dither<IN_TYPE, OUT_TYPE, W_MAX, H_MAX, use_sp, 64, 32, AC_TRN, AC_WRAP> DitherObj;
  // Call top-level function
  DitherObj.run(streamIn, streamOut, widthIn, heightIn);

  if (streamIn.debug_size() != 0) {
    cout << "FAILED. Image input not completely consumed by design." << endl;
    return false;
  }

  // Make sure output and input sizes are the same.
  if (streamOut.debug_size() != unsigned(width)*unsigned(height)) {
    cout << "FAILED. Image output size not same as input size." << endl;
    return false;
  }

  // flag_unexp: set to true if an unexpected output flag (TUSER/TLAST) is seen.
  bool flag_unexp = false;
  // matches_ref: set to false if the design output does not match reference output.
  bool matches_ref = true;

  // Read output channel, store output in io_array.
  for (int i = int(height) - 1; i >= 0; i--) {
    for (int j = 0; j < int(width); j++) {
      int img_idx = i*int(width) + j;
      OUT_TYPE pixOut = streamOut.read();
      flag_unexp = flag_unexp || copyToOutArr<IN_CDEPTH>(pixOut, i, j, int(width), int(height), io_rarray, io_garray, io_barray);
      if (use_ref_img) {
        // If a reference image is used, the design output is compared against the reference output.
        matches_ref = matches_ref && ref_rarray[img_idx] == io_rarray[img_idx] && ref_garray[img_idx] == io_garray[img_idx] && ref_barray[img_idx] == io_barray[img_idx];
      }
      if (flag_unexp || !matches_ref) { break; }
    }
    if (flag_unexp || !matches_ref) { break; }
  }

  if (flag_unexp) {
    cout << "FAILED. Unexpected output flag value(s)." << endl;
    return false;
  }

  if (!matches_ref) {
    cout << "FAILED. Design output does not match reference." << endl;
    return false;
  }

  bool write_fail = bmp_24_write((char *)outf_name.c_str(), width, height, io_rarray, io_garray, io_barray);
  if (write_fail) {
    return false;
  }

  delete[] io_rarray;
  delete[] io_garray;
  delete[] io_barray;
  delete[] ref_rarray;
  delete[] ref_garray;
  delete[] ref_barray;

  cout << "PASSED." << endl;
  return true;
}

int main(int argc, char *argv[])
{
  cout << "==================================================================================" << endl;
  cout << "------------------------- Running rtest_ac_dither.cpp ----------------------------" << endl;
  cout << "==================================================================================" << endl;

  bool all_tests_pass = true;

  all_tests_pass = test_driver<ac_int<8, false>,    ac_int<1, false>,    true >("out_image_dither_GS_3_bpp_sp.bmp",  "out_image_dither_ref_ac_int.bmp")   && all_tests_pass;
  all_tests_pass = test_driver<ac_int<8, false>,    ac_int<1, false>,    false>("out_image_dither_GS_3_bpp_dp.bmp",  "out_image_dither_ref_ac_int.bmp")   && all_tests_pass;
  all_tests_pass = test_driver<ac_ipl::RGB_1PPC<8>, ac_ipl::RGB_1PPC<1>, true >("out_image_dither_RGB_3_bpp_sp.bmp", "out_image_dither_ref_RGB_1PPC.bmp") && all_tests_pass;
  all_tests_pass = test_driver<ac_ipl::RGB_1PPC<8>, ac_ipl::RGB_1PPC<1>, false>("out_image_dither_RGB_3_bpp_dp.bmp", "out_image_dither_ref_RGB_1PPC.bmp") && all_tests_pass;

  all_tests_pass = test_driver<ac_int<8, false>,    ac_int<3, false>,    true >("out_image_dither_GS_9_bpp_sp.bmp")  && all_tests_pass;
  all_tests_pass = test_driver<ac_int<8, false>,    ac_int<3, false>,    false>("out_image_dither_GS_9_bpp_dp.bmp")  && all_tests_pass;
  all_tests_pass = test_driver<ac_ipl::RGB_1PPC<8>, ac_ipl::RGB_1PPC<3>, true >("out_image_dither_RGB_9_bpp_sp.bmp") && all_tests_pass;
  all_tests_pass = test_driver<ac_ipl::RGB_1PPC<8>, ac_ipl::RGB_1PPC<3>, false>("out_image_dither_RGB_9_bpp_dp.bmp") && all_tests_pass;

  cout << "  Testbench finished." << endl;

  // Notify the user whether or not the test was a failure.
  if (!all_tests_pass) {
    cout << "  ac_dither - FAILED" << endl;
    return -1;
  }

  cout << "  ac_dither - PASSED" << endl;
  return 0;
}

