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
//******************************************************************************************
// Decription: ac_filter2d.h
//  file includes the 2d filter architecture for dynamic Kernel and Image Size user have to provide required
//  configuration and data via ac_channels generated RTL would be containing ready and valid for both the ports
//  output port will be provided with vld bit along with vld of output port user can treat the vld bit as actual
//  data valid.
//
//  Output Structure
//  struct{
//     data;
//     vld;
//  };
// Usage:
// Notes:
//
// Revision History:
//    3.4.3  - dgb - Updated compiler checks to work with MS VS 2019
//
//******************************************************************************************

#ifndef _AC_FILTER_2D_H_
#define _AC_FILTER_2D_H_

#include <ac_int.h>
#include <ac_fixed.h>
#include <ac_channel.h>
#include <ac_ipl/ac_linebuffer.h>

// template < Input-datatype, Maximum Width of Image,Maximum Height of Image, Kernel Width , Padding Size>
template <class pType, int WIDTH, int HEIGHT, int KSIZE, int PADDING>
struct filter2dOut {
  pType data;
  bool vld;
};

// template < Input-datatype, Maximum Width of Image,Maximum Height of Image, Kernel Width , Padding Size>
template <class pType, int WIDTH, int HEIGHT, int KSIZE, int PADDING>
struct filter2dconfig {
  ac_int < ac::nbits < WIDTH + 2 * PADDING >::val, 0 > width;
  ac_int < ac::nbits < HEIGHT + 2 * PADDING >::val, 0 > height;
};
#include <mc_scverify.h>

// The design uses static_asserts, which are only supported by C++11 or later compiler standards.
// The #error directive below informs the user if they're not using those standards.
#if (defined(__GNUC__) && (__cplusplus < 201103L))
#error Please use C++11 or a later standard for compilation.
#endif
#if (defined(_MSC_VER) && (_MSC_VER < 1920) && !defined(__EDG__))
#error Please use Microsoft VS 2019 or a later standard for compilation.
#endif

//PIPELINE LENGTH provides the feedback length of accumulation
//for example PE produced an output will be added to corrospnding
//next value after 5-Cycles if PIPELENGTH is 5
enum {
  PIPELENGTH = 5
};

//============================================================================================================
// Class: lineBuffer
// Description:
// This class implements a line buffer. line buffer is based on Dual/saperate read write port Memory
//============================================================================================================

template <class elemType, class cfgType, int WIDTH, int KSIZE>
class lineBuffer {
public:
  // Constructor
  lineBuffer() {
  }

  // Run() provides the main interface to linebuffer Block
  // must mapped to sequential CCORE
  #pragma hls_design interface ccore
  #pragma hls_pipeline_init_interval 1
  void CCS_BLOCK(run)(elemType &d_in, const bool rst_in, const bool weightEn, elemType d_out[KSIZE], cfgType width) {
    
    if (rst_in == true | weightEn == true) {
      cnt = 0;
      w_cnt = width;

      #pragma hls_unroll yes
      for (int i = 0; i < KSIZE; i++) {
        d_out[i] = 0;
      }
    }
    else {
      INPUT_TYPE packed_input;
      packed_input[0] = d_in;  // Pack the input element
      
      OUTPUT_TYPE lb_output;
      // Call run function of linebuffer class from ac_linebuffer
      lb_inst.run(true, packed_input, cnt, lb_output);
      
      // Output logic with vertical ramp-up control
      d_out[0] = d_in;
      
      #pragma hls_unroll yes
      for (unsigned char i = 0; i < KSIZE - 1; i++) {

        d_out[i+1] = (global_row_cnt > i) ? lb_output[KSIZE - 2 - i][0] : elemType(0);

      }
      
      cnt++;
      if (cnt == w_cnt) {

        cnt = 0;
        global_row_cnt++;

      }
    }
  }

private:
  typedef ac_packed_vector<elemType, 1> INPUT_TYPE;
  typedef ac_array<INPUT_TYPE, KSIZE> OUTPUT_TYPE;
  ac_linebuffer<INPUT_TYPE, OUTPUT_TYPE, WIDTH, KSIZE, AC_DUAL> lb_inst;
  ac_int < ac::log2_ceil<WIDTH>::val + 1, false > cnt;
  ac_int < ac::log2_ceil<WIDTH>::val + 1, false > global_row_cnt;
  cfgType w_cnt;
};

//============================================================================================================
// Class: filterPE
// Description:
// This class implements a Processing Element for the PE array.
// Processing Element comprise of mac Units of kernel size and Output shift Registers to store the output
//============================================================================================================

