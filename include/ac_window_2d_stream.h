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
// 05-26-09 - Mike Fingeroff - added singleport RAM support
//////////////////////////////////////////////////////////////////////////////////

#ifndef __AC_WINDOW_2D_STREAM_H
#define __AC_WINDOW_2D_STREAM_H

#ifndef __SYNTHESIS__
#include <cassert>
#endif

#ifndef ac_compile_time_assert
#ifdef BOOST_STATIC_ASSERT
#define ac_compile_time_assert(cond, msg) \
      BOOST_STATIC_ASSERT(cond) ;
#else
#define ac_compile_time_assert(cond, msg) \
      typedef char msg[(cond) ? 1 : -1]
#endif
#endif


#include "ac_window_2d_flag.h"
template<class T, int AC_WN_ROW, int AC_WN_COL, int AC_NROW, int AC_NCOL, int AC_WMODE = AC_WIN|AC_DUALPORT>
class ac_window_2d_stream
{
public:
  ac_window_2d_stream();
  int operator++ ();
  int operator++ (int) { // non-canonical form (s.b. const window)
    return this->operator++();
  }
  T &operator()      (int r,int c);
  const T &operator()(int r, int c) const ;
  void write(T *s, int row_sz, int col_sz);
  bool valid();
private:
  enum {
    logAC_NROW = (AC_NROW < 2 ? 1 : AC_NROW < 4 ? 2 : AC_NROW < 8 ? 3 : AC_NROW < 16 ? 4 : AC_NROW < 32 ? 5 :
                  AC_NROW < 64 ? 6 : AC_NROW < 128 ? 7 : AC_NROW < 256 ? 8 : AC_NROW < 512 ? 9 : AC_NROW < 1024 ? 10 :
                  AC_NROW < 2048 ? 11 : AC_NROW < 4096 ? 12 : AC_NROW < 8192 ? 13 : AC_NROW < 16384 ? 14 :
                  AC_NROW < 32768 ? 15 : AC_NROW < 65536 ? 16 : 32)
  };
  enum {
    logAC_NCOL = (AC_NCOL < 2 ? 1 : AC_NCOL < 4 ? 2 : AC_NCOL < 8 ? 3 : AC_NCOL < 16 ? 4 : AC_NCOL < 32 ? 5 :
                  AC_NCOL < 64 ? 6 : AC_NCOL < 128 ? 7 : AC_NCOL < 256 ? 8 : AC_NCOL < 512 ? 9 : AC_NCOL < 1024 ? 10 :
                  AC_NCOL < 2048 ? 11 : AC_NCOL < 4096 ? 12 : AC_NCOL < 8192 ? 13 : AC_NCOL < 16384 ? 14 :
                  AC_NCOL < 32768 ? 15 : AC_NCOL < 65536 ? 16 : 32)
  };
  ac_window_2d_flag<T,AC_WN_ROW,AC_WN_COL,AC_NCOL,AC_WMODE> w;   //window flag class
  ac_int<logAC_NROW+1, false> row_cnt_;
  ac_int<logAC_NCOL+1, false> col_cnt_;                              // Counter, state variable used to determine if:
  //  1/ window has ramped up and data is valid
  //  2/ what and when and how to mux value from data_ to wout_
  ac_int<logAC_NROW,false>  row_sz_prev_;      // size of previous input array
  ac_int<logAC_NROW,false>  row_sz_;                   // size of current input array
  ac_int<logAC_NCOL,false>  col_sz_prev_;      // size of previous input array
  ac_int<logAC_NCOL,false>  col_sz_;                   // size of current input array
  T  *src_;       // the array for which this is a window
  bool sof_;
  bool eof_;
  bool sol_;
  bool eol_;
};

