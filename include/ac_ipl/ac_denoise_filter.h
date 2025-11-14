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
//***************************************************************************
// File: ac_denoise_filter.h
//***************************************************************************

#ifndef _INCLUDED_AC_DENOISE_FILTER_H_
#define _INCLUDED_AC_DENOISE_FILTER_H_

#include <ac_int.h>
#include <ac_fixed.h>
#include <ac_window_2d_flag.h>
#include <ac_channel.h>
#include <mc_scverify.h>

template <unsigned CDEPTH, unsigned W_MAX, unsigned H_MAX, bool USE_SINGLEPORT = false>
class ac_denoise_filter
{
public:
  // Define IO types.
  typedef ac_int<CDEPTH, false> pixInType;
  typedef ac_int<CDEPTH, false> pixOutType;
  typedef ac_int<ac::nbits<W_MAX>::val, false> widthInType;
  typedef ac_int<ac::nbits<H_MAX>::val, false> heightInType;

  #pragma hls_pipeline_init_interval 1
  #pragma hls_design interface
  void CCS_BLOCK(run) (
    ac_channel<pixInType>  &streamIn,   // Pixel input stream
    ac_channel<pixOutType> &streamOut,  // Pixel output stream
    const widthInType      widthIn,     // Input width
    const heightInType     heightIn    // Input height
  ) {
    const unsigned WN_SZ = 3;

    enum {
      FILT_WMODE = USE_SINGLEPORT ? AC_MIRROR | AC_SINGLEPORT : AC_MIRROR,
      OTHER_WMODE = USE_SINGLEPORT ? AC_BOUNDARY | AC_SINGLEPORT : AC_BOUNDARY,
    };

    // Declare window object to store input pixel values for gaussian filtering.
    ac_window_2d_flag<pixInType, WN_SZ, WN_SZ, W_MAX, FILT_WMODE> acWindObj;

    ac_int<ac::nbits<H_MAX>::val, false> i = 0;
    ac_int<ac::nbits<W_MAX>::val, false> j = 0;

    bool inRead = true, eofOut = false;

    #pragma hls_pipeline_init_interval 1
    GAUSS_PROC_LOOP: do {
      pixInType pixIn = inRead ? streamIn.read() : pixInType(0);
      // Calculate sof, eof, sol and eol based on image coordinates.
      bool sol = (j == 0);
      bool sof = (i == 0) && sol;
      bool eol = (j == widthIn - 1);
      bool eof = (i == heightIn - 1) && eol;
      // Write input frame value to the window object.
      acWindObj.write(pixIn, sof, eof, sol, eol);
      if (eof) {
        inRead = false; // Stop reading input channel after valid image region.
      }
      j++; // Update column counter after every pixel received.
      if (j == widthIn) {
        j = 0; // Max. possible j value = widthIn - 1.
        i++; // Update row counter after every line received.
        if (i == heightIn) {
          i = 0; // Max. possible i value = heightIn - 1.
        }
      }

      bool sofOut, solOut, eolOut;
      acWindObj.readFlags(sofOut, eofOut, solOut, eolOut);
      if (acWindObj.valid()) {
        // If the window has ramped up, call medianFilt() to produce a filtered output.
        pixOutType medianOp = medianFilt<pixInType> (acWindObj);
        streamOut.write(medianOp);
      }
    } while (!eofOut); // Stop processing once the entire image output has been read.
  }

  ac_denoise_filter() { }

// Carry out filtering with kernel and window values.
  template<class pixInType, int K_SZ, int IN_WMODE>
  pixOutType medianFilt(
    const ac_window_2d_flag<pixInType, K_SZ, K_SZ, W_MAX, IN_WMODE> &acWindObj
  ) {
    pixInType filtOp = 0.0;
    const int WN_EL = 9;
    // Window output is stored in temporary array. While you can also directly multiply values from
    // acWindObj and get the same QofR, having a temporary array can be more convenient for debugging.
    pixInType acWindOut[WN_EL];
    #pragma hls_unroll yes
    CONV_OP_ROW_LOOP: for (int r = 0; r < int(K_SZ); r++) {
      #pragma hls_unroll yes
      CONV_OP_COL_LOOP: for (int c = 0; c < int(K_SZ); c++) {
        acWindOut[((r*K_SZ) + c)] = acWindObj(r - (K_SZ/2), c - (K_SZ/2));
      }
    }
    insertionSort(acWindOut, WN_EL);
    filtOp = acWindOut[((K_SZ-1)/2)];
    return filtOp;
  }

// Function to sort an array using insertion sort
  template<class pixInType>
  void insertionSort(pixInType arr[], int n) {    //const acWindType (&arr)[K_SZ][K_SZ]
    int i, key, j;
    for (i = 1; i < n; i++) {
      key = arr[i];
      j = i - 1;

      /* Move elements of arr[0..i-1], that are
      greater than key, to one position ahead
      of their current position */
      while (j >= 0 && arr[j] > key) {
        arr[j + 1] = arr[j];
        j = j - 1;
      }
      arr[j + 1] = key;
    }
  }

};

#endif // #ifndef _INCLUDED_AC_DENOISE_FILTER_H