template <class elemType, int KSIZE, int BSize, int RSize, int YBits>
class filterPE {
  typedef typename elemType:: template rt_T<elemType>::mult T;
  typedef typename T::rt_unary:: template set < KSIZE + 1 >::sum resultType;
  typedef typename resultType::rt_unary:: template set<2>::sum tmpType;

public:
  filterPE() {
    outregWrite = 0;
    outregRead = 0;
    canRead = false;
    #pragma hls_unroll yes
    for (int i = 0; i < BSize; i++) {
      PixelWindow[i] = 0;
      WeightsRegs[i] = 0;
    }
  }
  // Run method provide the interface to sequential CCore
  #pragma hls_design interface ccore
  #pragma hls_pipeline_init_interval 1
  void CCS_BLOCK(run)(elemType &dataIn,
                      elemType &psumIn,
                      elemType &weightIn,
                      ac_int<YBits, false> idY, // "Y" position of element in array
                      bool &weightEn,           // when true, shift weights into WeightsRegs
                      elemType &dataOut,
                      elemType &weightOut,
                      elemType &psumOut
                     ) {
    // pass thru feature data
    dataOut = PixelWindow[0];
    weightOut = WeightsRegs[0];
    // Shift feature data in
    #pragma hls_unroll yes
    PIXWINDOW: for (int i = 0; i < KSIZE - 1; i++) {
      PixelWindow[i] = PixelWindow[i + 1];
    }
    PixelWindow[KSIZE - 1] = dataIn;
    tmpType result = psumIn;
    // pass thru weights through if enabled
    if (weightEn) {
      macResult = 0;
      #pragma hls_unroll yes
      WEIGHTWDW: for (int i = 0; i < KSIZE - 1; i++) {
        WeightsRegs[i] = WeightsRegs[i + 1];
      }
      WeightsRegs[KSIZE - 1] = weightIn;
    }
    else {
      macResult = PixelWindow[0] * WeightsRegs[0];
      #pragma hls_unroll yes
      MAC: for (int i = 1; i < KSIZE; i++) {
        macResult += PixelWindow[i] * WeightsRegs[i];
      }
    }
    // Circular buffer for the output data sync across the PEs
    OutputShiftReg[outregWrite] = macResult;
    outregWrite++;
    if (idY == 0) {
      result += macResult;
    }
    else {
      if (canRead) {
        result += OutputShiftReg[outregRead];
        outregRead++;
      }
      else {
        outregRead = 0;
      }
    }
    if (outregWrite == (idY * (PIPELENGTH + 1))) {
      canRead = true;
    }
    // Compute partial sum output (taps output circular buffer)
    psumOut = result;
  }

private:
public:
  elemType PixelWindow[BSize];     // feature data shifts through window
  elemType WeightsRegs[BSize];     // weights loaded during config
  elemType OutputShiftReg[RSize];  // output shift register
  resultType macResult;

  ac_int<YBits, false> outregRead;
  ac_int<YBits, false> outregWrite;
  bool canRead;
};

//============================================================================================================
// Class: vldComp
// Description:
//        This class implements the valid signals for the PE outputs and it also implements the striding feature.
//        Striding reduces the feature map size by the striding factor * striding factor by processing the
//        alternate pixel in a row and the row itself. For filter design Striding is kept as fixed to 1
//============================================================================================================

template <class typeConfig, int KSIZE, int BSize, int RSize>
class vldComp {
public:
  vldComp() {}

  #pragma hls_design interface ccore
  #pragma hls_pipeline_init_interval 1
  void CCS_BLOCK(run)( typeConfig width, typeConfig height, typeConfig wdKsize, const bool &rst_in, bool &weightEn, bool &outVld ) {
    if (rst_in | weightEn) {
      widthCntr = heightCntr = totalCnt = vldCnt = 0;
      totalCnt = wdKsize + ((KSIZE - 1) * (PIPELENGTH + 1));
      vldCnt = wdKsize - width + KSIZE - 1 + ((KSIZE - 1) * (PIPELENGTH + 1));
      outVld = false;
      widthVld = true;
      heightVld = true;
      oldwidthCntr = 0;
      oldheightCntr = 0;
      startCnt = idCnt = 0;
      start = (idCnt == 0);     // first column (first output channel) starts immediately
    }
    else {
      if (!start) {
        startCnt++;
        if (startCnt == KSIZE + 1) {
          idCnt++;
          startCnt = 0;
        }
        start = (idCnt == 0);
        outVld = false;
      }
      else {
        if (widthCntr == totalCnt) {
          totalCnt = width;
          widthCntr = 0;
          widthVld = true;
          if (heightVld) { oldheightCntr = heightCntr; }
          heightCntr++;
          if (oldheightCntr + 1 == heightCntr) {
            heightVld = true;
          }
          else {
            heightVld = false;
          }
          vldCnt = KSIZE - 1;
        }
        outvldBuff = (widthCntr >= vldCnt) & (heightCntr < height - KSIZE + 1);
        outVld = outvldBuff & widthVld & heightVld;
        widthCntr++;
        if (outvldBuff) {
          if (widthVld) {
            oldwidthCntr = strCntr;
          }
          strCntr++;
          if (oldwidthCntr + 1 == strCntr) {
            widthVld = true;
          }
          else {
            widthVld = false;
          }
        }
        else {
          strCntr = 0;
        }
      }
    }
  }

private:
  typeConfig widthCntr;             // Width Counter
  typeConfig heightCntr;            // Height Counter
  typeConfig oldwidthCntr;          // Stores the previuos width count - only in striding
  typeConfig oldheightCntr;         // Stores the previuos height count - only in striding
  typeConfig totalCnt;              // Current total loop count
  typeConfig vldCnt;                // Current valid count
  typeConfig strCntr;               // Stride counter