// The only defined CTOR
template<class T, int AC_WN_ROW, int AC_WN_COL, int AC_NROW, int AC_NCOL, int AC_WMODE> ac_window_2d_stream<T, AC_WN_ROW, AC_WN_COL, AC_NROW, AC_NCOL, AC_WMODE>::ac_window_2d_stream()
{
  row_cnt_= 0;
  col_cnt_= 0;
  row_sz_=0;
  row_sz_prev_=0;
  col_sz_=0;
  col_sz_prev_=0;

  ac_compile_time_assert((AC_WN_ROW > 0), ac_window_must_have_positive_window_width ) ;
  ac_compile_time_assert((AC_WN_COL > 0), ac_window_must_have_positive_window_height ) ;
  ac_compile_time_assert((AC_WN_ROW < AC_NROW), ac_window_width_must_be_smaller_than_array_width ) ;
  ac_compile_time_assert((AC_WN_COL < AC_NCOL), ac_window_height_must_be_smaller_than_array_height ) ;

  #ifndef __SYNTHESIS__
  assert((AC_WN_ROW > 0) && "ac_window must have positive window width");
  assert((AC_WN_COL > 0) && "ac_window must have positive window height");
  assert((AC_WN_ROW < AC_NROW) && "ac_window width must be smaller than array width");
  assert((AC_WN_COL < AC_NCOL) && "ac_window height must be smaller than array height");
  #endif
}

// "Connect" the input data to the window.
// Can't be done in the ctor anymore, as the object must be made static...
template<class T, int AC_WN_ROW, int AC_WN_COL, int AC_NROW, int AC_NCOL, int AC_WMODE>
void ac_window_2d_stream<T, AC_WN_ROW, AC_WN_COL, AC_NROW, AC_NCOL,AC_WMODE>::write(T *s, int row_sz, int col_sz)
{
  src_=s;
  row_sz_=row_sz;
  col_sz_=col_sz;
};

// Have we passed the first WN/2+1 rampup cycles? Is this now valid data?
template<class T, int AC_WN_ROW, int AC_WN_COL, int AC_NROW, int AC_NCOL, int AC_WMODE>
bool ac_window_2d_stream<T, AC_WN_ROW, AC_WN_COL, AC_NROW, AC_NCOL, AC_WMODE>::valid()
{
  return w.valid();
};

template<class T, int AC_WN_ROW, int AC_WN_COL, int AC_NROW, int AC_NCOL, int AC_WMODE>
int ac_window_2d_stream<T, AC_WN_ROW, AC_WN_COL, AC_NROW, AC_NCOL, AC_WMODE>::operator++ ()
{

  if (col_cnt_==0)
  { col_sz_prev_= col_sz_; }

  if (row_cnt_ == 0 & col_cnt_ == 0) {
    row_sz_prev_= row_sz_;
    sof_ = 1;
  } else
  { sof_ = 0; }

  if (col_cnt_== 0)
  { sol_ = 1; }
  else
  { sol_ = 0; }

  if (col_cnt_ == col_sz_prev_ - 1)
  { eol_ = 1; }
  else
  { eol_ = 0; }

  if (row_cnt_ == row_sz_prev_-1 & col_cnt_== col_sz_prev_ - 1)
  { eof_ = 1; }
  else
  { eof_ = 0; }

  w.write(*src_++,sof_,eof_,sol_,eol_);

  if (col_cnt_!=(col_sz_prev_-1))
  { col_cnt_++; }
  else {
    col_cnt_=0;
    if (row_cnt_!=(row_sz_prev_-1)) { row_cnt_++; }
    else { row_cnt_=0; }
  }
  return (int)(row_cnt_ * row_sz_prev_ + col_cnt_);
}

template<class T, int AC_WN_ROW, int AC_WN_COL, int AC_NROW, int AC_NCOL, int AC_WMODE>
inline  T &ac_window_2d_stream<T, AC_WN_ROW, AC_WN_COL, AC_NROW, AC_NCOL, AC_WMODE>::operator() (int r, int c)
{
  return w(r,c);
}
template<class T, int AC_WN_ROW, int AC_WN_COL, int AC_NROW, int AC_NCOL, int AC_WMODE>
inline  const T &ac_window_2d_stream<T, AC_WN_ROW, AC_WN_COL, AC_NROW, AC_NCOL, AC_WMODE>::operator()(int r, int c) const
{
  return w(r,c);
}

#endif
