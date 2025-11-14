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
#include <ac_ipl/ac_gaussian_pyr.h>

#include <string>
#include <fstream>
#include <iostream>
using namespace std;
#include <bmpUtil/bmp_io.cpp>

template <int W_MAX, int N_LEVELS>
struct outWMaxCalc {
  enum {
    W_MAX_FINAL = W_MAX/2 + int(W_MAX%2 != 0),
    OUT_W_MAX_FINAL = W_MAX_FINAL + outWMaxCalc<W_MAX_FINAL, N_LEVELS - 1>::OUT_W_MAX_FINAL,
  };
};

template <int W_MAX>
struct outWMaxCalc<W_MAX, 1> {
  enum {
    W_MAX_FINAL = W_MAX/2 + int(W_MAX%2 != 0),
    OUT_W_MAX_FINAL = W_MAX_FINAL,
  };
};

// Generic struct to find number of integer bits in input type. Specialization added later.
template <class T>
struct find_int_bits { };

// find_int_bits, specialized for unsigned ac_int types.
template <int W>
struct find_int_bits<ac_int<W, false> > {
  enum {
    I_BITS = W,
  };
};

// find_int_bits, specialized for RGB_imd of unsigned ac_int types.
template <int W>
struct find_int_bits<ac_ipl::RGB_imd<ac_int<W, false> > > {
  enum {
    I_BITS = W,
  };
};

template <int CDEPTH>
void initArrPixIn(
  const unsigned char R, const unsigned char G, const unsigned char B,
  unsigned char& invR, unsigned char& invG, unsigned char& invB,
  ac_int<CDEPTH, false> &pixIn
) {
  // ac_fixed values are used here instead of double values; this is done to ensure consistency between 32- and 64-bit compilations.
  ac_fixed<64, 0, false> coeff1 = 0.299, coeff2 = 0.587, coeff3 = 0.114;
  typedef ac_fixed<CDEPTH, CDEPTH, false> T_fi;
  int pixVal = (coeff1*T_fi(R) + coeff2*T_fi(G) + coeff3*T_fi(B)).to_int();
  invR = pixVal;
  invG = pixVal;
  invB = pixVal;
  pixIn = pixVal;
}

template <int CDEPTH>
void initArrPixIn(
  const unsigned char R, const unsigned char G, const unsigned char B,
  unsigned char& invR, unsigned char& invG, unsigned char& invB,
  ac_ipl::RGB_imd<ac_int<CDEPTH, false> >& pixIn
) {
  invR = R;
  invG = G;
  invB = B;
  pixIn.R = int(R);
  pixIn.G = int(G);
  pixIn.B = int(B);
}

void clipInd(const int i, const int j, const int r, const int c, int &iClip, int &jClip) {
  if (i < 0) {
    iClip = 0;
  } else if (i >= r) {
    iClip = r - 1;
  } else {
    iClip = i;
  }

  if (j < 0) {
    jClip = 0;
  } else if (j >= c) {
    jClip = c - 1;
  } else {
    jClip = j;
  }
}

template <int W1, int I, ac_q_mode Q1, ac_o_mode O1, int W2, ac_q_mode Q2, ac_o_mode O2>
bool compareOut(ac_fixed<W1, I, false, Q1, O1> outputDes, ac_ipl::RGB_imd<ac_fixed<W2, I, false, Q2, O2> > accVar) {
  typedef ac_fixed<W1, I, false, Q1, O1> outType;
  return (outputDes != outType(accVar.R) || outputDes != outType(accVar.R) || outputDes != outType(accVar.B));
}

template <int W1, int I, ac_q_mode Q1, ac_o_mode O1, int W2, ac_q_mode Q2, ac_o_mode O2>
bool compareOut(ac_ipl::RGB_imd<ac_fixed<W1, I, false, Q1, O1> > outputDes, ac_ipl::RGB_imd<ac_fixed<W2, I, false, Q2, O2> > accVar) {
  ac_ipl::RGB_imd<ac_fixed<W1, I, false, Q1, O1> > accVar2 = accVar;
  return outputDes != accVar2;
}