  typeConfig startCnt;              // Start counter - only in multiple output channel
  typeConfig idCnt;                 // id counter to validate the current vldComp block - only in multiple output channel

  bool outvldBuff;                  // Valid signal for the current output pixel
  bool widthVld;                    // Valid signal for the current width - only if striding is more than 1
  bool heightVld;                   // Valid signal for the current height - only if striding is more than 1
  bool start;                       // Start signal to start the current vldComp block
};

//============================================================================================================
// Class: peArray
// Description:
//      This class implements the interconnection between the Processing elements, lineBuffers and the Valid checking modules.
//
//============================================================================================================

template <class pType, class typeOutput, class typeConfig, int WIDTH, int HEIGHT, int KSIZE>
class peArray {
public: // helper enums
  enum {
    BSize = (1 << ac::log2_ceil<KSIZE>::val),       // power-of-2 size of pixel and weight buffer
    RSize = (1 << ac::log2_ceil < (KSIZE - 1) * (PIPELENGTH + 1) + 1 >::val), // power-of-2 size of output regs buffer
    YBits = (ac::log2_ceil < (KSIZE - 1) * (PIPELENGTH + 1) + 1 >::val) // number of bits to represent a row number
  };

public:
  peArray() {
    #pragma hls_unroll yes
    for (int i = 0; i < PIPELENGTH; i++) {
      #pragma hls_unroll yes
      for (int j = 0; j < KSIZE; j++) {
        pipelinePSUM[j][i] = 0;
      }
    }
    #pragma hls_unroll yes
    for (int i = 0; i < KSIZE; i++) {
      psumOut[i] = 0;
      dataOut[i] = 0;
    }
    ac::init_array<AC_VAL_0>(buffWeight, KSIZE);
  }

  #pragma hls_design interface ccore
  #pragma hls_pipeline_init_interval 1
  void CCS_BLOCK(run)(pType dataIn,
                      pType partialIn,
                      pType weightIn[KSIZE],
                      typeConfig width, typeConfig height, typeConfig wdKsize,
                      bool rst_in, bool weightEn,
                      typeOutput &outFilter) {
    //Generating Valid Signals for all output Data
    vldObj.run( width, height, wdKsize, rst_in, weightEn, outVld);
    //Stepping the array of line Buffers
    lineBuffers.run(dataIn, rst_in, weightEn, inBuff, width);
    // Preparing Partial_sum_Input using sumOutput from PEs
    #pragma hls_unroll yes
    OUTSHIFT: for (int k = (KSIZE) - 1; k > 0; k--) {
      psumIn[k] = pipelinePSUM[k][PIPELENGTH - 1];
      #pragma hls_unroll yes
      for (int j = (PIPELENGTH - 1); j > 0; j--) {
        pipelinePSUM[ k][j] = pipelinePSUM[k][j - 1];
      }
      pipelinePSUM[k][0] = psumOut[k - 1];
    }
    psumIn[0] = partialIn;
    #pragma hls_unroll yes
    for (int k = 0; k < (KSIZE); k++) {
      buffIN[k] = inBuff[k % KSIZE];
      buffWeight[k] = weightIn[k];
    }
    weightEnbuff = weightEn;
    #pragma hls_unroll yes
    ARRAY_INTERCONNECT: for (int y = 0; y < KSIZE; y++) {
      peInstArray[y].run(buffIN[y], psumIn[y], buffWeight[y], y, weightEnbuff, dataOut[y], weightOut[y], psumOut[y]);
    }
    outFilter.data = psumOut[(KSIZE) - 1];
    outFilter.vld = outVld;
  }
private:
  filterPE<pType, KSIZE, BSize, RSize, YBits> peInstArray[KSIZE];  // PE array
  lineBuffer<pType, typeConfig, WIDTH, KSIZE> lineBuffers;         // Line buffer array
  vldComp<typeConfig, KSIZE, BSize, RSize> vldObj;                 // Valid checking block array

