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
#include <ac_ipl/ac_gamma.h>

#include <stdio.h>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <iostream>

using namespace std;

#include <bmpUtil/bmp_io.hpp>
#include <bmpUtil/bmp_io.cpp>


/*#############################################
Input pixel initialization function for greyscale images: Convert RGB to grayscale input.
#############################################*/
template <int IN_CDEPTH>
void initPixIn(const unsigned char *io_rarray, const unsigned char *io_garray, const unsigned char *io_barray, int i, int j, int widthInt, int heightInt, ac_int<IN_CDEPTH, false> &pixIn)
{
  // ac_fixed values are used here instead of double values; this is done to ensure consistency between 32- and 64-bit compilations.
  ac_fixed<64, 0, false> coeff1 = 0.299, coeff2 = 0.587, coeff3 = 0.114;
  typedef ac_fixed<IN_CDEPTH, IN_CDEPTH, false> T_fi;
  pixIn = (coeff1*T_fi(io_rarray[i*widthInt + j]) + coeff2*T_fi(io_garray[i*widthInt + j]) + coeff3*T_fi(io_barray[i*widthInt + j])).to_int();
}

/*#############################################
 Input pixel initialization function for RGB images: Copy RGB components in image arrays to input pixel's RGB components
#############################################*/

template <unsigned IN_CDEPTH>
void initPixIn(const unsigned char *io_rarray, const unsigned char *io_garray, const unsigned char *io_barray, int i, int j, int widthInt, int heightInt, ac_ipl::RGB_imd<ac_int<IN_CDEPTH,false> > &pixIn)
{
  pixIn.R = io_rarray[i*widthInt + j];
  pixIn.G = io_garray[i*widthInt + j];
  pixIn.B = io_barray[i*widthInt + j];
}

/*#############################################
// copyToOutArr copies greyscale output pixel to all three output image arrays, and also checks for output image flags.
#############################################*/

template <int IN_CDEPTH>
void copyToOutArr(ac_int<IN_CDEPTH, false> pixOut, int i, int j, int widthInt, int heightInt, unsigned char *io_rarray, unsigned char *io_garray, unsigned char *io_barray)
{
  io_rarray[i*widthInt + j] = pixOut;
  io_garray[i*widthInt + j] = pixOut;
  io_barray[i*widthInt + j] = pixOut;
}

/*#############################################
// copyToOutArr copies RGB components of color output to output image arrays, and also checks for output image flags.
#############################################*/

template <int IN_CDEPTH>
void copyToOutArr(ac_ipl::RGB_imd<ac_int<IN_CDEPTH,false> > pixOut, int i, int j, int widthInt, int heightInt, unsigned char *io_rarray, unsigned char *io_garray, unsigned char *io_barray)
{
  io_rarray[i*widthInt + j] = pixOut.R;
  io_garray[i*widthInt + j] = pixOut.G;
  io_barray[i*widthInt + j] = pixOut.B;
}

int main(int argc, char *argv[])
{
// Temaplate parameteres declaration.
  enum {
    Color_depth =    8, // Color depth/pixel bitwidth = 8
    W_MAX  = 1024, // Maximum width = 1024
    H_MAX  = 1024, // Maximum height = 1024
    gamma_width = 18, // Bit width of the Gamma variblae
    gamma_integer_bits = 2 //
  };
  
  // Initialize input/output filenames.
  typedef ac_ipl::RGB_imd<ac_int<Color_depth,false> > PixType;
  string outf_name = "gamma_rgb_output.bmp";
  string inf_name  = "gamma_input.bmp";
  unsigned long width;
  long height;

  // Declare input/output pixel arrays, one for each color component.
  unsigned char *io_rarray = new unsigned char[W_MAX*H_MAX];
  unsigned char *io_garray = new unsigned char[W_MAX*H_MAX];
  unsigned char *io_barray = new unsigned char[W_MAX*H_MAX];

  ac_gamma<PixType, Color_depth, gamma_width, gamma_integer_bits> gammaObj;  // Instantitate gamma correction class.


  cout << "=================================================================================" << endl;
  cout << "------------------------- Running rtest_ac_gamma.cpp ----------------------------" << endl;
  cout << "=================================================================================" << endl;


// Read the input BMP file.
  cout << "Reading image from file: " << inf_name << endl;
  bool read_fail = bmp_read((char *)inf_name.c_str(), &width, &height, &io_rarray, &io_garray, &io_barray);
 
  cout << "Width: " << width << " Height :"<< height << endl;

  // Declare input/output channels.
  ac_channel<PixType> streamIn; // input ac_channel of type RGB, with user defined CDEPTH.
  ac_channel<PixType> streamOut; // output ac_channel of type RGB. with user defined CDEPTH.

  // Define a test gamma value of type ac_Fixed with widths defined above in enum.
  double gamma_ref=0.22;
  ac_fixed<gamma_width, gamma_integer_bits, false, AC_TRN> gamma=gamma_ref;

  PixType pixOut;
  PixType pixIn;

  // Read in reverse row order; bmp files store the images in an inverted format.
  // Read the image from the i/o array into the ac_channel of type RGB struct.
  for (int i = height - 1; i >= 0; i--) {
    for (int j = 0; j < width; j++) {
      initPixIn<Color_depth>(io_rarray, io_garray, io_barray, i, j, int(width), int(height), pixIn);
      streamIn.write(pixIn);
    }
  }

  gammaObj.run(streamIn, streamOut, gamma); // Call the top-level run() function.


  // Read output channel, store output in io_array.
  for (int i = height - 1; i >= 0; i--) {
    for (int j = 0; j < width; j++) {
      pixOut = streamOut.read();
      copyToOutArr<Color_depth>(pixOut, i, j, int(width), int(height), io_rarray, io_garray, io_barray);
    }
  }

  cout << "Writing image output to file: " << outf_name << endl;
  bool write_fail = bmp_24_write ((char *)outf_name.c_str(), width,  height, io_rarray, io_garray, io_barray);
 
  // clear the io arrays declared.
  delete[] io_rarray;
  delete[] io_garray;
  delete[] io_barray;

  return 0;
}


