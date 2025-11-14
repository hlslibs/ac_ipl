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
#ifndef _INCLUDED_AC_LK_H_
#define _INCLUDED_AC_LK_H_

#include <ac_int.h>
#include <ac_fixed.h>
#include <ac_ipl/ac_pixels.h>
#include <ac_window_2d_flag.h>
#include <ac_math/ac_reciprocal_pwl.h>
#include <ac_math/ac_div.h>
#include <ac_math/ac_determinant.h>

#include <ac_channel.h>
#include <mc_scverify.h>

/*####################################################################
  Templatetized ac_optical flow class.
  Template Details:
  1. IN_TYPE -> Defines the type of the Frame1 and Frame 2 input channel (RGB2PPC, 1PCC, or imd), cureently supports only RGB1PPC of CDEPTH
  2. CDEPTH -> Color Depth
  3. W_MAX -> Max width of the input frames supported.
  4. H_MAX -> Max height of the input frames supported.
  5. USE_SINGLEPORT -> set to false For DUAL PORT AC_WINDOW MEMORY
####################################################################*/

#pragma hls_design top
template <class IN_TYPE, unsigned CDEPTH, unsigned W_MAX, unsigned H_MAX, bool USE_SINGLEPORT = false>
class ac_opticalflow
{

public:

  /*####################################################################
    1. FRAC_BITS: Fractional bits used to store the results of fixed point intermediate calculations/coefficients.
    2. AC_WIN_MODE: singleport and dualport implementations are supported.
    3. SF_KS: Spatial Filter Kernel Size [5x5]
    4. CI_KS: Compute Integrals Kernel Size [11x11]
  ####################################################################*/
  enum {
    SF_KS = 5,
    CI_KS = 11,
	THRESHOLD = CI_KS *CI_KS,
    AC_WIN_MODE = USE_SINGLEPORT ? AC_BOUNDARY | AC_SINGLEPORT : AC_BOUNDARY
  };

  // Dimension types are bitwidth-constrained according to the max dimensions possible.
  typedef ac_int<ac::nbits<W_MAX>::val, false> widthInType;
  typedef ac_int<ac::nbits<H_MAX>::val, false> heightInType;

  // Typedefs for ac_fixed values and the filter coefficients.
  typedef const ac_int<3, true> SpatialFilterType;
  typedef ac_fixed<16, 4, true> fix_16_4;
 

  //ac_window data type, defined as rgb_imd, but B is unused in the Spatial derivative that gets optimized in Syntheisis.
  typedef ac_int<2*CDEPTH,false> SP_DER_FRAME_WIND;
  typedef ac_int<3*CDEPTH,false> CI_FRAME_WIND;

  #pragma hls_design interface
  void CCS_BLOCK(run) (
    ac_channel<IN_TYPE>  &FrameIn_1,    // Pixel input stream
    ac_channel<IN_TYPE>  &FrameIn_2,    // Pixel input stream
    ac_channel<IN_TYPE> &Vx,  			// Pixel output stream
    ac_channel<IN_TYPE> &Vy,  			// Pixel output stream
     const widthInType    widthIn,     	// Input width
    const heightInType   heightIn    	// Input height
  ) {
    spatialderivative(FrameIn_1, FrameIn_2, Xder, Yder, Tder, widthIn, heightIn);
    computeintegrals(Xder, Yder, Tder, XX, XY, YY, TX, TY, widthIn, heightIn);
	ComputeVectors(XX, XY, YY, TX, TY, Vx, Vy, widthIn, heightIn);
  }
  ac_opticalflow() { }

private:

