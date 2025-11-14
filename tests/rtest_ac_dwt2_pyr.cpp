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
#include <ac_ipl/ac_dwt2_pyr.h>

#include <string>
#include <fstream>
#include <iostream>
using namespace std;
#include <bmpUtil/bmp_io.cpp>

// Struct to calculate maximum possible size of out_array_d image via templatized recursion.
template <int W_MAX, int N_LEVELS>
struct outWMaxCalc {
  enum {
    W_MAX_FINAL = W_MAX/2,
    OUT_W_MAX_FINAL = W_MAX_FINAL + outWMaxCalc<W_MAX_FINAL, N_LEVELS - 1>::OUT_W_MAX_FINAL,
  };
};

template <int W_MAX>
struct outWMaxCalc<W_MAX, 1> {
  enum {
    W_MAX_FINAL = W_MAX/2,
    OUT_W_MAX_FINAL = W_MAX_FINAL,
  };
};

// Helper function to initialize pixel input array, for greyscale image input.
template <int CDEPTH>
void initArrPixIn(
  const unsigned char R, const unsigned char G, const unsigned char B, // Pixel input.
  unsigned char &inv_R, unsigned char &inv_G, unsigned char &inv_B, // Pixel output used to initialize inverted array.
  ac_int<CDEPTH, false> &pix_in // Pixel input streamed to ac_dwt2_pyr design.
)
{
  // ac_fixed values are used here instead of double values; this is done to ensure consistency between 32- and 64-bit compilations.
  ac_fixed<64, 0, false> coeff1 = 0.299, coeff2 = 0.587, coeff3 = 0.114;
  typedef ac_fixed<CDEPTH, CDEPTH, false> T_fi;
  // Convert color pixel to greyscale value.
  int pix_val = (coeff1*T_fi(R) + coeff2*T_fi(G) + coeff3*T_fi(B)).to_int();
  inv_R = inv_G = inv_B = pix_val;
  pix_in = pix_val;
}

// Helper function to initialize pixel input array, for RGB image input.
template <int CDEPTH>
void initArrPixIn(
  const unsigned char R, const unsigned char G, const unsigned char B, // Pixel input.
  unsigned char &inv_R, unsigned char &inv_G, unsigned char &inv_B, // Pixel output used to initialize inverted array.
  ac_ipl::RGB_imd<ac_int<CDEPTH, false> > &pix_in // Pixel input streamed to ac_dwt2_pyr design.
)
{
  inv_R = R;
  inv_G = G;
  inv_B = B;
  pix_in.R = int(R);
  pix_in.G = int(G);
  pix_in.B = int(B);
}

// Helper function to produce indices for input pixel clipping.
void clipInd(const int i, const int j, const int r, const int c, int &i_clip, int &j_clip)
{
  if (i < 0) {
    i_clip = 0;
  } else if (i >= r) {
    i_clip = r - 1;
  } else {
    i_clip = i;
  }

  if (j < 0) {
    j_clip = 0;
  } else if (j >= c) {
    j_clip = c - 1;
  } else {
    j_clip = j;
  }
}

// Compare greyscale design output with testbench output. Return "true" if mismatch is observed.
template <int W1, int I1, bool S1, ac_q_mode Q1, ac_o_mode O1, int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
bool compareOut (
  out_struct<ac_fixed<W1, I1, S1, Q1, O1> > output_des,
  out_struct<ac_ipl::RGB_imd<ac_fixed<W2, I2, S2, Q2, O2> > > acc_var
)
{
  typedef ac_fixed<W1, I1, S1, Q1, O1> OUT_TYPE;

  bool output = false;
  output = output || (output_des.out_ap != OUT_TYPE(acc_var.out_ap.R)) || (output_des.out_ap != OUT_TYPE(acc_var.out_ap.G)) || (output_des.out_ap != OUT_TYPE(acc_var.out_ap.B));
  output = output || (output_des.out_ho != OUT_TYPE(acc_var.out_ho.R)) || (output_des.out_ho != OUT_TYPE(acc_var.out_ho.G)) || (output_des.out_ho != OUT_TYPE(acc_var.out_ho.B));
  output = output || (output_des.out_ve != OUT_TYPE(acc_var.out_ve.R)) || (output_des.out_ve != OUT_TYPE(acc_var.out_ve.G)) || (output_des.out_ve != OUT_TYPE(acc_var.out_ve.B));
  output = output || (output_des.out_di != OUT_TYPE(acc_var.out_di.R)) || (output_des.out_di != OUT_TYPE(acc_var.out_di.G)) || (output_des.out_di != OUT_TYPE(acc_var.out_di.B));
  return output;
}

