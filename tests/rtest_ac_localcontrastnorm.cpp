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
#include <ac_ipl/ac_localcontrastnorm.h>

#include <string>
#include <fstream>
#include <iostream>
using namespace std;
#include <bmpUtil/bmp_io.hpp>
#include <bmpUtil/bmp_io.cpp>

//   To compile standalone and run:
//   $MGC_HOME/bin/c++ -std=c++11 -I$MGC_HOME/shared/include rtest_ac_localcontrastnorm.cpp -o design
//   ./design <localcontrastnorm_input.bmp> <localcontrastnorm_output.bmp>

int main(int argc, char *argv[])
{
  enum {
    CDEPTH = 8,              // Color depth/pixel bitwidth
    H_MAX  = 1024,           // Maximum height
    W_MAX  = 1024            // Maximum width
  };

  // Define IO and dimension types
  typedef ac_ipl::RGB_1PPC<CDEPTH> pixInType;
  typedef ac_ipl::RGB_1PPC<CDEPTH> pixOutType;
	typedef ac_int<ac::nbits<W_MAX>::val, false> widthInType;
  typedef ac_int<ac::nbits<H_MAX>::val, false> heightInType;

  cout << "=================================================================================" << endl;
  cout << "------------------------- Running rtest_ac_localcontrastnorm.cpp ----------------------------" << endl;
  cout << "=================================================================================" << endl;
  
  // Initialize input/output filenames.
  string inf_name  = "in_image.bmp";
  string outf_name = "out_image.bmp";

  unsigned long width;
  long height;
  // Declare input/output pixel arrays, one for each color component.
  unsigned char *io_rarray = new unsigned char[W_MAX*H_MAX];
  unsigned char *io_garray = new unsigned char[W_MAX*H_MAX];
  unsigned char *io_barray = new unsigned char[W_MAX*H_MAX];

  cout << "Reading image from file: " << inf_name << endl;
  bmp_read((char *)inf_name.c_str(), &width, &height, &io_rarray, &io_garray, &io_barray);
  cout << "Image read successfully. Width = " << width << ", height = " << height << endl;

  // Declare input/output channels.
  ac_channel<pixInType>  streamIn;
  ac_channel<pixOutType> streamOut;

  // Read in reverse row order; bmp files store the images in an inverted format.
  for (int i = height - 1; i >= 0; i--) {
    for (int j = 0; j < width; j++) {
      pixInType pixIn;
      pixIn.R = int(io_rarray[i*width + j]);
      pixIn.G = int(io_garray[i*width + j]);
      pixIn.B = int(io_barray[i*width + j]);
      pixIn.TUSER = (i == height - 1 && j == 0);
      pixIn.TLAST = (j == width - 1);
      streamIn.write(pixIn);
    }
  }

	// image dimensions
  widthInType  widthIn  = width;
  heightInType heightIn = height;
  // Instantiate object of top level class.
	ac_localcontrastnorm<CDEPTH, W_MAX, H_MAX> LocalContrastNormObj;
  // Call top-level function
  LocalContrastNormObj.run(streamIn, streamOut, widthIn, heightIn);

  cout << "Design execution complete. Output size = " << streamOut.debug_size() << endl;

  // Read output channel, store output in io_array.
  for (int i = height - 1; i >= 0; i--) {
    for (int j = 0; j < width; j++) {
      pixOutType pixOut = streamOut.read();
      io_rarray[i*width + j] = (pixOut.R.to_int());
      io_garray[i*width + j] = (pixOut.G.to_int());
      io_barray[i*width + j] = (pixOut.B.to_int());
    }
  }

  cout << "Writing image to file: " << outf_name << endl;
  bmp_24_write ((char *)outf_name.c_str(), width,  height, io_rarray, io_garray, io_barray);
  cout << "Image written successfully." << endl;
  
  delete[] io_rarray;
  delete[] io_garray;
  delete[] io_barray;

  return 0;
}
