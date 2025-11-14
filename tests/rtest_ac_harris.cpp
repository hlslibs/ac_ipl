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
#include <ac_ipl/ac_harris.h>
#include <string>
#include <fstream>
#include <iostream>
using namespace std;
#include <bmpUtil/bmp_io.hpp>
#include <bmpUtil/bmp_io.cpp>

// This file tests for grayscale images. For testing color images, the i/o datatypes have to be changed to RGB_imd. For that, refer documentation 

//   To compile standalone and run:
//   $MGC_HOME/bin/c++ -std=c++11 -I$MGC_HOME/shared/include $MGC_HOME/shared/include/bmpUtil/bmp_io.cpp rtest_ac_harris.cpp -o design
//   ./design 

// Input pixel initialization function for greyscale images.
template <int CDEPTH>
void initPixIn(
  const unsigned char *io_rarray, const unsigned char *io_garray, const unsigned char *io_barray,
  int i, int j,
  int widthInt, int heightInt,
  ac_int<CDEPTH, false> &pixIn
)
{
  ac_fixed<64, 0, false> coeff1 = 0.299, coeff2 = 0.587, coeff3 = 0.114;
  typedef ac_fixed<CDEPTH, CDEPTH, false> T_fi;
  pixIn = (coeff1*T_fi(io_rarray[i*widthInt + j]) + coeff2*T_fi(io_garray[i*widthInt + j]) + coeff3*T_fi(io_barray[i*widthInt + j])).to_int();
}

// Input pixel initialization function for RGB images: Copy RGB components in image arrays to input pixel's RGB components
template <int CDEPTH>
void initPixIn(const unsigned char *io_rarray, const unsigned char *io_garray, const unsigned char *io_barray, int i, int j, int widthInt, int heightInt, ac_ipl::RGB_imd<ac_int<CDEPTH, false> > &pixIn)
{
  pixIn.R = int(io_rarray[i*widthInt + j]);
  pixIn.G = int(io_garray[i*widthInt + j]);
  pixIn.B = int(io_barray[i*widthInt + j]);
}

// assignIn_to_Out copies input pixel to the ouput pixel when corners are not present.
template <int CDEPTH>
void assignIn_to_Out(ac_int<CDEPTH, false> &pixOut, ac_int<CDEPTH, false> x)
{
  if (pixOut == 0) {
    pixOut = x;
  }
}

template <int CDEPTH>
void assignIn_to_Out(ac_ipl::RGB_imd<ac_int<CDEPTH, false> > &pixOut, ac_ipl::RGB_imd<ac_int<CDEPTH, false> > x)
{
  if ((pixOut.R == 0) && (pixOut.G == 0) && (pixOut.R == 0)) {
    pixOut.R = x.R;
    pixOut.G = x.G;
    pixOut.B = x.B;
  }
}

// copyToOutArr copies greyscale output pixel to all three output image arrays.
template <unsigned CDEPTH>
void copyToOutArr(ac_int<CDEPTH, false> pixOut, int i, int j, int widthInt, int heightInt, unsigned char *io_rarray, unsigned char *io_garray, unsigned char *io_barray)
{
  io_rarray[i*widthInt + j] = (pixOut.to_int());
  io_garray[i*widthInt + j] = (pixOut.to_int());
  io_barray[i*widthInt + j] = (pixOut.to_int());
}

// copyToOutArr copies RGB components of color output to output image arrays.
template <unsigned CDEPTH>
void copyToOutArr(ac_ipl::RGB_imd<ac_int<CDEPTH, false> > pixOut, int i, int j, int widthInt, int heightInt, unsigned char *io_rarray, unsigned char *io_garray, unsigned char *io_barray)
{
  io_rarray[i*widthInt + j] = (pixOut.R.to_int());
  io_garray[i*widthInt + j] = (pixOut.G.to_int());
  io_barray[i*widthInt + j] = (pixOut.B.to_int());
}