// Compare color design output with testbench output. Return "true" if mismatch is observed.
template <int W1, int I1, bool S1, ac_q_mode Q1, ac_o_mode O1, int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
bool compareOut (
  out_struct<ac_ipl::RGB_imd<ac_fixed<W1, I1, S1, Q1, O1> > > output_des,
  out_struct<ac_ipl::RGB_imd<ac_fixed<W2, I2, S2, Q2, O2> > > acc_var
)
{
  typedef ac_ipl::RGB_imd<ac_fixed<W1, I1, S1, Q1, O1> > OUT_TYPE;
  return (output_des.out_ap != OUT_TYPE(acc_var.out_ap) || output_des.out_ho != OUT_TYPE(acc_var.out_ho) || output_des.out_ve != OUT_TYPE(acc_var.out_ve) || output_des.out_di != OUT_TYPE(acc_var.out_di));
}

// MAC operation.
template <int W1, int I1, bool S1, ac_q_mode Q1, ac_o_mode O1, int W2, int I2, bool S2, ac_q_mode Q2, ac_o_mode O2>
void macOp(const ac_ipl::RGB_imd<double> win_val_d_elem, ac_fixed<W1, I1, S1, Q1, O1> kernel_val, ac_ipl::RGB_imd<ac_fixed<W2, I2, S2, Q2, O2> > &acc_var)
{
  acc_var.R = acc_var.R + ac_fixed<64, 32, true>(win_val_d_elem.R)*kernel_val;
  acc_var.G = acc_var.G + ac_fixed<64, 32, true>(win_val_d_elem.G)*kernel_val;
  acc_var.B = acc_var.B + ac_fixed<64, 32, true>(win_val_d_elem.B)*kernel_val;
}

// Convert ac_fixed accumulator output to double.
template <int W, int I, bool S, ac_q_mode Q, ac_o_mode O>
out_struct<ac_ipl::RGB_imd<double> > oSDoubleConv(const out_struct<ac_ipl::RGB_imd<ac_fixed<W, I, S, Q, O> > > acc_var)
{
  out_struct<ac_ipl::RGB_imd<double> > out_conv;
  out_conv.out_ap.R = acc_var.out_ap.R.to_double();
  out_conv.out_ap.G = acc_var.out_ap.G.to_double();
  out_conv.out_ap.B = acc_var.out_ap.B.to_double();
  out_conv.out_ho.R = acc_var.out_ho.R.to_double();
  out_conv.out_ho.G = acc_var.out_ho.G.to_double();
  out_conv.out_ho.B = acc_var.out_ho.B.to_double();
  out_conv.out_ve.R = acc_var.out_ve.R.to_double();
  out_conv.out_ve.G = acc_var.out_ve.G.to_double();
  out_conv.out_ve.B = acc_var.out_ve.B.to_double();
  out_conv.out_di.R = acc_var.out_di.R.to_double();
  out_conv.out_di.G = acc_var.out_di.G.to_double();
  out_conv.out_di.B = acc_var.out_di.B.to_double();
  return out_conv;
}

