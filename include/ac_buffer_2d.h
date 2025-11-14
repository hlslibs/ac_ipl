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
////////////////////////////////////////////////////////////////////////////////
//
// 05-26-09 - Mike Fingeroff - added singleport RAM support
//
////////////////////////////////////////////////////////////////////////////////
#ifndef __AC_BUFFER_2D_H
#define __AC_BUFFER_2D_H
#include <ac_int.h>
#include <ac_fixed.h>
#include <ac_window_1d_flag.h>
#ifndef AC_WINDOW_CUSTOM
#include <ac_window_structs.h>
#endif

#ifndef __SYNTHESIS__
#include <cassert>
#include <stdio.h>
#endif

template<typename DTYPE, int AC_WMODE, int AC_NCOL, int AC_NROW>
class ac_linebuf
{
  enum {
    IS_SINGLEPORT = bool(AC_WMODE&AC_SINGLEPORT),
    AC_NCOL2 = (AC_NCOL/(IS_SINGLEPORT ? 2 : 1)) + int(AC_NCOL%2 != 0 && IS_SINGLEPORT),
  };

  ac_linebuf< DTYPE, AC_WMODE,AC_NCOL,AC_NROW-1> buf;
  ac_int<ac::nbits<AC_NCOL - 1>::val, false> addr_int;
  ac_int<1,false> cnt;
  typedef ac_width2x<DTYPE, IS_SINGLEPORT> ac_width_2x_type;
  typedef typename ac_width_2x_type::data linebuf_type;
  linebuf_type data[AC_NCOL2];
  linebuf_type tmp_out;
  linebuf_type tmp_in;
  DTYPE tmp;

public:
  ac_linebuf() : cnt(0) {
    linebuf_type dummy;
    for (int i=0; i<AC_NCOL2; i++) {
      #pragma hls_waive UMR
      data[i] =  dummy;
    }
  }
  
  void print() {
    #ifndef __SYNTHESIS__
    for (int i=0; i<AC_NCOL2; i++)
    { printf("%d  ",data[i].to_int()); }
    printf("\n");
    #endif
  }
  
  void access(DTYPE din[AC_NROW], DTYPE dout[AC_NROW], int addr, bool w[AC_NROW]) {
    addr_int = addr;
    #pragma hls_waive CNS
    if (IS_SINGLEPORT) {
      // Force cnt to reset once you reach a new line.
      if (addr == 0) { cnt = 0; }
      if (w[AC_NROW-1]) {
        if (cnt==1)
        { ac_width_2x_type::set_half(1,din[AC_NROW-1],tmp_in); }
        else
        { ac_width_2x_type::set_half(0,din[AC_NROW-1],tmp_in); }
      }

      if ((cnt&1)==0) { //read on even
        #pragma hls_waive UMR
        tmp_out = data[addr_int>>1];
      } else { //write on odd
        if (w[AC_NROW-1]) {
          data[addr_int>>1] = tmp_in;
        }
      }

      bool sel = (cnt==1)?1:0;
      ac_width_2x_type::get_half(sel,tmp_out,dout[AC_NROW-1]);
      cnt++;
    } else {
      #pragma hls_waive UMR
      dout[AC_NROW-1] = data[addr_int];
      if (w[AC_NROW-1]) {
        data[addr_int] = din[AC_NROW-1];
      }
    }
    buf.access(din,dout,addr,w);
  }

};

template<typename DTYPE, int AC_WMODE, int AC_NCOL>
class ac_linebuf<DTYPE, AC_WMODE,AC_NCOL,1>
{
  enum {
    IS_SINGLEPORT = bool(AC_WMODE&AC_SINGLEPORT),
    // Add an extra element to the linebuffer array if your column size is odd and if you're using singleport memories.
    AC_NCOL2 = (AC_NCOL/(IS_SINGLEPORT ? 2 : 1)) + int(AC_NCOL%2 != 0 && IS_SINGLEPORT),
  };

