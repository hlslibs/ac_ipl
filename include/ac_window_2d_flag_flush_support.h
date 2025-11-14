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
// File: ac_window_2d_flag_flush_support.h
//
// Description:
//  Version of ac_window_2d_flag with flushing support added.
//
// Notes:
//  Unlike the ac_window_2d_flag design, this design has a non-continuous consumption of inputs and
//  production of outputs, so as to account for the lag produced by ramping up the linebuffers.
//
// Revision History:
//    3.3.0  - Added CDesignChecker waivers for CNS and CCC violations.
//
//*********************************************************************************************************

#ifndef __AC_WINDOW_2D_FLAG_FLUSH_SUPPORT_H
#define __AC_WINDOW_2D_FLAG_FLUSH_SUPPORT_H

#include <ac_int.h>
#ifndef __SYNTHESIS__
#include <cassert>
#endif

#include "ac_buffer_2d.h"
#include "ac_window_1d_flag.h"

#ifndef __SYNTHESIS__
#include <iostream>
using namespace std;
#endif

template<class T, int AC_WN_ROW, int AC_WN_COL, int AC_NCOL, int AC_WMODE>
class ac_window_2d_flag_flush_support
{
public:
  ac_window_2d_flag_flush_support();
  ac_window_2d_flag_flush_support(T bval);
  void reset();
  T &operator()      (int r,int c);
  const T &operator()(int r, int c) const ;
  void write(T src, bool sof, bool eof, bool sol, bool eol);
  void exec(T src, bool sof, bool eof, bool sol, bool eol, bool w);

  bool valid();
  void readFlags(bool &sof, bool &eof, bool &sol, bool &eol);
  void rewind();
  // Following two functions can be used externally to determine how to loop through the input and output images while using singleport memories.
  bool isExtraWriteAllowed(); // Return allow_extra_write value, lets us know if we have to write an extra value to linebuffer in right extension region.
  bool spExtraItDynamic(); // Tells us whether or not we need an extra iteration for flushing due to the usage of singleport memories and odd dynamic image widths.

  enum { AC_EVEN_ROW = ((AC_WN_ROW%2)==0) };
  enum { AC_EVEN_COL = ((AC_WN_COL%2)==0) };
  // If the row/column size is even and the windowing mode is set to AC_MIRROR, we have
  // to increase the size of the mirrors by one.
  enum { PLUS_1_MIRROR_ROW = int((AC_EVEN_ROW) && (bool(AC_WMODE&AC_MIRROR))) };
  enum { PLUS_1_MIRROR_COL = int((AC_EVEN_COL) && (bool(AC_WMODE&AC_MIRROR))) };
  enum { logAC_NCOL = ac::nbits<AC_NCOL - 1>::val };

  enum { flushItRow = bool(AC_WMODE&AC_WIN) ? 0 : AC_WN_ROW/2 + AC_EVEN_ROW + PLUS_1_MIRROR_ROW }; // Extra flushing iterations required for row loop.
  enum {
    spOddCol = bool(AC_WMODE & AC_SINGLEPORT) && AC_NCOL%2 != 0, // Do we have odd column values for singleport implementations?
    spExtraIt = AC_WN_COL <= 2 && spOddCol, // Do we need an extra flushing iteration for the column loop? (based solely on the AC_NCOL template value)
    extraIt = bool(PLUS_1_MIRROR_COL) || spExtraIt, // We may also need an extra iteration for the column loop to account for the increased rampup period for certain mirroring implementations.
    flushItCol = bool(AC_WMODE & AC_WIN) ? spOddCol : AC_WN_COL/2 - AC_EVEN_COL + int(extraIt), // Extra flushing iterations required for column loop.
  };

private:
  ac_buffer_2d<T,AC_NCOL,AC_WN_ROW + PLUS_1_MIRROR_ROW,AC_WMODE> vWind; //Vertical window

  T   data_[AC_WN_ROW + PLUS_1_MIRROR_ROW][AC_WN_COL + PLUS_1_MIRROR_COL]; // This array stores the input samples
  T   woutH_[AC_WN_ROW + PLUS_1_MIRROR_ROW][AC_WN_COL + PLUS_1_MIRROR_COL]; // This array is what really gets read
  bool sol_[AC_WN_COL + PLUS_1_MIRROR_COL];  // Start of line control
  bool eol_[AC_WN_COL + PLUS_1_MIRROR_COL];  // End of line control
  bool s_; // start bit set when start detected in shift reg
  bool e_; // end bit set when start detected in shift reg

  T   wout_[AC_WN_ROW + PLUS_1_MIRROR_ROW];             // This array stores the vWind output
  ac_int<logAC_NCOL,false> addr;  // Address counter for line buffers
  bool sofOut_[AC_WN_COL + PLUS_1_MIRROR_COL];  // Start of frame output control
  bool eofOut_[AC_WN_COL + PLUS_1_MIRROR_COL];  // End of frame output control
  bool sof_[AC_WN_ROW + PLUS_1_MIRROR_ROW];  // Start of frame control
  bool eof_[AC_WN_ROW + PLUS_1_MIRROR_ROW];  // End of frame control
  bool solOut;  // Start of line output control
  bool eolOut;  // End of line output control

