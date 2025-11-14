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
#ifndef __AC_WINDOW_1D_STREAM_H
#define __AC_WINDOW_1D_STREAM_H

#ifndef __SYNTHESIS__
#include <cassert>
#endif

#include "ac_window_1d_flag.h"
template<class T, int AC_WN, int AC_NCOL, ac_window_mode AC_WMODE = AC_WIN>
class ac_window_1d_stream
{
public:
  ac_window_1d_stream();
  int operator++ ();
  int operator++ (int) { // non-canonical form (s.b. const window)
    return this->operator++();
  }
  T &operator[]      (int i);
  const T &operator[](int i) const ;
  void write(T *s, int n);
  bool valid();
private:
  enum {
    logAC_NCOL = (AC_NCOL < 2 ? 1 : AC_NCOL < 4 ? 2 : AC_NCOL < 8 ? 3 : AC_NCOL < 16 ? 4 : AC_NCOL < 32 ? 5 :
                  AC_NCOL < 64 ? 6 : AC_NCOL < 128 ? 7 : AC_NCOL < 256 ? 8 : AC_NCOL < 512 ? 9 : AC_NCOL < 1024 ? 10 :
                  AC_NCOL < 2048 ? 11 : AC_NCOL < 4096 ? 12 : AC_NCOL < 8192 ? 13 : AC_NCOL < 16384 ? 14 :
                  AC_NCOL < 32768 ? 15 : AC_NCOL < 65536 ? 16 : 32)
  };
  ac_window_1d_flag<T,AC_WN,AC_WMODE> w;   //window flaf class
  ac_int<logAC_NCOL+1> cnt_;                               // Counter, state variable used to determine if:
  //  1/ window has ramped up and data is valid
  //  2/ what and when and how to mux value from data_ to wout_

  ac_int<logAC_NCOL,false>  sz_prev_;      // size of previous input array
  ac_int<logAC_NCOL,false>  sz_;                   // size of current input array
  T  *src_;       // the array for which this is a window
  bool sol_;
  bool eol_;
};

// The only defined CTOR
template<class T, int AC_WN, int AC_NCOL, ac_window_mode AC_WMODE> ac_window_1d_stream<T, AC_WN, AC_NCOL, AC_WMODE>::ac_window_1d_stream()
{
  cnt_= 0;
  sz_=0;
  sz_prev_=0;
}

// "Connect" the input data to the window.
// Can't be done in the ctor anymore, as the object must be made static...
template<class T, int AC_WN, int AC_NCOL, ac_window_mode AC_WMODE>
void ac_window_1d_stream<T, AC_WN, AC_NCOL,AC_WMODE>::write(T *s, int n)
{
  src_=s;
  sz_=n;
};

// Have we passed the first AC_WN/2+1 rampup cycles? Is this now valid data?
template<class T, int AC_WN, int AC_NCOL, ac_window_mode AC_WMODE>
bool ac_window_1d_stream<T, AC_WN, AC_NCOL, AC_WMODE>::valid()
{
  return w.valid();
};

template<class T, int AC_WN, int AC_NCOL, ac_window_mode AC_WMODE>
int ac_window_1d_stream<T, AC_WN, AC_NCOL, AC_WMODE>::operator++ ()
{

  if (cnt_==0)
  { sz_prev_=sz_; }

  if (cnt_== 0)
  { sol_ = 1; }
  else
  { sol_ = 0; }

  if (cnt_ == sz_prev_ - 1)
  { eol_ = 1; }
  else
  { eol_ = 0; }

  w.write(*src_++,sol_,eol_);

  if (cnt_<(sz_prev_-1)) { cnt_++; }
  else { cnt_=0; }

  return cnt_;
}

template<class T, int AC_WN, int AC_NCOL, ac_window_mode AC_WMODE>
inline  T &ac_window_1d_stream<T, AC_WN, AC_NCOL, AC_WMODE>::operator[] (int i)
{
  return w[i];
}
template<class T, int AC_WN, int AC_NCOL, ac_window_mode AC_WMODE>
inline  const T &ac_window_1d_stream<T, AC_WN, AC_NCOL, AC_WMODE>::operator[](int i) const
{
  return w[i];
}

#endif