template <class IN_TYPE, class OUT_TYPE, int W_MAX, int H_MAX, int N_LEVELS, int MAX_FRAC_BITS, bool USE_SP>
bool test_driver(
  string inf_name,
  string outf_name
) {
  enum {
    IN_I = find_int_bits<IN_TYPE>::I_BITS,
    OUT_I = gaussian_pyr_type_info<OUT_TYPE, 0, 0>::CDEPTH,
    OUT_H_MAX = H_MAX/2 + int(H_MAX%2 != 0),
    OUT_W_MAX = outWMaxCalc<W_MAX, N_LEVELS>::OUT_W_MAX_FINAL,
  };
  
  static_assert(IN_I == OUT_I, "Input and output types don't have the same number of integer bits.");

  cout << "TEST: ac_gaussian_pyr INPUT: ";
  cout.width(25);
  cout << left << IN_TYPE::type_name();
  cout << "OUTPUT: ";
  cout.width(45);
  cout << left << OUT_TYPE::type_name();
  cout << "USE_SP: ";
  cout.width(6);
  cout << left << (USE_SP ? "true" : "false");
  cout << "RESULT: ";
  
  unsigned long width;
  long height;
  // Declare input/output pixel arrays, one for each color component.
  unsigned char *in_rarray_1 = new unsigned char[H_MAX*W_MAX];
  unsigned char *in_garray_1 = new unsigned char[H_MAX*W_MAX];
  unsigned char *in_barray_1 = new unsigned char[H_MAX*W_MAX];
  unsigned char *in_rarray_2 = new unsigned char[H_MAX*W_MAX];
  unsigned char *in_garray_2 = new unsigned char[H_MAX*W_MAX];
  unsigned char *in_barray_2 = new unsigned char[H_MAX*W_MAX];
  unsigned char *out_rarray = new unsigned char[OUT_H_MAX*OUT_W_MAX];
  unsigned char *out_garray = new unsigned char[OUT_H_MAX*OUT_W_MAX];
  unsigned char *out_barray = new unsigned char[OUT_H_MAX*OUT_W_MAX];
  ac_ipl::RGB_imd<double> *out_array_d = new ac_ipl::RGB_imd<double>[OUT_H_MAX*OUT_W_MAX];
  
  for (int i = 0; i < OUT_H_MAX; i++) {
    for (int j = 0; j < OUT_W_MAX; j++) {
      out_rarray[i*OUT_W_MAX + j] = 0;
      out_garray[i*OUT_W_MAX + j] = 0;
      out_barray[i*OUT_W_MAX + j] = 0;
    }
  }
  
  bool read_fail = bmp_read((char *)inf_name.c_str(), &width, &height, &in_rarray_1, &in_garray_1, &in_barray_1);
  
  if (read_fail) {
    return false;
  }
  
  // Declare IO channels.
  ac_channel<IN_TYPE> streamIn;
  ac_channel<OUT_TYPE> streamOut[N_LEVELS];
  
  const int heightInt = int(height), widthInt = int(width);
  
  // Read input image in reverse row order and store them in properly ordered array.
  for (int i = heightInt - 1; i >= 0; i--) {
    for (int j = 0; j < widthInt; j++) {
      IN_TYPE pixIn;
      int ind = i*widthInt + j;
      int indInv = (heightInt - 1 - i)*widthInt + j;
      initArrPixIn(in_rarray_1[ind], in_garray_1[ind], in_barray_1[ind], in_rarray_2[indInv], in_garray_2[indInv], in_barray_2[indInv], pixIn);
      streamIn.write(pixIn);
    }
  }
  
  ac_gaussian_pyr<IN_TYPE, OUT_TYPE, W_MAX, H_MAX, N_LEVELS, MAX_FRAC_BITS, USE_SP> gaussPyrInst;
  gaussPyrInst.run(streamIn, streamOut, widthInt, heightInt);
  
  if (streamIn.debug_size() != 0) {
    cout << "FAILED. Image input not completely consumed by design." << endl;
    return false;
  }

  const int K_SZ = 5;

  const double gaussKernel[K_SZ][K_SZ] = {
    {0.00390625, 0.015625, 0.0234375, 0.015625,	0.00390625},
    {0.015625,   0.06250,  0.09375,   0.0625,	0.015625},
    {0.0234375,	 0.09375,  0.140625,  0.09375,	0.0234375},
    {0.015625,	 0.0625,   0.09375,   0.0625,   0.015625},
    {0.00390625, 0.015625, 0.0234375, 0.015625,	0.00390625}
  };

  int heightLev[N_LEVELS], widthOutTotal = 0, widthLev[N_LEVELS];

  for (int levCnt = 0; levCnt < N_LEVELS; levCnt++) {
    int widthLevInput = (levCnt == 0) ? widthInt : widthLev[levCnt - 1];
    widthLev[levCnt] = widthLevInput/2 + widthLevInput%2;
    widthOutTotal += widthLev[levCnt];
    int heightLevInput = (levCnt == 0) ? heightInt : heightLev[levCnt - 1];
    heightLev[levCnt] = heightLevInput/2 + heightLevInput%2;
  }

  int cOffsetOut = 0;

  for (int levCnt = 0; levCnt < N_LEVELS; levCnt++) {
    int widthLevInput = (levCnt == 0) ? widthInt : widthLev[levCnt - 1];
    int heightLevInput = (levCnt == 0) ? heightInt : heightLev[levCnt - 1];
    
    if (streamOut[levCnt].debug_size() != heightLev[levCnt]*widthLev[levCnt]) {
      cout << "Incorrect number of outputs produced by design, for level " << (levCnt + 1) << ". Test FAILED." << endl;
      return false;
    }

    for (int i = 0; i < heightLev[levCnt]; i++) {
      for (int j = 0; j < widthLev[levCnt]; j++) {
        ac_ipl::RGB_imd<ac_fixed<8 + MAX_FRAC_BITS, 8, false> > accVar = 0.0;
        ac_ipl::RGB_imd<double> winValD[K_SZ][K_SZ];
        for (int k = -K_SZ/2; k <= K_SZ/2; k++) {
          for (int l = -K_SZ/2; l <= K_SZ/2; l++) {
            int iClip, jClip;
            clipInd(2*i + k, 2*j + l, heightLevInput, widthLevInput, iClip, jClip);
            ac_ipl::RGB_imd<double> inVal;
            if (levCnt == 0) {
              inVal.R = double(in_rarray_2[iClip*widthInt + jClip]);
              inVal.G = double(in_garray_2[iClip*widthInt + jClip]);
              inVal.B = double(in_barray_2[iClip*widthInt + jClip]);
            } else {
              int cOffsetIn = cOffsetOut - widthLev[levCnt - 1];
              inVal = out_array_d[iClip*widthOutTotal + jClip + cOffsetIn];
            }
            winValD[k + K_SZ/2][l + K_SZ/2] = inVal;
            double kernelVal = gaussKernel[k + K_SZ/2][l + K_SZ/2];
            accVar.R = accVar.R.to_double() + winValD[k + K_SZ/2][l + K_SZ/2].R*kernelVal;
            accVar.G = accVar.G.to_double() + winValD[k + K_SZ/2][l + K_SZ/2].G*kernelVal;
            accVar.B = accVar.B.to_double() + winValD[k + K_SZ/2][l + K_SZ/2].B*kernelVal;
          }
        }
        int outInd = i*widthOutTotal + j + cOffsetOut;
        int outIndInv = (heightLev[0] - 1 - i)*widthOutTotal + j + cOffsetOut;
        out_array_d[outInd].R = accVar.R.to_double();
        out_array_d[outInd].G = accVar.G.to_double();
        out_array_d[outInd].B = accVar.B.to_double();
        out_rarray[outIndInv] = (ac_fixed<8, 8, false, AC_RND>(accVar.R)).to_int();
        out_garray[outIndInv] = (ac_fixed<8, 8, false, AC_RND>(accVar.G)).to_int();
        out_barray[outIndInv] = (ac_fixed<8, 8, false, AC_RND>(accVar.B)).to_int();
        OUT_TYPE outputDes = streamOut[levCnt].read();
        if (compareOut(outputDes, accVar)) {
          cout << "Reference and design output(s) do not match. Test FAILED." << endl;
          #ifdef DEBUG
          cout << "levCnt = " << levCnt << endl;
          cout << "i = " << i << endl;
          cout << "j = " << j << endl;
          #endif
          return false;
        }
      }
    }
    cOffsetOut += widthLev[levCnt];
  }
  
  bool write_fail = bmp_24_write((char *)outf_name.c_str(), widthOutTotal,  heightLev[0], out_rarray, out_garray, out_barray);
  if (write_fail) {
    return false;
  }

  delete[] in_rarray_1;
  delete[] in_garray_1;
  delete[] in_barray_1;
  delete[] in_rarray_2;
  delete[] in_garray_2;
  delete[] in_barray_2;
  delete[] out_rarray;
  delete[] out_garray;
  delete[] out_barray;
  delete[] out_array_d;

  cout << "PASSED." << endl;
  
  return true;
}