  bool allow_extra_write;
  bool ru_cc;
  bool rampup_;             // Has the window ramped up?
  bool rampup_v;            // Has the window ramped up in the vertical direction?
  bool rampup_h;            // Has the window ramped up in the horizontal direction?
  bool eofOutSeenPrevPix;   // EOF seen on previous output pixel. Used to set rampup_ to false.
  bool s;                   // start bit set when start detected in shift reg
  bool e;                   // end bit set when start detected in shift reg
  int m;

  T boundaryVal;   //constant for boundary condition
};


template<class T, int AC_WN_ROW, int AC_WN_COL, int AC_NCOL, int AC_WMODE> ac_window_2d_flag_flush_support<T, AC_WN_ROW, AC_WN_COL,AC_NCOL,AC_WMODE>::ac_window_2d_flag_flush_support():
  addr(0), allow_extra_write(false), ru_cc(false), rampup_(false), rampup_v(false), rampup_h(false), eofOutSeenPrevPix(false), s(false), e(false), m(0)
{

  #ifdef __SYNTHESIS__
  #pragma hls_unroll yes
  #endif
  #ifndef __SYNTHESIS__

  assert((AC_WN_ROW > 0) && "ac_window must have positive window width");
  assert((AC_WN_COL > 0) && "ac_window must have positive window height");
  assert((AC_WN_COL < AC_NCOL) && "ac_window width must be smaller than array width");
  #endif
  for (int i=0; i<AC_WN_ROW + PLUS_1_MIRROR_ROW; i++) {
    wout_[i] = 0;
    sof_[i] = 0;
    eof_[i] = 0;
  }
  #ifdef __SYNTHESIS__
  #pragma hls_unroll yes
  #endif
  for (int i=0; i<AC_WN_COL + PLUS_1_MIRROR_COL; i++) {
    sol_[i] = 0;
    eol_[i] = 0;
    sofOut_[i] = 0;
    eofOut_[i] = 0;
  }

  boundaryVal = 0; // Set to zero if parameterized constructor is not used.
}

template<class T, int AC_WN_ROW, int AC_WN_COL, int AC_NCOL, int AC_WMODE> ac_window_2d_flag_flush_support<T, AC_WN_ROW, AC_WN_COL,AC_NCOL,AC_WMODE>::ac_window_2d_flag_flush_support(T bval):
  addr(0), allow_extra_write(false), ru_cc(false), rampup_(false), rampup_v(false), rampup_h(false), eofOutSeenPrevPix(false), s(false), e(false), m(0)
{
  #ifdef __SYNTHESIS__
  #pragma hls_unroll yes
  #endif
  #ifndef __SYNTHESIS__
  assert((AC_WN_ROW > 0) && "ac_window must have positive window width");
  assert((AC_WN_COL > 0) && "ac_window must have positive window height");
  assert((AC_WN_COL < AC_NCOL) && "ac_window width must be smaller than array width");
  #endif
  for (int i=0; i<AC_WN_ROW + PLUS_1_MIRROR_ROW; i++) {
    wout_[i] = 0;
    sofOut_[i] = 0;
    eofOut_[i] = 0;
    sof_[i] = 0;
    eof_[i] = 0;
  }
  #ifdef __SYNTHESIS__
  #pragma hls_unroll yes
  #endif
  for (int i=0; i<AC_WN_COL + PLUS_1_MIRROR_COL; i++) {
    sol_[i] = 0;
    eol_[i] = 0;
  }
  boundaryVal = bval;
}

// Reset member variables in order to start with new configurations with different
// frame sizes.
template<class T, int AC_WN_ROW, int AC_WN_COL, int AC_NCOL, int AC_WMODE>
void ac_window_2d_flag_flush_support<T, AC_WN_ROW, AC_WN_COL,AC_NCOL,AC_WMODE>::reset()
{
  addr = 0;
  allow_extra_write = false;
  ru_cc = false;
  rampup_ = false;
  rampup_v = false;
  rampup_h = false;
  eofOutSeenPrevPix = false;
  s = false;
  e = false;
  m = 0;

  #ifdef __SYNTHESIS__
  #pragma hls_unroll yes
  #endif
  for (int i=0; i<AC_WN_ROW + PLUS_1_MIRROR_ROW; i++) {
    wout_[i] = 0;
    sofOut_[i] = 0;
    eofOut_[i] = 0;
    sof_[i] = 0;
    eof_[i] = 0;
  }

  #ifdef __SYNTHESIS__
  #pragma hls_unroll yes
  #endif
  for (int i=0; i<AC_WN_COL + PLUS_1_MIRROR_COL; i++) {
    sol_[i] = 0;
    eol_[i] = 0;
  }
}

// Have we passed the first AC_WN_ROW/2+1 rampup cycles? Is this now valid data?
template<class T, int AC_WN_ROW, int AC_WN_COL, int AC_NCOL, int AC_WMODE>
bool ac_window_2d_flag_flush_support<T, AC_WN_ROW, AC_WN_COL,AC_NCOL,AC_WMODE>::valid()
{
  return rampup_;
}