  ac_int<ac::nbits<AC_NCOL - 1>::val, false> addr_int;
  ac_int<1,false> cnt;
  typedef ac_width2x<DTYPE, IS_SINGLEPORT> ac_width_2x_type;
  typedef typename ac_width_2x_type::data linebuf_type;
  linebuf_type data[AC_NCOL2];
  linebuf_type tmp_out;
  linebuf_type tmp_in;
  DTYPE tmp;

public:
  ac_linebuf() : cnt(0) {
    linebuf_type dummy;
    for (int i=0; i<AC_NCOL2; i++) {
      #pragma hls_waive UMR
      data[i] = dummy;
    }
  }
  
  void print() {
    #ifndef __SYNTHESIS__
    for (int i=0; i<AC_NCOL2; i++)
    { printf("%d  ",data[i].to_int()); }
    printf("\n");
    #endif
  }
  
  void access(DTYPE din[1], DTYPE dout[1], int addr, bool w[1]) {
    addr_int = addr;
    #pragma hls_waive CNS
    if (IS_SINGLEPORT) {
      // Force cnt to reset once you reach a new line.
      if (addr == 0) { cnt = 0; }
      if (w[0]) {
        if (cnt==1)
        { ac_width_2x_type::set_half(1,din[0],tmp_in); }
        else
        { ac_width_2x_type::set_half(0,din[0],tmp_in); }
      }
      if ((cnt&1)==0) { //read on even
        #pragma hls_waive UMR
        tmp_out = data[addr_int>>1];
      } else { //write on odd
        if (w[0]) {
          data[addr_int>>1] = tmp_in;
        }
      }

      bool sel = (cnt==1)?1:0;
      ac_width_2x_type::get_half(sel,tmp_out,dout[0]);
      cnt++;
    } else {
      #pragma hls_waive UMR
      dout[0] = data[addr_int];
      if (w[0]) {
        data[addr_int] = din[0];
      }
    }
  }
};

template<typename DTYPE, int AC_NCOL, int AC_NROW, int AC_WMODE=AC_DUALPORT>
class ac_buffer_2d
{

  enum {
    logAC_NROW = ac::log2_ceil< AC_NROW >::val
  };
  enum {AC_REWIND_VAL = (AC_REWIND&AC_WMODE)?1:0 };
  int cptr;                     //points to current col written
  bool dummy[AC_NROW-1];
  DTYPE wout_[AC_NROW];             // This array is what really gets read
  ac_linebuf< DTYPE, AC_WMODE,AC_NCOL,AC_NROW-1+AC_REWIND_VAL> data; //recursive line buffer class
  ac_int<logAC_NROW,false> sel;
  ac_int<logAC_NROW+1,true> sel1;
  DTYPE b[AC_NROW];
  DTYPE t_tmp[AC_NROW-1+AC_REWIND_VAL];
  DTYPE t[AC_NROW];
  bool s[AC_NROW];
  DTYPE data_tmp[AC_NROW];
public:
  ac_buffer_2d() : cptr(0), sel(AC_NROW-2+AC_REWIND_VAL), sel1(0) {
    #ifdef __SYNTHESIS__
#pragma hls_unroll yes
    #endif
    for (int i=0; i<AC_NROW; i++) {
      wout_[i] = DTYPE(0);
      b[i] = DTYPE(0);
      t[i] = DTYPE(0);
      s[i] = 0;
    }
  }
  DTYPE &operator[]      (int i);
  const DTYPE &operator[](int i) const ;
  void set_dummy(int idx, bool val) { dummy[idx] = val; }
  void set_cptr(int idx) {cptr = idx; }
  void set_wout(int idx, DTYPE val) { wout_[idx] = val; }
  void write(DTYPE src, int i, bool w);