int main(int argc, char *argv[])
{
  cout << "==================================================================================" << endl;
  cout << "--------------------- Running rtest_ac_gaussian_pyr.cpp --------------------------" << endl;
  cout << "==================================================================================" << endl;

  bool all_tests_pass = true;

  //template <class IN_TYPE, class OUT_TYPE, int W_MAX, int H_MAX, int N_LEVELS, int MAX_FRAC_BITS, bool USE_SP>
  all_tests_pass = test_driver<ac_int<8, false>, ac_fixed<18, 8, false>, 1024, 1024, 5, 16, true >("in_image.bmp", "out_image_gaussian_pyr_GS_SP_e_w.bmp") && all_tests_pass;
  all_tests_pass = test_driver<ac_int<8, false>, ac_fixed<18, 8, false>, 1024, 1024, 5, 16, false>("in_image.bmp", "out_image_gaussian_pyr_GS_DP_e_w.bmp") && all_tests_pass;
  all_tests_pass = test_driver<ac_ipl::RGB_imd<ac_int<8, false> >, ac_ipl::RGB_imd<ac_fixed<18, 8, false> >, 1024, 1024, 5, 16, true >("in_image.bmp", "out_image_gaussian_pyr_RGB_SP_e_w.bmp") && all_tests_pass;
  all_tests_pass = test_driver<ac_ipl::RGB_imd<ac_int<8, false> >, ac_ipl::RGB_imd<ac_fixed<18, 8, false> >, 1024, 1024, 5, 16, false>("in_image.bmp", "out_image_gaussian_pyr_RGB_DP_e_w.bmp") && all_tests_pass;
  
  all_tests_pass = test_driver<ac_int<8, false>, ac_fixed<18, 8, false>, 1024, 1024, 5, 16, true >("in_image_o_w.bmp", "out_image_gaussian_pyr_GS_SP_o_w.bmp") && all_tests_pass;
  all_tests_pass = test_driver<ac_int<8, false>, ac_fixed<18, 8, false>, 1024, 1024, 5, 16, false>("in_image_o_w.bmp", "out_image_gaussian_pyr_GS_DP_o_w.bmp") && all_tests_pass;
  all_tests_pass = test_driver<ac_ipl::RGB_imd<ac_int<8, false> >, ac_ipl::RGB_imd<ac_fixed<18, 8, false> >, 1024, 1024, 5, 16, true >("in_image_o_w.bmp", "out_image_gaussian_pyr_RGB_SP_o_w.bmp") && all_tests_pass;
  all_tests_pass = test_driver<ac_ipl::RGB_imd<ac_int<8, false> >, ac_ipl::RGB_imd<ac_fixed<18, 8, false> >, 1024, 1024, 5, 16, false>("in_image_o_w.bmp", "out_image_gaussian_pyr_RGB_DP_o_w.bmp") && all_tests_pass;

  cout << "  Testbench finished." << endl;
  
  // Notify the user whether or not the test was a failure.
  if (!all_tests_pass) {
    cout << "  ac_gaussian_pyr - FAILED" << endl;
    return -1;
  }
  
  return 0; 
}