int main(int argc, char *argv[])
{
  enum {
    CDEPTH = 8,              // Color depth/pixel bitwidth
    W_MAX  = 1024,           // Maximum width = 1024
    H_MAX  = 1024,           // Maximum height = 1024
  };

  // Define IO and dimension types
  typedef ac_int<CDEPTH, false> pixInType;
  typedef ac_int<CDEPTH, false> pixOutType;


  typedef ac_int<ac::nbits<W_MAX>::val, false> widthInType;
  typedef ac_int<ac::nbits<H_MAX>::val, false> heightInType;

  // Initialize input/output filenames.
  string inf_name  = "House_image.bmp";
  string outf_name = "out_image_harris.bmp";

  unsigned long width;
  long height;
  // Declare input/output pixel arrays, one for each color component.
  unsigned char *io_rarray = new unsigned char[W_MAX*H_MAX];
  unsigned char *io_garray = new unsigned char[W_MAX*H_MAX];
  unsigned char *io_barray = new unsigned char[W_MAX*H_MAX];


  cout << "Reading image from file: " << inf_name << endl;
  bool read_fail = bmp_read((char *)inf_name.c_str(), &width, &height, &io_rarray, &io_garray, &io_barray);
  if (!read_fail) { cout << "Image read successfully. Width = " << width << ", height = " << height << endl; }
  else            { return -1; } // Return -1 and exit main() if reading from input file fails.

  // Declare input/output channels.
  ac_channel<pixInType> streamIn;
  ac_channel<pixInType> streamCopyIn;
  ac_channel<pixOutType> streamOut;

  // Read in reverse row order; bmp files store the images in an inverted format.
  for (int i = height - 1; i >= 0; i--) {
    for (int j = 0; j < width; j++) {
      pixInType pixIn;
      initPixIn(io_rarray, io_garray, io_barray, i, j, int(width), int(height), pixIn);
      streamIn.write(pixIn);
      streamCopyIn.write(pixIn);
    }
  }

  // ac_int dimension variables.
  widthInType  widthIn  = width;
  heightInType heightIn = height;

  // Specify the component of the RGB pixel for which harris corner detection is required. R = 0, G = 1, B = 2
  ac_int<2, false> component = 0; 
  // Epsilon can have values 0-6. Generally higher the value, more corner points are detected.
  ac_int<3, false> epsilon = 6;
  // Threshold (Ideal value = 1000). Higher the value, lesser the corner points and vice versa.
  ac_int<14, false> threshold = 1000;

  ac_harris<pixInType, pixOutType, CDEPTH, W_MAX, H_MAX, true> harrisObj; // Instantitate harris corner detect class.

  harrisObj.run(streamIn, streamOut, widthIn, heightIn, component, epsilon, threshold); // Call the top-level run() function.

  cout << "Design execution complete. Output size = " << streamOut.debug_size() << endl;

  if (streamIn.debug_size() != 0) {
    cout << "Test FAILED. Image input not completely consumed by design." << endl;
    return -1;
  }

  // Make sure output and input sizes are the same.
  if (streamOut.debug_size() != unsigned(width)*unsigned(height)) {
    cout << "Test FAILED. Image output size not same as input size." << endl;
    return -1;
  }

  // Read output channel, store output in io_array.
  for (int i = int(height) - 1; i >= 0; i--) {
    for (int j = 0; j < int(width); j++) {
      pixOutType pixOut, x;
      pixOut = streamOut.read();
      x      = streamCopyIn.read();
      assignIn_to_Out(pixOut,x);
      copyToOutArr<CDEPTH>(pixOut, i, j, int(width), int(height), io_rarray, io_garray, io_barray);
    }
  }

  cout << "Writing image to file: " << outf_name << endl;
  bool write_fail = bmp_24_write ((char *)outf_name.c_str(), width,  height, io_rarray, io_garray, io_barray);
  if (!write_fail) { cout << "Image written successfully." << endl; }
  else             { return -1; } // Return -1 and exit main() if writing to output file fails.

  delete[] io_rarray;
  delete[] io_garray;
  delete[] io_barray;

  return 0;
}