  //Interconnections
  pType psumOut[KSIZE];
  pType psumIn[KSIZE];
  pType buffWeight[KSIZE];
  pType buffIN[KSIZE];
  pType dataOut[KSIZE];
  pType weightOut[KSIZE];
  pType inBuff[KSIZE];
  pType pipelinePSUM[KSIZE][PIPELENGTH];
  bool outVld;
  bool weightEnbuff;
};

//============================================================================================================
// Class: ac_filter2d
// Description:
//      This class controls the over all operations for convolution and control logic of PE Array. It includes
//      loading of the configuration data Kernel weight streaming in and out the feature maps.
//============================================================================================================

#pragma hls_design top
template <class pType, int WIDTH, int HEIGHT, int KSIZE, int PADDING>
class ac_filter2d {

public: // helper enums
  enum {
    DW = (1 << ac::log2_ceil<ac::log2_ceil<(WIDTH *HEIGHT)>::val>::val + 1),
    CW = (1 << ac::log2_ceil < ac::log2_ceil < (WIDTH * HEIGHT + KSIZE + KSIZE - 1) >::val >::val),
    WR = (1 << ac::log2_ceil<(KSIZE)>::val),
    WC = (1 << ac::log2_ceil<(KSIZE)>::val),
    numWeights = KSIZE
  };

  typedef ac_int<DW, false> typeConfig;
  typedef filter2dOut<pType, WIDTH, HEIGHT, KSIZE, PADDING > typeOutput;
  typedef filter2dconfig<pType, WIDTH, HEIGHT, KSIZE, PADDING > filterConf;
public:
  ac_filter2d() {
    mainIter = startIter = 0;
    psumIn = 0;
    w_cnt = h_cnt = 0;
  }

  #pragma hls_design interface
  #pragma hls_pipeline_init_interval 1
  void CCS_BLOCK(run)(ac_channel< pType > &inFilter, ac_channel<filterConf > &configIn, ac_channel< typeOutput > &outFilter) {
    filterConf configuration;
    typeConfig config[6];
    // read input data
    pType input;
    configuration = configIn.read();
    do {
      LDCONFIG: if (mainIter == 0) {
        padWidth  = configuration.width + 2 * PADDING;
        padHeight = configuration.height + 2 * PADDING;
        padWKsize = padWidth * KSIZE;
        padWHsize = padWidth * padHeight ; // Pad_width * Pad_Height (Img size)
        // Total execution count once the input pixel sequence starts (Total image size + flushing count)
        loopCnt = padWHsize + KSIZE * (PIPELENGTH + 1) + KSIZE - PIPELENGTH + 1;
        reset = true;
        weightEn = false;
        weightCnt = 0;
      }
      else if (mainIter > 0 & mainIter < numWeights + 1) {
        // Weight Loading
        weightEn = true;
        //#pragma hls_unroll yes
        for (int i = 0; i < WC; i++) {
          // Since the column of the weight matrix is greater than KSIZE, we don't have to read the location after KSIZE
          if (i < KSIZE) {
            ac_int<ac::log2_ceil<WC>::val, false> tmp_cnt = i;
            input = inFilter.read();
            weight[i] = input;
          }
        }
        weightCnt++;
      }
      else {
        reset = weightEn = false;
        if (startIter < padWHsize) {
          if ((w_cnt >= PADDING & w_cnt < configuration.width + PADDING) & (h_cnt >= PADDING & h_cnt < configuration.height + PADDING)) {
            input = inFilter.read();
          }
          else {
            input = 0;
          }
          dataIn = input;
        }
        startIter++;
        w_cnt++;
        if (w_cnt == padWidth) {
          w_cnt = 0;
          h_cnt++;
        }
      }
      arrayInst.run(dataIn, psumIn, weight, padWidth, padHeight, padWKsize, reset, weightEn, psumOut);
      output.data = psumOut.data;
      output.vld = psumOut.vld;
      outFilter.write(output);
      mainIter++;
      done = (startIter == loopCnt);
    } while (!done);
  }

private:
  peArray<pType, typeOutput, typeConfig, WIDTH, HEIGHT, KSIZE> arrayInst;
  pType weight[KSIZE];
  pType dataIn;
  pType psumIn;
  typeOutput psumOut;
  typeOutput output;
  ac_int< CW, false> mainIter;
  ac_int< CW, false> loopCnt;
  typeConfig startIter;
  ac_int<ac::log2_ceil<WR>::val, false> weightCnt;

  typeConfig padWidth;
  typeConfig padHeight;
  typeConfig padWKsize;
  typeConfig padWHsize;
  typeConfig h_cnt;
  typeConfig w_cnt;

  bool reset;
  bool weightEn;
  bool done;
};
#endif

