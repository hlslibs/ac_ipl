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
// To compile and execute stand-alone:
// $MGC_HOME/bin/c++ -std=c++11 -I$MGC_HOME/shared/include rtest_ac_ctc.cpp -o design
// ./design

#include <ac_ipl/ac_ctc.h>

#include <string>
#include <fstream>
#include <iostream>
using namespace std;
#include <bmpUtil/bmp_io.cpp>

int main(int argc, char *argv[])
{
  // Template parameters for ac_ctc design.
  enum {
    CDEPTH = 8,
    W_MAX = 1024,
    H_MAX = 1024,
    TEMP_MAX = 13700,
  };

  typedef ac_ipl::RGB_1PPC<CDEPTH> IO_TYPE;
  
  // Define types for direct inputs.
  typedef ac_int<ac::nbits<W_MAX>::val, false> widthInType;
  typedef ac_int<ac::nbits<H_MAX>::val, false> heightInType;
  typedef ac_int<ac::nbits<TEMP_MAX>::val, false> tempInType;

  // Declare input/output pixel arrays, one for each color component.
  unsigned char *rArray = new unsigned char[W_MAX*H_MAX];
  unsigned char *gArray = new unsigned char[W_MAX*H_MAX];
  unsigned char *bArray = new unsigned char[W_MAX*H_MAX];

  // Define number of images and target color temperature.
  enum {
    n_images = 8,
    tempInEnum = 4000,
  };

  // Define array of input file names.
  string inf_names[n_images] = {
    "in_image_0.bmp",
    "in_image_1.bmp",
    "in_image_2.bmp",
    "in_image_3.bmp",
    "in_image_4.bmp",
    "in_image_5.bmp",
    "in_image_6.bmp",
    "in_image_7.bmp"
  };

  for (int k = 0; k < n_images; k++) {
    #ifdef DEBUG
    cout << "Processing frame number: " << k << endl;
    #endif
    
    // Initialize input/output filenames.
    string inFileName  = inf_names[k];
    string outFileName = "ctc_" + inFileName;
    
    unsigned long width;
    long height;
    
    #ifdef DEBUG
    cout << "Reading input image from file: " << inFileName << endl;
    #endif

    bool read_fail = bmp_read((char *)inFileName.c_str(), &width, &height, &rArray, &gArray, &bArray);
    
    if (!read_fail) {
      #ifdef DEBUG
      cout << "Input image read successfully. Width = " << width << ", height = " << height << endl;
      #endif
    } else {
      return -1;
    } // Return -1 and exit main() if reading from input file fails.
    
    // Declare input/output channels.
    ac_channel<IO_TYPE> streamIn, streamOut;
    
    // Read input image in reverse row order; bmp files store the images in an inverted format.
    for (int i = int(height) - 1; i >= 0; i--) {
      for (int j = 0; j < int(width); j++) {
        IO_TYPE pixIn;
        int img_idx = i*int(width) + j;
        pixIn.R = int(rArray[img_idx]);
        pixIn.G = int(gArray[img_idx]);
        pixIn.B = int(bArray[img_idx]);
        pixIn.TUSER = (i == int(height) - 1 && j == 0);
        pixIn.TLAST = (j == int(width) - 1);
        streamIn.write(pixIn); // Write pixels to input channel.
      }
    }
    
    widthInType  widthIn  = width;
    heightInType heightIn = height;
    tempInType tempIn = tempInEnum;

    #ifdef DEBUG
    cout << "Design execution about to start. Input size = " << streamIn.debug_size() << endl;   
    #endif

    // Instantiate object of top level design and call run() function.
    ac_ctc<CDEPTH, W_MAX, H_MAX, TEMP_MAX> ctcObj;
    ctcObj.run(streamIn, streamOut, widthIn, heightIn, tempIn);
    
    #ifdef DEBUG
    cout << "Design execution complete. Output size = " << streamOut.debug_size() << endl;
    #endif
   
    if (streamIn.debug_size() != 0) {
      cout << "Test FAILED. Image input not completely consumed by design." << endl;
      return -1;
    }

    // Make sure output and input sizes are the same.
    if (streamOut.debug_size() != unsigned(width)*unsigned(height)) {
      cout << "Test FAILED. Image output size not same as input size." << endl;
      return -1;
    }
    
    // Read output channel, store output in image arrays.
    for (int i = int(height) - 1; i >= 0; i--) {
      for (int j = 0; j < int(width); j++) {
        int img_idx = i*int(width) + j;
        IO_TYPE pixOut = streamOut.read();
        rArray[img_idx] = pixOut.R.to_int();
        gArray[img_idx] = pixOut.G.to_int();
        bArray[img_idx] = pixOut.B.to_int();
      }
    }
    
    #ifdef DEBUG
    cout << "Writing image to file: " << outFileName << endl;
    #endif
    bool write_fail = bmp_24_write((char *)outFileName.c_str(), width,  height, rArray, gArray, bArray);

    if (!write_fail) {
      #ifdef DEBUG
      cout << "Image written successfully." << endl;
      #endif
    } else {
      return -1;
    }
  }
  
  delete[] rArray;
  delete[] gArray;
  delete[] bArray;

  return 0;
}
