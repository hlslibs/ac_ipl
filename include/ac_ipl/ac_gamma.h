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
//*********************************************************************************************************
// File: ac_gamma.h
//
// Description:
//  Deign of a hardware implementation of a Gamma correction algorithm.
//  For a given input gamma_in value image value correction (encoding or decoding) is done for all the three pixel components of a color image.
//  Gamma value can be between 0 and 2.4, based on the defined standard for the gamma_in encoding/decoding.
//  If gamma_in is 1 (the default), the mapping is linear.
//  If gamma_in is less than 1, the mapping is weighted toward higher (brighter) output values which can also be called gamma_in encoding or gamma_in compression.
//  Conversely If gamma_in is greater than 1, the mapping is weighted toward lower (darker) output values or is called gamma_in decoding or gamma_in expansion.
//*********************************************************************************************************

#ifndef _INCLUDED_GAMMA_H_
#define _INCLUDED_GAMMA_H_

#include <ac_int.h>
#include <ac_channel.h>
#include <stdio.h>

#include <ac_math/ac_div.h>
#include <ac_math/ac_shift.h>
#include <ac_math/ac_pow_pwl.h>
#include <ac_math/ac_reciprocal_pwl.h>
#include <ac_ipl/ac_pixels.h>
#include <mc_scverify.h>

using namespace std;

#pragma hls_design
template <class PIX_TYP, unsigned CDEPTH, unsigned gamma_in_width=18, unsigned gamma_in_integer_bits=2>
class ac_gamma
{

public:

/*#############################################
Define the IO Types, and the gamma value type
#############################################*/

  typedef PIX_TYP pix_rgb;
  typedef ac_fixed<gamma_in_width, gamma_in_integer_bits, false, AC_TRN> gamma_in_type;

  ac_gamma() {};
  #pragma hls_pipeline_init_interval 1
  #pragma hls_design interface
  void CCS_BLOCK(run)(
    ac_channel<PIX_TYP> &ImageIn, // PixelIn
    ac_channel<PIX_TYP> &ImageOut, // PixelOut
    gamma_in_type &gamma_in // Gamma value applied
  ) {
    #ifndef __SYNTHESIS__
    while (ImageIn.available(1))
    #endif
    {
      ImgIn=ImageIn.read(); // Pixel Read
      gamma_correction<255,CDEPTH>(ImgIn, ImgOut, gamma_in); // Gamma function call.
      ImageOut.write(ImgOut); // Pixel Written oput
    }
  }

private:
  PIX_TYP ImgIn, ImgOut;


/*#############################################
Gamma correction function for Grayscale image, speciallized based on the type of the PixIn defined and passed using template params
#############################################*/

  template<int RefValue, int CD>
  void gamma_correction(ac_int<CD,false> pixIn, ac_int<CD,false> &pixOut, gamma_in_type gamma_in) {
    ac_fixed<CD, CD, false, AC_RND> Img, Shi, Sub;
    ac_fixed<gamma_in_width, gamma_in_integer_bits, false, AC_TRN> Div, Pow;
	#pragma hls_waive FXD
    Img=pixIn;
	#pragma hls_waive FXD
    ac_fixed<CD, CD, false> RefPixel_value=RefValue; 
    ac_math::ac_div(Img, RefPixel_value, Div); // normalize by 255
    ac_math::ac_pow_pwl(Div, gamma_in, Pow); // pow to the gamma value
    ac_math::ac_shift_left(Pow, CD, Shi); //Mul by 256
    Sub = Shi-Pow; // adjust to accuracy.
    pixOut= Sub.to_int();

    #ifndef DEBUG
    cout << "==========================Debug========================"<< endl;
    cout << "RGB_IN              " << pixIn << endl;
    cout << "Divide by 255       " << Div << endl;
    cout << "Pow to gamma_in "	   << Pow << endl;
    cout << "Shift by 8       "    << Shi << endl;
    cout << "Sub by 256 " 		   << Sub << endl;
    cout << "RGB_OUT             " << pixOut << endl;
    #endif

  };


/*#############################################
Gamma correction function for RGB image, speciallized based on the type of the PixIn defined and passed using template params
#############################################*/

  template<int RefValue, int CD>
  void gamma_correction(ac_ipl::RGB_imd<ac_int<CD,false> > pixIn, ac_ipl::RGB_imd<ac_int<CD,false> > &pixOut, gamma_in_type gamma_in) {
    ac_ipl::RGB_imd<ac_fixed<CD, CD, false, AC_RND> > Img, Shi,Sub;
    ac_ipl::RGB_imd<ac_fixed<gamma_in_width, gamma_in_integer_bits, false, AC_TRN> > Div,Pow;
    ac_fixed<CD, CD, false> RefPixel_value=RefValue;



    // =========== R ====================== //

    Img.R=pixIn.R;									// Read the ac_int into ac_Fixed value
    ac_math::ac_div(Img.R, RefPixel_value, Div.R); 	//Normalize to the reference value.
    ac_math::ac_pow_pwl(Div.R, gamma_in, Pow.R); 	//Raise to the power of gamma_in
    ac_math::ac_shift_left(Pow.R, CDEPTH, Shi.R); 	//Mul implemented as shift by 8 bits (Mul by 256)
    Sub.R = Shi.R-Pow.R; 							// Adjust the error in shift so we Mul by 255 (shift + Substract).
    pixOut.R= Sub.R.to_int(); 						// convert to interger value.

    // =========== G ====================== //

    Img.G=pixIn.G;
    ac_math::ac_div(Img.G, RefPixel_value, Div.G);
    ac_math::ac_pow_pwl(Div.G, gamma_in, Pow.G);
    ac_math::ac_shift_left(Pow.G, CDEPTH, Shi.G);
    Sub.G = Shi.G-Pow.G;
    pixOut.G= Sub.G.to_int();

    // =========== B ====================== //

    Img.B=pixIn.B;
    ac_math::ac_div(Img.B, RefPixel_value, Div.B);
    ac_math::ac_pow_pwl(Div.B, gamma_in, Pow.B);
    ac_math::ac_shift_left(Pow.B, CDEPTH, Shi.B);
    Sub.B = Shi.B-Pow.B;
    pixOut.B= Sub.B.to_int();

    // =========== = ====================== //

    #ifdef DEBUG
    cout << "==========================RGB Debug========================"<< endl;
    cout << "RGB_IN              " << pixIn.R << " - " << pixIn.G << " - " << pixIn.B << endl;
    cout << "Divide by 255       " << Div.R << " - " << Div.G << " - " << Div.B << endl;
    cout << "Pow to gamma_in "	   << Pow.R << " - " << Pow.G << " - " << Pow.B << endl;
    cout << "Shift by 8       "    << Shi.R << " - " << Shi.G << " - " << Shi.B << endl;
    cout << "Sub by 256 " 		   << Sub.R << " - " << Sub.G << " - " << Sub.B << endl;
    cout << "RGB_OUT _Design     " << pixOut.R << " - " << pixOut.G << " - " << pixOut.B << endl;
    #endif
  }
};



#endif