  ac_channel<IN_TYPE> fil_frame1, fil_frame2;
  ac_channel<IN_TYPE> Xder, Yder, Tder;
  ac_channel<IN_TYPE> XX, XY, YY, TX, TY ;

 
  /*####################################################################
  Spacial derivative function block to compute the Ix Iy (from frame 1) and It from Frame 1 and 2.
  ####################################################################*/
  #pragma hls_pipeline_init_interval 1
  #pragma hls_design
  void spatialderivative(
    ac_channel<IN_TYPE>          &Frame1,
    ac_channel<IN_TYPE>          &Frame2,
    ac_channel<IN_TYPE>    	 	 &Ix,
    ac_channel<IN_TYPE>    	     &Iy,
    ac_channel<IN_TYPE>    	 	 &It,
    const widthInType            widthIn,
    const heightInType           heightIn
  ) {

    /*####################################################################
    AC Winodw object declared here.
    1. Window Data type SP_DER_FRAME_WIND , that is RGB _IMD of ac_int <CDEPTH_TYPE> that shall hold data from both frames.
    2. SF_KS is the Spatial filter Kernel Size.
    3. AC_WIN_MODE defines Mem type and the Boundary usage.
    ####################################################################*/
   
	ac_window_2d_flag<SP_DER_FRAME_WIND, SF_KS, SF_KS, W_MAX, AC_WIN_MODE> acWindObj(0);

    ac_int<ac::nbits<H_MAX>::val, false> i = 0;
    ac_int<ac::nbits<W_MAX>::val, false> j = 0;

    bool inRead = true, eofOut = false;
    SP_DER_FRAME_WIND pixIn;
  
    #pragma hls_pipeline_init_interval 1
    INT_CONT_LOOP: do {
      IN_TYPE Frame1_temp = inRead ? Frame1.read() : IN_TYPE(0);
      IN_TYPE Frame2_temp = inRead ? Frame2.read() : IN_TYPE(0);
      pixIn.set_slc(0,Frame1_temp);
      pixIn.set_slc(CDEPTH,Frame2_temp);

      bool sol = (j == 0);
      bool sof = (i == 0) && sol;
      bool eol = (j == widthIn - 1);
      bool eof = (i == heightIn - 1) && eol;
      acWindObj.write(pixIn, sof, eof, sol, eol);
      if (eof) {
        inRead = false;
      }
      j++;
      if (j == widthIn) {
        j = 0;
        i++;
        if (i == heightIn) {
          i = 0;
        }
      }

      bool sofOut, solOut, eolOut;
      acWindObj.readFlags(sofOut, eofOut, solOut, eolOut);
      if (acWindObj.valid()) {
		DerivativeFilter<IN_TYPE, SP_DER_FRAME_WIND, SpatialFilterType, SF_KS> (acWindObj, Ix, Iy, It);
      }
    } while (!eofOut); // Stop processing once the entire image output has been read.
  }

  /*####################################################################
  MAC Block to perform the AC_WINDOW x Filter coeeficients
  ####################################################################*/
  template<class filtOpType, class acWindType, class kType, int K_SZ>
  void DerivativeFilter(
    const ac_window_2d_flag<acWindType, K_SZ, K_SZ, W_MAX, AC_WIN_MODE> &acWindObj,
    ac_channel<filtOpType> &Hor,
    ac_channel<filtOpType> &Ver,
    ac_channel<filtOpType> &Del
  ) {

// Filter coeeficients to calculate the X derivative Filter Size is of 5x5.
    SpatialFilterType Ix_Filter[SF_KS][SF_KS] = {
      { 0,    0,    0,    0,    0},
      { 0,    0,    0,    0,    0},
      { 1,   -8,    0,    8,   -1},
      { 0,    0,    0,    0,    0},
      { 0,    0,    0,    0,    0}
    };

// Filter coeeficients to calculate the Y derivative Filter Size is of 5x5.
    SpatialFilterType Iy_Filter[SF_KS][SF_KS] = {
      { 0,    0,    1,    0,    0},
      { 0,    0,   -8,    0,    0},
      { 0,    0,    0,    0,    0},
      { 0,    0,    8,    0,    0},
      { 0,    0,   -1,    0,    0}
    };

    ac_int<32,true>  derivative_op_Hor=0, derivative_op_Ver=0, derivative_op_Del=0;
	filtOpType N_op_Hor, N_op_Ver, N_op_Del;
    acWindType acWindOut[K_SZ][K_SZ];
    #pragma hls_unroll yes
    CONV_OP_ROW_LOOP: for (int r = 0; r < int(K_SZ); r++) {
      #pragma hls_unroll yes
      CONV_OP_COL_LOOP: for (int c = 0; c < int(K_SZ); c++) {
        acWindOut[r][c] = acWindObj(r - (K_SZ/2), c - (K_SZ/2));
        derivative_op_Hor += acWindOut[r][c].template slc<CDEPTH>(0)*Ix_Filter[r][c];
        derivative_op_Ver += acWindOut[r][c].template slc<CDEPTH>(0)*Iy_Filter[r][c];
        if (r==2 & c==2) {
          derivative_op_Del = acWindOut[r][c].template slc<CDEPTH>(0)-acWindOut[r][c].template slc<CDEPTH>(CDEPTH);
        }
      }
    }

	N_op_Hor=derivative_op_Hor/12;
	N_op_Ver=derivative_op_Ver/12;
	N_op_Del=derivative_op_Del/12;
	
    Hor.write(N_op_Hor);
    Ver.write(N_op_Ver);
    Del.write(N_op_Del);
  }


  /*####################################################################
  Compute Integral block.
  Derives the Integrals, A11, A12, A22, B1 and B2, the window size here is CI_KS
  ####################################################################*/