template <ac_dwt2_function DWT_FN_VAL, class IN_TYPE, class OUT_TYPE, int W_MAX, int H_MAX, int N_LEVELS, int ACC_W, int ACC_I, bool USE_SP>
bool test_driver(string inf_name, string outf_name) {
  enum {
    OUT_H_MAX = H_MAX/2 + int(H_MAX%2 != 0),
    OUT_W_MAX = outWMaxCalc<W_MAX, N_LEVELS>::OUT_W_MAX_FINAL,
  };

  cout << "TEST: ac_dwt2_pyr INPUT: ";
  cout.width(25);
  cout << left << IN_TYPE::type_name();
  cout << "OUTPUT: ";
  cout.width(45);
  cout << left << OUT_TYPE::type_name();
  cout << "USE_SP: ";
  cout.width(6);
  cout << left << (USE_SP ? "true" : "false");
  cout << "inf_name: ";
  cout.width(17);
  cout << left << inf_name;
  cout << "RESULT: ";
  
  unsigned long width;
  long height;
  
  unsigned char *in_rarray_1 = new unsigned char[H_MAX*W_MAX];
  unsigned char *in_garray_1 = new unsigned char[H_MAX*W_MAX];
  unsigned char *in_barray_1 = new unsigned char[H_MAX*W_MAX];
  // in_*array_2: Use to store the inverted input image. This inversion is necessary to correct the inverted representation of stored BMP image pixels.
  unsigned char *in_rarray_2 = new unsigned char[H_MAX*W_MAX];
  unsigned char *in_garray_2 = new unsigned char[H_MAX*W_MAX];
  unsigned char *in_barray_2 = new unsigned char[H_MAX*W_MAX];
  unsigned char *out_rarray = new unsigned char[H_MAX*W_MAX];
  unsigned char *out_garray = new unsigned char[H_MAX*W_MAX];
  unsigned char *out_barray = new unsigned char[H_MAX*W_MAX];
  out_struct<ac_ipl::RGB_imd<double> > *out_array_d = new out_struct<ac_ipl::RGB_imd<double> >[OUT_H_MAX*OUT_W_MAX];
  
  bool read_fail = bmp_read((char *)inf_name.c_str(), &width, &height, &in_rarray_1, &in_garray_1, &in_barray_1);
  
  if (read_fail) {
    return false;
  }

  const int height_int = int(height), width_int = int(width);

  ac_channel<IN_TYPE> stream_in;
  ac_channel<out_struct<OUT_TYPE> > stream_out[N_LEVELS];

  for (int i = height_int - 1; i >= 0; i--) {
    for (int j = 0; j < width_int; j++) {
      IN_TYPE pix_in;
      int ind_inv = (height_int - 1 - i)*width_int + j;
      int ind = i*width_int + j;
      // Invert the input image and store the inverted pixels in in_*array_2, to correct the inverted representation of stored BMP image pixels.
      // Also initialize the stream_ing pixel input.
      initArrPixIn(in_rarray_1[ind], in_garray_1[ind], in_barray_1[ind], in_rarray_2[ind_inv], in_garray_2[ind_inv], in_barray_2[ind_inv], pix_in);
      // Write input pixel to stream_in channel.
      stream_in.write(pix_in);
    }
  }
  
  ac_dwt2_pyr<DWT_FN_VAL, IN_TYPE, OUT_TYPE, W_MAX, H_MAX, N_LEVELS, ACC_W, ACC_I, USE_SP> dwt2_pyr_inst;
  dwt2_pyr_inst.run(stream_in, stream_out, width_int, height_int);
  
  if (stream_in.debug_size() != 0) {
    cout << "FAILED. Image input not completely consumed by design." << endl;
    return false;
  }

  typedef kernel_values_struct<DWT_FN_VAL> kernel_values_struct_type;

  const int K_SZ = kernel_values_struct_type::K_SZ;

  // Store the width and height for each level input in the width_lev and height_lev arrays. Also store the total dynamic output image width in width_out_total.
  int height_lev[N_LEVELS], width_out_total = 0, width_lev[N_LEVELS];

  for (int lev_cnt = 0; lev_cnt < N_LEVELS; lev_cnt++) {
    int width_lev_input = (lev_cnt == 0) ? width_int : width_lev[lev_cnt - 1];
    // width after conducting downsampling = floor(level_input_width/2). Same goes for height.
    width_lev[lev_cnt] = width_lev_input/2;
    // Outputs of each level are store side-by-side in the final output image, from left to right.
    // Hence, the total output width is the output image width for every level, combined.
    width_out_total += width_lev[lev_cnt];
    int height_lev_input = (lev_cnt == 0) ? height_int : height_lev[lev_cnt - 1];
    height_lev[lev_cnt] = height_lev_input/2;
  }

  int c_offset_out = 0;

  // lev_cnt = 0 for Level 1, 1 for Level 2, 2 for Level 3, and so on...
  for (int lev_cnt = 0; lev_cnt < N_LEVELS; lev_cnt++) {
    int width_lev_input = (lev_cnt == 0) ? width_int : width_lev[lev_cnt - 1];
    int height_lev_input = (lev_cnt == 0) ? height_int : height_lev[lev_cnt - 1];

    if (stream_out[lev_cnt].debug_size() != height_lev[lev_cnt]*width_lev[lev_cnt]) {
      cout << "FAILED. Incorrect number of outputs produced by design, for level " << (lev_cnt + 1) << "." << endl;
      return false;
    }

    for (int i = 0; i < height_lev[lev_cnt]; i++) {
      for (int j = 0; j < width_lev[lev_cnt]; j++) {
        out_struct<ac_ipl::RGB_imd<ac_fixed<ACC_W, ACC_I, true> > > acc_var;
        acc_var.out_ap = acc_var.out_ho = acc_var.out_ve = acc_var.out_di = 0.0;
        ac_ipl::RGB_imd<double> win_val_d[K_SZ][K_SZ]; // Temporary array to store window inputs. Makes debugging in gdb easier.
        for (int k = -K_SZ/2; k <= (K_SZ/2 - 1); k++) {
          for (int l = -K_SZ/2; l <= (K_SZ/2 - 1); l++) {
            int i_clip, j_clip;
            clipInd(2*i + k + 1, 2*j + l + 1, height_lev_input, width_lev_input, i_clip, j_clip);
            ac_ipl::RGB_imd<double> in_val;
            // Write input pixel value.
            if (lev_cnt == 0) {
              int clip1D = i_clip*width_int + j_clip;
              // If we are at level 1, extract pixel from input image.
              in_val.R = double(in_rarray_2[clip1D]);
              in_val.G = double(in_garray_2[clip1D]);
              in_val.B = double(in_barray_2[clip1D]);
            } else {
              // If we are at a level > 1, extract pixel from previous level that was stored in out_array_d during the prior lev_cnt iteration.
              int cOffsetIn = c_offset_out - width_lev[lev_cnt - 1];
              in_val = out_array_d[i_clip*width_out_total + j_clip + cOffsetIn].out_ap;
            }
            win_val_d[k + K_SZ/2][l + K_SZ/2] = in_val;
            kernel_values_struct_type k_v_i;
            macOp(win_val_d[k + K_SZ/2][l + K_SZ/2], k_v_i.k_ap[k + K_SZ/2][l + K_SZ/2], acc_var.out_ap);
            macOp(win_val_d[k + K_SZ/2][l + K_SZ/2], k_v_i.k_ho[k + K_SZ/2][l + K_SZ/2], acc_var.out_ho);
            macOp(win_val_d[k + K_SZ/2][l + K_SZ/2], k_v_i.k_ve[k + K_SZ/2][l + K_SZ/2], acc_var.out_ve);
            macOp(win_val_d[k + K_SZ/2][l + K_SZ/2], k_v_i.k_di[k + K_SZ/2][l + K_SZ/2], acc_var.out_di);
          }
        }

        // Index used to store output pixel value in out_array_d to allow for calculation of outputs for level > 1
        int op_idx = i*width_out_total + j + c_offset_out;
        int op_idx_iv_ap = (height_int - 1 - i)*width_int + j;
        int op_idx_iv_ho = (height_int - 1 - i - height_lev[lev_cnt])*width_int + j;
        int op_idx_iv_ve = (height_int - 1 - i)*width_int + j + width_lev[lev_cnt];
        int op_idx_iv_di = (height_int - 1 - i - height_lev[lev_cnt])*width_int + j + width_lev[lev_cnt];
        out_array_d[op_idx] = oSDoubleConv(acc_var);
        
        out_rarray[op_idx_iv_ap] = (ac_fixed<8, 8, false, AC_RND, AC_SAT>(acc_var.out_ap.R)).to_int();
        out_garray[op_idx_iv_ap] = (ac_fixed<8, 8, false, AC_RND, AC_SAT>(acc_var.out_ap.G)).to_int();
        out_barray[op_idx_iv_ap] = (ac_fixed<8, 8, false, AC_RND, AC_SAT>(acc_var.out_ap.B)).to_int();

        out_rarray[op_idx_iv_ho] = (ac_fixed<8, 8, false, AC_RND, AC_SAT>(acc_var.out_ho.R)).to_int();
        out_garray[op_idx_iv_ho] = (ac_fixed<8, 8, false, AC_RND, AC_SAT>(acc_var.out_ho.G)).to_int();
        out_barray[op_idx_iv_ho] = (ac_fixed<8, 8, false, AC_RND, AC_SAT>(acc_var.out_ho.B)).to_int();

        out_rarray[op_idx_iv_ve] = (ac_fixed<8, 8, false, AC_RND, AC_SAT>(acc_var.out_ve.R)).to_int();
        out_garray[op_idx_iv_ve] = (ac_fixed<8, 8, false, AC_RND, AC_SAT>(acc_var.out_ve.G)).to_int();
        out_barray[op_idx_iv_ve] = (ac_fixed<8, 8, false, AC_RND, AC_SAT>(acc_var.out_ve.B)).to_int();

        out_rarray[op_idx_iv_di] = (ac_fixed<8, 8, false, AC_RND, AC_SAT>(acc_var.out_di.R)).to_int();
        out_garray[op_idx_iv_di] = (ac_fixed<8, 8, false, AC_RND, AC_SAT>(acc_var.out_di.G)).to_int();
        out_barray[op_idx_iv_di] = (ac_fixed<8, 8, false, AC_RND, AC_SAT>(acc_var.out_di.B)).to_int();

        out_struct<OUT_TYPE> output_des = stream_out[lev_cnt].read();
        if (compareOut(output_des, acc_var)) {
          cout << "FAILED. Reference and design output(s) do not match." << endl;
          #ifdef DEBUG
          cout << "lev_cnt = " << lev_cnt << endl;
          cout << "i = " << i << endl;
          cout << "j = " << j << endl;
          cout << "output_des.out_ap = " << output_des.out_ap << endl;
          cout << "output_des.out_ho = " << output_des.out_ho << endl;
          cout << "output_des.out_ve = " << output_des.out_ve << endl;
          cout << "output_des.out_di = " << output_des.out_di << endl;
          cout << "acc_var.out_ap = " << acc_var.out_ap << endl;
          cout << "acc_var.out_ho = " << acc_var.out_ho << endl;
          cout << "acc_var.out_ve = " << acc_var.out_ve << endl;
          cout << "acc_var.out_di = " << acc_var.out_di << endl;
          #endif
          return false;
        }
      }
    }
    c_offset_out += width_lev[lev_cnt];
  }
  
  bool write_fail = bmp_24_write((char *)outf_name.c_str(), width, height, out_rarray, out_garray, out_barray);
  if (write_fail) {
    return false;
  }
  
  // Delete pointers to dynamically allocated arrays, so as to avoid memory leaks.
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
  cout << "==============================================================================" << endl;
  cout << "--------------------- Running rtest_ac_dwt2_pyr.cpp --------------------------" << endl;
  cout << "==============================================================================" << endl;

  bool all_tests_pass = true;
  
  // template <ac_dwt2_function DWT_FN_VAL, class IN_TYPE, class OUT_TYPE, int W_MAX, int H_MAX, int N_LEVELS, int ACC_W, int ACC_I, bool USE_SP>
  all_tests_pass =
    test_driver<AC_HAAR, ac_ipl::RGB_imd<ac_int<8, false> >, ac_ipl::RGB_imd<ac_fixed<24, 12, true> >, 1024, 1024, 4, 24, 12, true>("in_image_o_w.bmp", "out_image_dwt2_pyr_HAAR_RGB_SP_o_w.bmp")
    && all_tests_pass;
  all_tests_pass =
    test_driver<AC_DB2, ac_int<8, false>, ac_fixed<24, 12, true>, 1024, 1024, 4, 24, 12, true> ("in_image.bmp", "out_image_dwt2_pyr_DB2_GS_SP_e_w.bmp")
    && all_tests_pass;
  all_tests_pass =
    test_driver<AC_DB2, ac_int<8, false>, ac_fixed<24, 12, true>, 1024, 1024, 4, 24, 12, false>("in_image.bmp", "out_image_dwt2_pyr_DB2_GS_DP_e_w.bmp")
    && all_tests_pass;
  all_tests_pass =
    test_driver<AC_DB2, ac_ipl::RGB_imd<ac_int<8, false> >, ac_ipl::RGB_imd<ac_fixed<24, 12, true> >, 1024, 1024, 4, 24, 12, true>("in_image.bmp", "out_image_dwt2_pyr_DB2_RGB_SP_e_w.bmp")
    && all_tests_pass;
  all_tests_pass =
    test_driver<AC_DB2, ac_ipl::RGB_imd<ac_int<8, false> >, ac_ipl::RGB_imd<ac_fixed<24, 12, true> >, 1024, 1024, 4, 24, 12, false>("in_image.bmp", "out_image_dwt2_pyr_DB2_RGB_DP_e_w.bmp")
    && all_tests_pass;
  all_tests_pass =
    test_driver<AC_DB2, ac_int<8, false>, ac_fixed<24, 12, true>, 1024, 1024, 4, 24, 12, true> ("in_image_o_w.bmp", "out_image_dwt2_pyr_DB2_GS_SP_o_w.bmp")
    && all_tests_pass;
  all_tests_pass =
    test_driver<AC_DB2, ac_int<8, false>, ac_fixed<24, 12, true>, 1024, 1024, 4, 24, 12, false>("in_image_o_w.bmp", "out_image_dwt2_pyr_DB2_GS_DP_o_w.bmp")
    && all_tests_pass;
  all_tests_pass =
    test_driver<AC_DB2, ac_ipl::RGB_imd<ac_int<8, false> >, ac_ipl::RGB_imd<ac_fixed<24, 12, true> >, 1024, 1024, 4, 24, 12, true>("in_image_o_w.bmp", "out_image_dwt2_pyr_DB2_RGB_SP_o_w.bmp")
    && all_tests_pass;
  all_tests_pass =
    test_driver<AC_DB2, ac_ipl::RGB_imd<ac_int<8, false> >, ac_ipl::RGB_imd<ac_fixed<24, 12, true> >, 1024, 1024, 4, 24, 12, false>("in_image_o_w.bmp", "out_image_dwt2_pyr_DB2_RGB_DP_o_w.bmp")
    && all_tests_pass;
  
  cout << "  Testbench finished." << endl;
  
  // Notify the user whether or not the test was a failure.
  if (!all_tests_pass) {
    cout << "  ac_dwt2_pyr - FAILED" << endl;
    return -1;
  }
  
  return 0;
}