template<class T, int AC_WN_ROW, int AC_WN_COL, int AC_NCOL, int AC_WMODE>
void ac_window_2d_flag_flush_support<T,AC_WN_ROW,AC_WN_COL,AC_NCOL,AC_WMODE>::write(T src, bool sof, bool eof, bool sol, bool eol)
{
  if (sol) {                     //Store sof at start of each new line
    #ifdef __SYNTHESIS__
    #pragma hls_unroll yes
    #endif
    for (int i=0; i<AC_WN_ROW - 1 + PLUS_1_MIRROR_ROW; i++)
    { sof_[i] = sof_[i+1]; }
    sof_[AC_WN_ROW - 1 + PLUS_1_MIRROR_ROW] = sof;
  }

  #ifdef __SYNTHESIS__
  #pragma hls_unroll yes
  #endif
  for (int i=0; i<AC_WN_COL - 1 + PLUS_1_MIRROR_COL; i++)
  { sofOut_[i] = sofOut_[i+1]; }

  #pragma hls_waive CNS
  if (AC_WMODE&AC_WIN)
  { sofOut_[AC_WN_COL - 1 + PLUS_1_MIRROR_COL] = sol ? sof_[0]:(bool)0; }
  else
  { sofOut_[AC_WN_COL - 1 + PLUS_1_MIRROR_COL] = sol ? sof_[AC_WN_ROW/2]:(bool)0; }

  if (sol) {
    addr = 0; //Address resets at each start of line
  }

  bool isRER = false; // Is the window in the right extension region ("RER")?

  #pragma hls_waive CNS
  if (!bool(AC_WMODE&AC_WIN)) {
    #ifdef __SYNTHESIS__
    #pragma hls_unroll yes
    #endif
    for (int i = AC_WN_COL - 1 + PLUS_1_MIRROR_COL; i >= (AC_WN_COL/2) + 1; i--) {
      #pragma hls_waive CCC CNS
      if (i == AC_WN_COL - 1 + PLUS_1_MIRROR_COL || i == AC_WN_COL - 2 + PLUS_1_MIRROR_COL) { // CCC and CNS violations waived
        isRER |= eol_[i] && !allow_extra_write;
      } else {
        isRER |= eol_[i];
      }
    }
  }

  // If linebuffer address is even when the receive eol pixel is received, and if the windowing mode is singleport, we know
  // that the number of image columns is odd and that we will have to allow an extra write in the right extension region,
  // so as to complete the writing of all pixel values to the SPRAM.
  if (bool(AC_WMODE&AC_SINGLEPORT) && eol && (addr%2 == 0)) {
    allow_extra_write = true;
  }

  // For the right extension region, no valid inputs are present, and we must hence disable writing to the linebuffers.
  // This is done in order to avoid overwriting valid pixel values stored in memory with dummy values.
  bool buffWrite = !isRER;

  vWind.write(src, (int)(addr), buffWrite);

  if (addr != AC_NCOL-1) // Prevents out-of-bounds addressing. We use != operator instead of < operator to reduce area.
  { addr++; }
  #ifdef __SYNTHESIS__
  #pragma hls_unroll yes
  #endif
  for (int i=0; i<AC_WN_ROW + PLUS_1_MIRROR_ROW; i++) {   //get vertical window output
    wout_[i] = vWind[i];
  }

  #pragma hls_waive CNS
  if (AC_WMODE&AC_CLIP) {
    s = false;
    #ifdef __SYNTHESIS__
    #pragma hls_unroll yes // vertical lower clipping
    #endif
    for (int i=AC_WN_ROW/2-1; i>=0; i--) {
      s |= sof_[i+1];
      wout_[i] = s ? wout_[i+1] : wout_[i];
    }

    e = false;
    #ifdef __SYNTHESIS__
    #pragma hls_unroll yes //  vertical upper clipping
    #endif
    for (int i=AC_WN_ROW/2+1; i< AC_WN_ROW; i++) {
      e |= eof_[i];
      wout_[i] = e ? wout_[i-1] : wout_[i];
    }
  }

  #pragma hls_waive CNS
  if (AC_WMODE&AC_MIRROR) {
    s = false;
    #ifdef __SYNTHESIS__
    #pragma hls_unroll yes //  vertical lower clipping
    #endif
    for (int i=AC_WN_ROW/2-1; i>=0; i--) {
      s |= sof_[i+1];
      if (sof_[i+1]==1)
      { m = i+1; }
      wout_[i] = s ? wout_[m*2-i] : wout_[i];
    }

    e = false;
    #ifdef __SYNTHESIS__
    #pragma hls_unroll yes //  vertical upper clipping
    #endif
    for (int i=AC_WN_ROW/2+1; i< AC_WN_ROW + PLUS_1_MIRROR_ROW; i++) {
      //!!!!!!!compare againt i instead of i-1 to go handle eol
      e |= eof_[i];
      if (eof_[i]==1)
      { m = i-1; }
      wout_[i] = e ? wout_[m*2-i] : wout_[i];
    }
  }

  #pragma hls_waive CNS
  if (AC_WMODE&AC_BOUNDARY) {
    s = false;
    #ifdef __SYNTHESIS__
    #pragma hls_unroll yes // vertical lower clipping
    #endif
    for (int i=AC_WN_ROW/2-1; i>=0; i--) {
      s |= sof_[i+1];
      wout_[i] = s ? boundaryVal : wout_[i];
    }

    e = false;
    #ifdef __SYNTHESIS__
    #pragma hls_unroll yes //  vertical upper clipping
    #endif
    for (int i=AC_WN_ROW/2+1; i< AC_WN_ROW; i++) {
      e |= eof_[i];
      wout_[i] = e ? boundaryVal : wout_[i];
    }
  }

  if (eol) {                     //Store eof at end of each new line
    #ifdef __SYNTHESIS__
    #pragma hls_unroll yes
    #endif
    for (int i=0; i<AC_WN_ROW - 1 + PLUS_1_MIRROR_ROW; i++)
    { eof_[i] = eof_[i+1]; }
    eof_[AC_WN_ROW - 1 + PLUS_1_MIRROR_ROW] = eof;
  }
  #ifdef __SYNTHESIS__
  #pragma hls_unroll yes
  #endif
  for (int i=0; i<AC_WN_COL - 1 + PLUS_1_MIRROR_COL; i++)
  { eofOut_[i] = eofOut_[i+1]; }
  #pragma hls_waive CNS
  if (bool(AC_WMODE&AC_WIN)) {
    eofOut_[AC_WN_COL - 1] = eol ? eof_[AC_WN_ROW - 1]:false;
  } else {
    eofOut_[AC_WN_COL - 1 + PLUS_1_MIRROR_COL] = eol ? eof_[AC_WN_ROW/2]:false;
  }

  #ifdef __SYNTHESIS__
  #pragma hls_unroll yes
  #endif
  for (int i=0; i<AC_WN_COL - 1 + PLUS_1_MIRROR_COL; i++)
  { sol_[i] = sol_[i+1]; }
  sol_[AC_WN_COL - 1 + PLUS_1_MIRROR_COL] = sol;

  // Store eol value from previous call to write() function.
  bool eol_zero_p = eol_[AC_WN_COL - 1];

  #ifdef __SYNTHESIS__
  #pragma hls_unroll yes
  #endif
  for (int i=0; i<AC_WN_COL - 1 + PLUS_1_MIRROR_COL; i++)
  { eol_[i] = eol_[i+1]; }
  eol_[AC_WN_COL - 1 + PLUS_1_MIRROR_COL] = eol;

  #ifdef __SYNTHESIS__
  #pragma hls_unroll yes
  #endif
  for (int j=0; j<AC_WN_ROW + PLUS_1_MIRROR_ROW; j++) {
    //HORIZONTAL WINDOWS
    #ifdef __SYNTHESIS__
    #pragma hls_unroll yes
    #endif
    for (int i=0; i<AC_WN_COL - 1 + PLUS_1_MIRROR_COL; i++) {
      #pragma hls_waive UMR
      data_[j][i] = data_[j][i+1];
    }
    data_[j][AC_WN_COL - 1 + PLUS_1_MIRROR_COL] = wout_[j];

    #ifdef __SYNTHESIS__
    #pragma hls_unroll yes
    #endif
    for (int i=0; i<AC_WN_COL + PLUS_1_MIRROR_COL; i++)
    { woutH_[j][i] = data_[j][i]; }

    #pragma hls_waive CNS
    if (AC_WMODE&AC_CLIP) {
      s_ = false;
      #ifdef __SYNTHESIS__
      #pragma hls_unroll yes //lower clipping
      #endif
      for (int i=AC_WN_COL/2-1; i>=0; i--) {
        s_ |= sol_[i+1];
        woutH_[j][i] = s_ ? woutH_[j][i+1] : woutH_[j][i];
      }

      e_ = false;
      #ifdef __SYNTHESIS__
      #pragma hls_unroll yes //upper clipping
      #endif
      for (int i=AC_WN_COL/2+1; i< AC_WN_COL; i++) {
        e_ |= eol_[i-1];
        woutH_[j][i] = e_ ? woutH_[j][i-1] : woutH_[j][i];
      }
    }

    #pragma hls_waive CNS
    if (AC_WMODE&AC_MIRROR) {
      s_ = false;
      int m_;
      #ifdef __SYNTHESIS__
      #pragma hls_unroll yes //lower clipping
      #endif
      for (int i=AC_WN_COL/2 - 1; i>=0; i--) {
        s_ |= sol_[i+1];
        if (sol_[i+1]==1)
        { m_ = i+1; }
        woutH_[j][i] = s_ ? woutH_[j][m_*2 - i] : woutH_[j][i];
      }

      e_ = false;
      #ifdef __SYNTHESIS__
      #pragma hls_unroll yes //upper clipping
      #endif
      for (int i=AC_WN_COL/2+1; i< AC_WN_COL + PLUS_1_MIRROR_COL; i++) {
        e_ |= eol_[i-1];
        if (eol_[i-1]==1)
        { m_ = i-1; }
        woutH_[j][i] = e_ ? woutH_[j][m_*2-i] : woutH_[j][i];
      }
    }

    #pragma hls_waive CNS
    if (AC_WMODE&AC_BOUNDARY) {
      s_ = false;
      #ifdef __SYNTHESIS__
      #pragma hls_unroll yes //lower clipping
      #endif
      for (int i=AC_WN_COL/2-1; i>=0; i--) {
        s_ |= sol_[i+1];
        woutH_[j][i] = s_ ? boundaryVal : woutH_[j][i];
      }

      e_ = false;
      #ifdef __SYNTHESIS__
      #pragma hls_unroll yes //upper clipping
      #endif
      for (int i=AC_WN_COL/2+1; i< AC_WN_COL; i++) {
        e_ |= eol_[i-1];
        woutH_[j][i] = e_ ? boundaryVal : woutH_[j][i];
      }
    }
  }

  #pragma hls_waive CNS
  if (AC_WMODE&AC_WIN) { //No clipping or mirroring means that flags are ready after prefill.
    solOut = sol_[0];
    eolOut = eol_[AC_WN_COL - 1]; // The output ends as soon as the input image ends.
  } else {
    solOut = sol_[AC_WN_COL/2];
    eolOut = eol_[AC_WN_COL/2];
  }

  bool ru_cc;

  #pragma hls_waive CNS
  if (AC_WMODE&AC_WIN) {
    ru_cc = allow_extra_write;
  } else {
    // If the image column size is odd and if we're using singleport memories, we expect an additional cycle at the end of every image line
    // to complete writing image pixels. This can be a problem for windows that are one or two pixels wide, as far as setting rampup_h to
    // false is concerned. Set an enum variable to denote that we must anticipate this corner case.
    ru_cc = allow_extra_write && (AC_WN_COL <= 2); // ru_cc : "r"amp"u"p "c"orner "c"ase.
  }

  // The window needs AC_WN_COL/2 pixels to prefill horizontally. The start of this prefill interval coincides with the
  // receipt of the input sol pixel, while the end coincides with the production of the output sol pixel.
  // Update rampup_h accordingly.
  if (solOut) {
    rampup_h = true;
  } else if (sol || (ru_cc && eol_zero_p)) {
    rampup_h = false;
  }

  // eofOutSeenPrevPix: The previous pixel was the eof pixel for the ac_window output. Hence, the ac_window has no more valid outputs to
  // produce and rampup_ is set to false, in preparation for the next frame that is to be processed.
  if (eofOutSeenPrevPix) {
    rampup_v = false;
  }

  // Update eofOutSeenPrevPix after the above condition is evaluated. This is done so that rampup_ isn't set to false just when the eof
  // pixel is encountered, but is instead set to false on the pixel after that.
  #pragma hls_waive CNS
  if (AC_WMODE&AC_WIN) {
    eofOutSeenPrevPix = eofOut_[0]; // No boundary condition handling.
  } else {
    eofOutSeenPrevPix = eofOut_[AC_WN_COL/2];
  }

  // Keeping track of whether the window has prefilled vertically and update the rampup_v flag by looking at the relevant
  // sof_ position.
  #pragma hls_waive CNS
  if (AC_WMODE&AC_WIN) {
    if (sof_[0]) {
      rampup_v = true;
    }
  } else if (sof_[AC_WN_ROW/2]) {
    rampup_v = true;
  }

  // The rampup_ signal (and, ultimately, the valid() output) is only set to true once the window has adequately prefilled in the
  // horizontal and vertical directions.
  rampup_ = rampup_h && rampup_v;
}