  #pragma hls_pipeline_init_interval 1
  #pragma hls_design
  void computeintegrals(
    ac_channel<IN_TYPE>    &Ix,
    ac_channel<IN_TYPE>    &Iy,
    ac_channel<IN_TYPE>    &It,
    ac_channel<IN_TYPE>    &A11,
    ac_channel<IN_TYPE>    &A12,
    ac_channel<IN_TYPE>    &A22,
    ac_channel<IN_TYPE>    &B1,
    ac_channel<IN_TYPE>    &B2,
    const widthInType            widthIn,
    const heightInType           heightIn
  ) {

    // /*####################################################################
    // AC Winodw object declared here.
    // 1. Window Data type SP_DER_FRAME_WIND , that is RGB _IMD of ac_int <CDEPTH_TYPE> that shall hold data from both frames.
    // 2. CI_KS is the Window size for computing the integrals.
    // 3. AC_WIN_MODE defines Mem type and the Boundary usage.
    // ####################################################################*/
   
	ac_window_2d_flag<CI_FRAME_WIND, CI_KS, CI_KS, W_MAX, AC_WIN_MODE> acWindObj(0);

    ac_int<ac::nbits<H_MAX>::val, false> i = 0;
    ac_int<ac::nbits<W_MAX>::val, false> j = 0;

    bool inRead = true, eofOut = false;
    CI_FRAME_WIND value_in;
  
    #pragma hls_pipeline_init_interval 1
    INT_CONT_LOOP: do {
      IN_TYPE X_temp = inRead ? Ix.read() : IN_TYPE(0);
      IN_TYPE Y_temp = inRead ? Iy.read() : IN_TYPE(0);
      IN_TYPE T_temp = inRead ? It.read() : IN_TYPE(0);
	  
      value_in.set_slc(0,X_temp);
      value_in.set_slc(CDEPTH,Y_temp);
	  value_in.set_slc(2*CDEPTH,T_temp);

      bool sol = (j == 0);
      bool sof = (i == 0) && sol;
      bool eol = (j == widthIn - 1);
      bool eof = (i == heightIn - 1) && eol;
      acWindObj.write(value_in, sof, eof, sol, eol);
      if (eof) {
        inRead = false;
      }
      j++;
      if (j == widthIn) {
        j = 0;
        i++;
        if (i == heightIn) {
          i = 0;
        }
      }

      bool sofOut, solOut, eolOut;
      acWindObj.readFlags(sofOut, eofOut, solOut, eolOut);
      if (acWindObj.valid()) {
        IntegralFilter<IN_TYPE, CI_FRAME_WIND,  CI_KS>(acWindObj, A11, A12, A22, B1, B2);
      }
    } while (!eofOut); // Stop processing once the entire image output has been read.
  }
  /*####################################################################
  MAC Block to perform the AC_WINDOW x AC_WINDOW
  ####################################################################*/
  template<class filtOpType, class acWindType, int K_SZ>
  void IntegralFilter(
    const ac_window_2d_flag<acWindType, K_SZ, K_SZ, W_MAX, AC_WIN_MODE> &acWindObj,
    ac_channel<filtOpType>    &Axx,
    ac_channel<filtOpType>    &Axy,
    ac_channel<filtOpType>    &Ayy,
    ac_channel<filtOpType>    &Btx,
    ac_channel<filtOpType>    &Bty
  ) {
    filtOpType N_Op_Axx = 0.0,  N_Op_Axy = 0.0, N_Op_Ayy = 0.0,  N_Op_Btx = 0.0,  N_Op_Bty = 0.0;
	ac_int<32, true> filtOp_Axx = 0.0,  filtOp_Axy = 0.0, filtOp_Ayy = 0.0,  filtOp_Btx = 0.0,  filtOp_Bty = 0.0;
    // Window output is stored in temporary array. While you can also directly multiply values from
    // acWindObj and get the same QofR, having a temporary array can be more convenient for debugging.
    acWindType acWindOut[K_SZ][K_SZ];
    #pragma hls_unroll yes
    CONV_OP_ROW_LOOP: for (int r = 0; r < int(K_SZ); r++) {
      #pragma hls_unroll yes
      CONV_OP_COL_LOOP: for (int c = 0; c < int(K_SZ); c++) {
        acWindOut[r][c] = acWindObj(r - (K_SZ/2), c - (K_SZ/2));

        filtOp_Axx += acWindOut[r][c].template slc<CDEPTH>(0)*acWindOut[r][c].template slc<CDEPTH>(0);
        filtOp_Axy += acWindOut[r][c].template slc<CDEPTH>(0)*acWindOut[r][c].template slc<CDEPTH>(CDEPTH);
        filtOp_Ayy += acWindOut[r][c].template slc<CDEPTH>(CDEPTH)*acWindOut[r][c].template slc<CDEPTH>(CDEPTH);
        filtOp_Btx += acWindOut[r][c].template slc<CDEPTH>(0)*acWindOut[r][c].template slc<CDEPTH>(2*CDEPTH);
        filtOp_Bty += acWindOut[r][c].template slc<CDEPTH>(CDEPTH)*acWindOut[r][c].template slc<CDEPTH>(2*CDEPTH);
      }
    }
	
	N_Op_Axx=filtOp_Axx>>8;
	N_Op_Axy=filtOp_Axy>>8;
	N_Op_Ayy=filtOp_Ayy>>8;
	N_Op_Btx=filtOp_Btx>>8;
	N_Op_Bty=filtOp_Bty>>8;
		
    Axx.write(N_Op_Axx);
    Axy.write(N_Op_Axy);
    Ayy.write(N_Op_Ayy);
    Btx.write(N_Op_Btx);
    Bty.write(N_Op_Bty);
  }