  #ifndef __SYNTHESIS__
  void print() {
    DTYPE w[AC_NROW-1+AC_REWIND_VAL];
    bool ss[AC_NROW-1+AC_REWIND_VAL];
    for (int i=0; i<AC_NROW-1+AC_REWIND_VAL; i++)
    { ss[i] = 0; }

    for (int i=0; i<AC_NROW-1+AC_REWIND_VAL; i++) {
      for (int j=0; j<AC_NCOL; j++) {
        data.access(w,w,j,ss);
        printf("%d  ", w[i].to_int());
      }
      printf("\n");
    }
    printf("sel = %d  sel1 = %d, cptr = %d\n   DATA %d \n", sel.to_int(), sel1.to_int(), cptr, data_tmp[0].to_int());
  }
  #endif

  DTYPE get_wout(int idx) const { return wout_[idx]; }
};

template<typename DTYPE, int AC_NCOL, int AC_NROW, int AC_WMODE>
inline  DTYPE &ac_buffer_2d<DTYPE, AC_NCOL,AC_NROW,AC_WMODE>::operator[] (int i)
{
  #ifndef __SYNTHESIS__
  assert((i>=0) && (i<AC_NROW));
  #endif
  return wout_[AC_NROW-1-i];
}

template<typename DTYPE, int AC_NCOL, int AC_NROW, int AC_WMODE>
inline  const DTYPE &ac_buffer_2d<DTYPE, AC_NCOL,AC_NROW,AC_WMODE>::operator[](int i) const
{
  #ifndef __SYNTHESIS__
  assert((i>=0) && (i<AC_NROW));
  #endif
  return wout_[AC_NROW-1-i];
}


template<typename DTYPE, int AC_NCOL, int AC_NROW, int AC_WMODE>
void ac_buffer_2d<DTYPE,AC_NCOL,AC_NROW,AC_WMODE>::write(DTYPE src,int i, bool w)
{
  #ifndef __SYNTHESIS__
  assert((i>=0) && (i < AC_NCOL));
  #endif
  ac_buffer_2d<DTYPE,AC_NCOL,AC_NROW,AC_WMODE>::set_cptr(i);

  if ((i==0) & w) {
    sel += 1;
  }
  if (sel==AC_NROW-1+AC_REWIND_VAL)
  { sel = 0; }
  #ifdef __SYNTHESIS__
#pragma hls_unroll yes
  #endif
  for (int j=0; j<AC_NROW-1+AC_REWIND_VAL; j++)
  { s[j] = (sel==j) ? w:0; }
  #ifdef __SYNTHESIS__
#pragma hls_unroll yes
  #endif
  for (int j=0; j<AC_NROW-1+AC_REWIND_VAL; j++)
  { data_tmp[j] = src; }

  data.access(data_tmp,t_tmp,i,s);

  #ifdef __SYNTHESIS__
#pragma hls_unroll yes
  #endif
  for (int j=0; j<AC_NROW-1+AC_REWIND_VAL; j++)
  { t[j] = t_tmp[j]; }

  #pragma hls_waive CNS
  if (!AC_REWIND_VAL) {
    #ifdef __SYNTHESIS__
#pragma hls_unroll yes
    #endif
    for (int j=0; j<AC_NROW-1; j++) {
      sel1 = sel-j;
      if (sel1<=0)
      { sel1 = sel1 + AC_NROW-1; }
      b[sel1] = t_tmp[j];
    }
    #pragma hls_waive CNS
    b[0] =  w ? src : t[AC_NROW-1];
  } else {
    #ifdef __SYNTHESIS__
#pragma hls_unroll yes
    #endif
    for (int j=0; j<AC_NROW; j++) {
      if (sel==j)
      { t[j] = w ? src : t[j]; }
    }

    #ifdef __SYNTHESIS__
#pragma hls_unroll yes
    #endif
    for (int j=0; j<AC_NROW; j++) {
      sel1 = sel-j;
      if (sel1<0)
      { sel1 = sel1 + AC_NROW; }
      b[sel1] = t[j];
    }
  }

  #ifdef __SYNTHESIS__
#pragma hls_unroll yes
  #endif
  for (int j=0; j<AC_NROW; j++)
  { ac_buffer_2d<DTYPE,AC_NCOL,AC_NROW,AC_WMODE>::set_wout(j,b[j]); }
}

#endif