template<class T, int AC_WN_ROW, int AC_WN_COL, int AC_NCOL, int AC_WMODE>
inline  T &ac_window_2d_flag_flush_support<T,AC_WN_ROW,AC_WN_COL,AC_NCOL,AC_WMODE>::operator() (int r, int c)
{
  #ifndef __SYNTHESIS__
  if (!(AC_WMODE&AC_LIN_INDEX)) {
    assert((-AC_WN_ROW/2 <= r) && (r <= AC_WN_ROW/2));
    assert((-AC_WN_COL/2 <= c) && (c <= AC_WN_COL/2));
  } else {
    assert((0 <= r) && (r <= AC_WN_ROW));
    assert((0 <= c) && (c <= AC_WN_COL));
  }
  #endif

  #pragma hls_waive CNS
  if (!(AC_WMODE&AC_LIN_INDEX))
  { return woutH_[r+AC_WN_ROW/2][c+AC_WN_COL/2]; }
  else
  { return woutH_[r][c]; }
}
template<class T, int AC_WN_ROW, int AC_WN_COL, int AC_NCOL, int AC_WMODE>
inline  const T &ac_window_2d_flag_flush_support<T,AC_WN_ROW,AC_WN_COL,AC_NCOL,AC_WMODE>::operator()(int r,int c) const
{
  #ifndef __SYNTHESIS__
  if (!(AC_WMODE&AC_LIN_INDEX)) {
    assert((-AC_WN_ROW/2 <= r) && (r <= AC_WN_ROW/2));
    assert((-AC_WN_COL/2 <= c) && (c <= AC_WN_COL/2));
  } else {
    assert((0 <= r) && (r <= AC_WN_ROW));
    assert((0 <= c) && (c <= AC_WN_COL));
  }
  #endif
  if (!(AC_WMODE&AC_LIN_INDEX))
  { return woutH_[r+AC_WN_ROW/2][c+AC_WN_COL/2]; }
  else
  { return woutH_[r][c]; }
}