 void matrix_invert(IN_TYPE A[2][2], IN_TYPE B[2], ac_int<32, true> threshold, ac_channel<IN_TYPE> &VxChan, ac_channel<IN_TYPE> &VyChan)
{
	IN_TYPE inv_A[2][2];
	ac_fixed<32,32,true> det_A, abs_det_A, neg_det_A;
	fix_16_4 recipr_det_A;
	ac_fixed<9,9,true> IN_MAT[2][2];

    IN_TYPE NVx, NVy;
	ac_int<32,false> Vx, Vy;
//Assign values to the array
	IN_MAT[0][0]=A[0][0];
	IN_MAT[0][1]=A[0][1];
	IN_MAT[1][0]=A[1][0];
	IN_MAT[1][1]=A[1][1];
	
	ac_int<32,true>  mult1, mult2, mult3, mult4;
	ac_int<32,true> t_Vx, t_Vy;
    ac_math::ac_determinant(IN_MAT, det_A);
	
	neg_det_A = -det_A;
	abs_det_A = (det_A > 0) ? det_A : neg_det_A;
	
	ac_math::ac_reciprocal_pwl(ac_fixed<32,32,true>(det_A), recipr_det_A);
	
	if (det_A == 0) recipr_det_A = 0.0;
	inv_A[0][0] =  A[1][1];
	inv_A[0][1] = -A[0][1];
	inv_A[1][0] = -A[1][0];
	inv_A[1][1] =  A[0][0];

	mult1 = inv_A[0][0] * B[0];
	mult2 = inv_A[0][1] * B[1];
	mult3 = mult2; 
	mult4 =  inv_A[1][1] *  B[1];
	t_Vx = -(mult1 + mult2);
	t_Vy = -(mult3 + mult4);

	Vx = (t_Vx * recipr_det_A).to_int();
	Vy = (t_Vy * recipr_det_A).to_int();

	if (det_A == 0) 
	{
		Vx = 0; Vy = 0;
	}
	else if (abs_det_A < threshold) 
	{
		Vx = 0; Vy = 0;
	}

	VxChan.write(Vx>>8);
    VyChan.write(Vy>>8); 
}



  #pragma hls_pipeline_init_interval 1
  #pragma hls_design
void ComputeVectors(
  ac_channel<IN_TYPE>    &A11,
  ac_channel<IN_TYPE>    &A12,
  ac_channel<IN_TYPE>    &A22,
  ac_channel<IN_TYPE>    &B1,
  ac_channel<IN_TYPE>    &B2,
  ac_channel<IN_TYPE> &vx_img,
  ac_channel<IN_TYPE> &vy_img,
  const widthInType            widthIn,
  const heightInType           heightIn)

{
  int row, col;
  IN_TYPE A[2][2];
  IN_TYPE  B[2];
  #pragma hls_pipeline_init_interval 1
  L1: for(row = 0; row < heightIn; row++)
  {
  #pragma hls_pipeline_init_interval 1
  L2: for(col = 0; col < widthIn; col++)
  {

  A[0][0] = A11.read();	//a11
  A[0][1] = A12.read(); //a12;
  A[1][0] = A[0][1]; 	//a21
  A[1][1] = A22.read(); //a22;
  B[0]    =  B1.read(); //b1
  B[1]    =  B2.read(); //b2
  matrix_invert(A, B, THRESHOLD, vx_img, vy_img);

  }
  }
}
  
};

#endif