template<class T, int AC_WN_ROW, int AC_WN_COL, int AC_NCOL, int AC_WMODE>
void ac_window_2d_flag_flush_support<T,AC_WN_ROW,AC_WN_COL,AC_NCOL,AC_WMODE>::readFlags(bool &sof, bool &eof, bool &sol, bool &eol)
{
  #pragma hls_waive CNS
  if (bool(AC_WMODE&AC_WIN)) {
    sof = sofOut_[0];
    eof = eofOut_[0];
  } else {
    sof = sofOut_[AC_WN_COL/2];
    eof = eofOut_[AC_WN_COL/2];
  }
  sol = solOut & rampup_;
  eol = eolOut & rampup_;
}

//Rewind support added for video scalers, etc.
template<class T, int AC_WN_ROW, int AC_WN_COL, int AC_NCOL, int AC_WMODE>
void ac_window_2d_flag_flush_support<T,AC_WN_ROW,AC_WN_COL,AC_NCOL,AC_WMODE>::exec(T src, bool sof, bool eof, bool sol, bool eol, bool w)
{
  if (sol&w) {                     //Store sof at start of each new line when writing
    #ifdef __SYNTHESIS__
    #pragma hls_unroll yes
    #endif
    for (int i=0; i<AC_WN_ROW - 1 + PLUS_1_MIRROR_ROW; i++)
    { sof_[i] = sof_[i+1]; }
    sof_[AC_WN_ROW - 1 + PLUS_1_MIRROR_ROW] = sof;
  }

  #pragma hls_waive CCC
  #ifdef __SYNTHESIS__
  #pragma hls_unroll yes
  #endif
  for (int i=0; i<AC_WN_COL - 1 + PLUS_1_MIRROR_COL; i++)
  { sofOut_[i] = sofOut_[i+1]; }

  #pragma hls_waive CNS
  if (AC_WMODE&AC_WIN)
  { sofOut_[AC_WN_COL - 1 + PLUS_1_MIRROR_COL] = sol ? sof_[0]:(bool)0; }
  else
  { sofOut_[AC_WN_COL - 1 + PLUS_1_MIRROR_COL] = sol ? sof_[AC_WN_ROW/2]:(bool)0; }

  if (sol) {
    addr = 0; //Address resets at each start of line
  }

  // If linebuffer address is even when the receive eol pixel is received, and if the windowing mode is singleport, we know
  // that the number of image columns is odd and that we will have to allow an extra write in the right extension region,
  // so as to complete the writing of all pixel values to the SPRAM.
  #pragma hls_waive CNS
  if (bool(AC_WMODE&AC_SINGLEPORT) && eol && (addr%2 == 0)) {
    allow_extra_write = true;
  }

  // For the exec() function, it is assumed that the user has already guarded the linebuffer against writes in the right extension region
  // by modifying the "w" flag appropriately. Hence, we do not include any logic to detect whether we are in the right extension region or not.
  vWind.write(src, (int)(addr), w);

  if (addr != AC_NCOL-1) // Prevents out-of-bounds addressing. We use != operator instead of < operator to reduce area.
  { addr++; }
  #ifdef __SYNTHESIS__
  #pragma hls_unroll yes
  #endif
  for (int i=0; i<AC_WN_ROW + PLUS_1_MIRROR_ROW; i++) {   //get vertical window output
    wout_[i] = vWind[i];
  }

  #pragma hls_waive CNS
  if (AC_WMODE&AC_CLIP) {
    s = false;
    #ifdef __SYNTHESIS__
    #pragma hls_unroll yes // vertical lower clipping
    #endif
    for (int i=AC_WN_ROW/2-1; i>=0; i--) {
      s |= sof_[i+1];
      wout_[i] = s ? wout_[i+1] : wout_[i];
    }

    e = false;
    #ifdef __SYNTHESIS__
    #pragma hls_unroll yes //  vertical upper clipping
    #endif
    for (int i=AC_WN_ROW/2+1; i< AC_WN_ROW; i++) {
      e |= eof_[i];
      wout_[i] = e ? wout_[i-1] : wout_[i];
    }
  }

  #pragma hls_waive CNS
  if (AC_WMODE&AC_MIRROR) {
    s = false;
    #ifdef __SYNTHESIS__
    #pragma hls_unroll yes //  vertical lower clipping
    #endif
    for (int i=AC_WN_ROW/2-1; i>=0; i--) {
      s |= sof_[i+1];
      if (sof_[i+1]==1)
      { m = i+1; }
      wout_[i] = s ? wout_[m*2-i] : wout_[i];
    }

    e = false;
    #ifdef __SYNTHESIS__
    #pragma hls_unroll yes //  vertical upper clipping
    #endif
    for (int i=AC_WN_ROW/2+1; i< AC_WN_ROW + PLUS_1_MIRROR_ROW; i++) {
      //!!!!!!!compare againt i instead of i-1 to go handle eol
      e |= eof_[i];
      if (eof_[i]==1)
      { m = i-1; }
      wout_[i] = e ? wout_[m*2-i] : wout_[i];
    }
  }

  #pragma hls_waive CNS
  if (AC_WMODE&AC_BOUNDARY) {
    s = false;
    #ifdef __SYNTHESIS__
    #pragma hls_unroll yes // vertical lower clipping
    #endif
    for (int i=AC_WN_ROW/2-1; i>=0; i--) {
      s |= sof_[i+1];
      wout_[i] = s ? boundaryVal : wout_[i];
    }

    e = false;
    #ifdef __SYNTHESIS__
    #pragma hls_unroll yes //  vertical upper clipping
    #endif
    for (int i=AC_WN_ROW/2+1; i< AC_WN_ROW; i++) {
      e |= eof_[i];
      wout_[i] = e ? boundaryVal : wout_[i];
    }
  }

  if (eol&w) {                     //Store eof at end of each new line when writing
    #ifdef __SYNTHESIS__
    #pragma hls_unroll yes
    #endif
    for (int i=0; i<AC_WN_ROW - 1 + PLUS_1_MIRROR_ROW; i++)
    { eof_[i] = eof_[i+1]; }
    eof_[AC_WN_ROW - 1 + PLUS_1_MIRROR_ROW] = eof;
  }

  #pragma hls_waive CCC
  #ifdef __SYNTHESIS__
  #pragma hls_unroll yes
  #endif
  for (int i=0; i<AC_WN_COL - 1 + PLUS_1_MIRROR_COL; i++)
  { eofOut_[i] = eofOut_[i+1]; }
  #pragma hls_waive CNS
  if (bool(AC_WMODE&AC_WIN)) {
    eofOut_[AC_WN_COL - 1] = eol ? eof_[AC_WN_ROW - 1]:false;
  } else {
    eofOut_[AC_WN_COL - 1 + PLUS_1_MIRROR_COL] = eol ? eof_[AC_WN_ROW/2]:false;
  }

  #pragma hls_waive CCC
  #ifdef __SYNTHESIS__
  #pragma hls_unroll yes
  #endif
  for (int i=0; i<AC_WN_COL - 1 + PLUS_1_MIRROR_COL; i++)
  { sol_[i] = sol_[i+1]; }
  sol_[AC_WN_COL - 1 + PLUS_1_MIRROR_COL] = sol;

  // Store eol value from previous call to write() function.
  bool eol_zero_p = eol_[AC_WN_COL - 1];

  #pragma hls_waive CCC
  #ifdef __SYNTHESIS__
  #pragma hls_unroll yes
  #endif
  for (int i=0; i<AC_WN_COL - 1 + PLUS_1_MIRROR_COL; i++)
  { eol_[i] = eol_[i+1]; }
  eol_[AC_WN_COL - 1 + PLUS_1_MIRROR_COL] = eol;

  #ifdef __SYNTHESIS__
  #pragma hls_unroll yes
  #endif
  for (int j=0; j<AC_WN_ROW + PLUS_1_MIRROR_ROW; j++) {
    //HORIZONTAL WINDOWS
    #pragma hls_waive CCC
    #ifdef __SYNTHESIS__
    #pragma hls_unroll yes
    #endif
    for (int i=0; i<AC_WN_COL - 1 + PLUS_1_MIRROR_COL; i++)
    { data_[j][i] = data_[j][i+1]; }
    data_[j][AC_WN_COL - 1 + PLUS_1_MIRROR_COL] = wout_[j];

    #ifdef __SYNTHESIS__
    #pragma hls_unroll yes
    #endif
    for (int i=0; i<AC_WN_COL + PLUS_1_MIRROR_COL; i++)
    { woutH_[j][i] = data_[j][i]; }

    #pragma hls_waive CNS
    if (AC_WMODE&AC_CLIP) {
      s_ = false;
      #pragma hls_waive CCC
      #ifdef __SYNTHESIS__
      #pragma hls_unroll yes //lower clipping
      #endif
      for (int i=AC_WN_COL/2-1; i>=0; i--) {
        s_ |= sol_[i+1];
        woutH_[j][i] = s_ ? woutH_[j][i+1] : woutH_[j][i];
      }

      e_ = false;
      #pragma hls_waive CCC
      #ifdef __SYNTHESIS__
      #pragma hls_unroll yes //upper clipping
      #endif
      for (int i=AC_WN_COL/2+1; i< AC_WN_COL; i++) {
        e_ |= eol_[i-1];
        woutH_[j][i] = e_ ? woutH_[j][i-1] : woutH_[j][i];
      }
    }

    #pragma hls_waive CNS
    if (AC_WMODE&AC_MIRROR) {
      s_ = false;
      int m_;
      #ifdef __SYNTHESIS__
      #pragma hls_unroll yes //lower clipping
      #endif
      for (int i=AC_WN_COL/2 - 1; i>=0; i--) {
        s_ |= sol_[i+1];
        if (sol_[i+1]==1)
        { m_ = i+1; }
        woutH_[j][i] = s_ ? woutH_[j][m_*2 - i] : woutH_[j][i];
      }

      e_ = false;
      #ifdef __SYNTHESIS__
      #pragma hls_unroll yes //upper clipping
      #endif
      for (int i=AC_WN_COL/2+1; i< AC_WN_COL + PLUS_1_MIRROR_COL; i++) {
        e_ |= eol_[i-1];
        if (eol_[i-1]==1)
        { m_ = i-1; }
        woutH_[j][i] = e_ ? woutH_[j][m_*2-i] : woutH_[j][i];
      }
    }

    #pragma hls_waive CNS
    if (AC_WMODE&AC_BOUNDARY) {
      s_ = false;
      #ifdef __SYNTHESIS__
      #pragma hls_unroll yes //lower clipping
      #endif
      for (int i=AC_WN_COL/2-1; i>=0; i--) {
        s_ |= sol_[i+1];
        woutH_[j][i] = s_ ? boundaryVal : woutH_[j][i];
      }

      e_ = false;
      #ifdef __SYNTHESIS__
      #pragma hls_unroll yes //upper clipping
      #endif
      for (int i=AC_WN_COL/2+1; i< AC_WN_COL; i++) {
        e_ |= eol_[i-1];
        woutH_[j][i] = e_ ? boundaryVal : woutH_[j][i];
      }
    }
  }

  #pragma hls_waive CNS
  if (AC_WMODE&AC_WIN) {
    //No clipping or mirroring means that flags are ready after prefill
    solOut = sol_[0];
    eolOut = eol_[AC_WN_COL - 1]; // The output ends as soon as the input image ends.
  } else {
    solOut = sol_[AC_WN_COL/2];
    eolOut = eol_[AC_WN_COL/2];
  }

  bool ru_cc;

  #pragma hls_waive CNS
  if (AC_WMODE&AC_WIN) {
    ru_cc = allow_extra_write;
  } else {
    // If the image column size is odd and if we're using singleport memories, we expect an additional cycle at the end of every image line
    // to complete writing image pixels. This can be a problem for windows that are one or two pixels wide, as far as setting rampup_h to
    // false is concerned. Set an enum variable to denote that we must anticipate this corner case.
    ru_cc = allow_extra_write && (AC_WN_COL <= 2); // ru_cc : "r"amp"u"p "c"orner "c"ase.
  }

  // The window needs AC_WN_COL/2 pixels to prefill horizontally. The start of this prefill interval coincides with the
  // receipt of the input sol pixel, while the end coincides with the production of the output sol pixel.
  // Update rampup_h accordingly.
  if (solOut) {
    rampup_h = true;
  } else if (sol || (ru_cc && eol_zero_p)) {
    rampup_h = false;
  }

  // eofOutSeenPrevPix: The previous pixel was the eof pixel for the ac_window output. Hence, the ac_window has no more valid outputs to
  // produce and rampup_ is set to false, in preparation for the next frame that is to be processed.
  if (eofOutSeenPrevPix) {
    rampup_v = false;
  }

  // Update eofOutSeenPrevPix after the above condition is evaluated. This is done so that rampup_ isn't set to false just when the eof
  // pixel is encountered, but is instead set to false on the pixel after that.
  #pragma hls_waive CNS
  if (AC_WMODE&AC_WIN) {
    eofOutSeenPrevPix = eofOut_[0]; // No boundary condition handling.
  } else {
    eofOutSeenPrevPix = eofOut_[AC_WN_COL/2];
  }

  // Keeping track of whether the window has prefilled vertically and update the rampup_v flag by looking at the relevant
  // sof_ position.
  #pragma hls_waive CNS
  if (AC_WMODE&AC_WIN) {
    if (sof_[0]) {
      rampup_v = true;
    }
  } else if (sof_[AC_WN_ROW/2]) {
    rampup_v = true;
  }

  // The rampup_ signal (and, ultimately, the valid() output) is only set to true once the window has adequately prefilled in the
  // horizontal and vertical directions.
  rampup_ = rampup_h && rampup_v;
}

//Rewind is used to reset the address into the line buffers.  This is used when performing scaling and
//the programmable line width is less than the maximum line width.
template<class T, int AC_WN_ROW, int AC_WN_COL, int AC_NCOL, int AC_WMODE>
void ac_window_2d_flag_flush_support<T,AC_WN_ROW,AC_WN_COL,AC_NCOL,AC_WMODE>::rewind()
{
  addr = 0;
}

template<class T, int AC_WN_ROW, int AC_WN_COL, int AC_NCOL, int AC_WMODE>
bool ac_window_2d_flag_flush_support<T,AC_WN_ROW,AC_WN_COL,AC_NCOL,AC_WMODE>::isExtraWriteAllowed()
{
  return allow_extra_write;
}

template<class T, int AC_WN_ROW, int AC_WN_COL, int AC_NCOL, int AC_WMODE>
bool ac_window_2d_flag_flush_support<T,AC_WN_ROW,AC_WN_COL,AC_NCOL,AC_WMODE>::spExtraItDynamic()
{
  // An extra flushing iteration is always required for odd image widths with singleport memories when we are using AC_WIN.
  bool noBound = bool(AC_WMODE&AC_WIN);
  return allow_extra_write && (noBound || AC_WN_COL <= 2);
}

#endif
