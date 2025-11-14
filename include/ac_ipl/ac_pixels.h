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
 *  Copyright 2018 Siemens                                                *
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
// Description:
// Usage:
// Notes:
// Revision History:
//		3.7.1  - abeemana - Removed RGB_pv struct. Added base class Color_pv, derived classes
//                        RGB_pv, YCbCr_pv
//    3.7.0  - dgb - Removed 'private' guard around data
//    3.4.3  - dgb - Updated compiler checks to work with MS VS 2019
//******************************************************************************************

#ifndef _INCLUDED_AC_PIXELS_H_
#define _INCLUDED_AC_PIXELS_H_

// One of the functions (ac_rgb_2_yuv422) has a default template parameter, the use of which
// is only supported by C++11 or later compiler standards. Inform the user if they are not using
// those standards.
#if (defined(__GNUC__) && (__cplusplus < 201103L))
#error Please use C++11 or a later standard for compilation.
#endif
#if (defined(_MSC_VER) && (_MSC_VER < 1920) && !defined(__EDG__))
#error Please use Microsoft VS 2019 or a later standard for compilation.
#endif

// structures, functions and coefficients for image pixel manipulation

#include <ac_int.h>
#include <ac_channel.h>
#include <ac_fixed.h>
#include <ac_matrix.h>

#ifndef __SYNTHESIS__
#include <string>
#include <iostream>
#endif

namespace ac_ipl
{

  // RGB format - 1 Pixel Per Clock
  template <unsigned CDEPTH>
  struct RGB_1PPC {
    enum {
      CDEPTH_ = CDEPTH,
      PPC = 1
    };

    ac_int<CDEPTH,false> G; // TDATA = ZeroPad + R + B + G
    ac_int<CDEPTH,false> B;
    ac_int<CDEPTH,false> R;
    bool                 TUSER; // Start-of-Frame
    bool                 TLAST; // End-of-Line

    RGB_1PPC() {}

    // Initialize color components of RGB_1PPC to a single data value. Initializing the TUSER and TLAST flags is left upto the user.
    template <class T> RGB_1PPC(T p) : R(p), G(p), B(p) {}

    // Use constructor to copy one RGB_1PPC object to another RGB_1PPC object of a different color depth.
    template <unsigned CDEPTH2> RGB_1PPC(RGB_1PPC<CDEPTH2> p) : R(p.R), G(p.G), B(p.B), TUSER(p.TUSER), TLAST(p.TLAST) {}

    // Comparison function, used to enable "==" and "!=" operator overloading.
    bool cmp(const RGB_1PPC<CDEPTH> &op2) const {
      return R == op2.R && G == op2.G && B == op2.B && TUSER == op2.TUSER && TLAST == op2.TLAST;
    }

    // See whether two RGB_1PPC objects have the same elements.
    bool operator == (const RGB_1PPC<CDEPTH> &op2) const {
      return cmp(op2);
    }

    // See whether two RGB_1PPC objects do not have the same elements.
    bool operator != (const RGB_1PPC<CDEPTH> &op2) const {
      return !cmp(op2);
    }

    #ifndef __SYNTHESIS__
    // Prints out type information for RGB_1PPC datatype. Is modeled after the type_name() functions in AC datatypes.
    static std::string type_name() {
      std::string r = "RGB_1PPC<";
      r += ac_int<32, false>(CDEPTH).to_string(AC_DEC);
      r += ">";
      return r;
    }
    #endif
  };

  // RGB format - 2 Pixels Per Clock
  template <unsigned CDEPTH>
  struct RGB_2PPC {
    enum {
      CDEPTH_ = CDEPTH,
      PPC = 2
    };

    ac_int<CDEPTH,false> G0; // TDATA = ZeroPad + R1 + B1 + G1 + R0 + B0 + G0
    ac_int<CDEPTH,false> B0;
    ac_int<CDEPTH,false> R0;
    ac_int<CDEPTH,false> G1;
    ac_int<CDEPTH,false> B1;
    ac_int<CDEPTH,false> R1;
    bool                 TUSER; // Start-of-Frame
    bool                 TLAST; // End-of-Line

    RGB_2PPC() {}

    // Initialize color components of RGB_2PPC to a single data value. Initializing the TUSER and TLAST flags is left upto the user.
    template <class T> RGB_2PPC(T p) : R0(p), G0(p), B0(p), R1(p), G1(p), B1(p) {}

    // Copy one RGB_2PPC object to another RGB_2PPC object with a different bitwidth, using a constructor.
    template <unsigned CDEPTH2> RGB_2PPC(RGB_2PPC<CDEPTH2> p) : R0(p.R0), G0(p.G0), B0(p.B0), R1(p.R1), G1(p.G1), B1(p.B1), TUSER(p.TUSER), TLAST(p.TLAST) {}

    bool cmp(const RGB_2PPC<CDEPTH> &op2) const {
      return R0 == op2.R0 && G0 == op2.G0 && B0 == op2.B0 && R1 == op2.R1 && G1 == op2.G1 && B1 == op2.B1 && TUSER == op2.TUSER && TLAST == op2.TLAST;
    }

    // See whether two RGB_2PPC objects have the same elements.
    bool operator == (const RGB_2PPC<CDEPTH> &op2) const {
      return cmp(op2);
    }

    // See whether two RGB_2PPC objects do not have the same elements.
    bool operator != (const RGB_2PPC<CDEPTH> &op2) const {
      return !cmp(op2);
    }

    #ifndef __SYNTHESIS__
    // Prints out type information for RGB_2PPC datatype. Is modeled after the type_name() functions in AC datatypes.
    static std::string type_name() {
      std::string r = "RGB_2PPC<";
      r += ac_int<32, false>(CDEPTH).to_string(AC_DEC);
      r += ">";
      return r;
    }
    #endif
  };

  // RGB format - Meant for intermediate ("imd") calculations that may require datatypes other than unsigned ac_ints.
  template <class T_imd>
  struct RGB_imd {
    typedef T_imd pix_type;

    T_imd R;
    T_imd G;
    T_imd B;

    RGB_imd() {}

    template <class T> RGB_imd(T p) : R(p), G(p), B(p) { }

    // Copy one RGB_imd object to another RGB_imd object with a different internal type.
    template <class T> RGB_imd(RGB_imd<T> p) : R(p.R), G(p.G), B(p.B) { }

    // Copy RGB components of an RGB_1PPC object to RGB_imd object.
    template <unsigned CDEPTH> RGB_imd(RGB_1PPC<CDEPTH> p) : R(p.R), G(p.G), B(p.B) { }

    // Add two RGB_imd objects together.
    template<class T>
    const RGB_imd<typename ac::rt_2T<T_imd, T>::plus> operator + (const RGB_imd<T> p) const {
      RGB_imd<typename ac::rt_2T<T_imd, T>::plus> res;
      res.R = R + p.R;
      res.G = G + p.G;
      res.B = B + p.B;
      return res;
    }

    // Add RGB_imd object and RGB_1PPC object together. The RGB_1PPC object must always be on the
    // RHS of the addition operation.
    // e.g. RGB_imd_obj1 = RGB_imd_obj2 + RGB_1PPC_obj;
    template<unsigned CDEPTH>
    const RGB_imd<typename ac::rt_2T<T_imd, ac_int<CDEPTH, false> >::plus> operator + (const RGB_1PPC<CDEPTH> p) const {
      RGB_imd<typename ac::rt_2T<T_imd, ac_int<CDEPTH, false> >::plus> res;
      res.R = R + p.R;
      res.G = G + p.G;
      res.B = B + p.B;
      return res;
    }

    // Add scalar value to RGB_imd datatype. Note that the return type is not derived, but is
    // instead the same as the type of LHS (i.e. RGB_imd<T_imd>). This is done to avoid compiler
    // errors.
    // The scalar value must always be on the right hand side of the addition operation.
    // e.g. RGB_imd_obj1 = RGB_imd_obj2 + 2;
    template<class T>
    const RGB_imd<T_imd> operator + (const T p) const {
      RGB_imd<T_imd> res;
      res.R = R + p;
      res.G = G + p;
      res.B = B + p;
      return res;
    }

    // Addition assignment operator for addition of RGB_imd value.
    template<class T>
    RGB_imd<T_imd> &operator += (RGB_imd<T> p) {
      R += p.R;
      G += p.G;
      B += p.B;
      return *this;
    }

    // Addition assignment operator for addition of scalar data.
    template<class T>
    RGB_imd<T_imd> &operator += (T p) {
      R += p;
      G += p;
      B += p;
      return *this;
    }

    // Subtract one RGB_imd object from another.
    template<class T>
    const RGB_imd<typename ac::rt_2T<T_imd, T>::plus> operator - (const RGB_imd<T> p) const {
      RGB_imd<typename ac::rt_2T<T_imd, T>::plus> res;
      res.R = R - p.R;
      res.G = G - p.G;
      res.B = B - p.B;
      return res;
    }

    // Multiply the RGB components of an RGB_imd object with a single data value.
    template <class T>
    const RGB_imd<T_imd> operator * (const T p) const {
      RGB_imd<T_imd> res;
      res.R = R*p;
      res.G = G*p;
      res.B = B*p;
      return res;
    }

    // Multiply two RGB_imd objects together.
    template<class T>
    const RGB_imd<typename ac::rt_2T<T_imd, T>::mult> operator * (const RGB_imd<T> p) const {
      RGB_imd<typename ac::rt_2T<T_imd, T>::mult> res;
      res.R = R * p.R;
      res.G = G * p.G;
      res.B = B * p.B;
      return res;
    }

    // Comparison function, used to enable "==" and "!=" operator overloading.
    template<class T>
    bool cmp(const RGB_imd<T> &op2) const {
      return R == op2.R && G == op2.G && B == op2.B;
    }

    // See whether two RGB_imd objects have the same elements.
    template<class T>
    bool operator == (const RGB_imd<T> &op2) const {
      return cmp(op2);
    }

    // See whether two RGB_imd objects do not have the same elements.
    template<class T>
    bool operator != (const RGB_imd<T> &op2) const {
      return !cmp(op2);
    }

    #ifndef __SYNTHESIS__
    // Prints out type information for the RGB_imd datatype. Is modeled after the type_name() functions
    // in AC datatypes.
    static std::string type_name() {
      std::string r = "RGB_imd<";
      r += T_imd::type_name();
      r += ">";
      return r;
    }
    #endif
  };

  // Color_pv class - Has ac_packed_vector ("pv" support). T must satisfy criteria for packed vector support:
  // (a) T must have a statically available called "width" which stores the type's bitwidth.
  // (b) T must have the slc() and set_slc() methods defined and publicly available.
  template <class T>
  class Color_pv
  {
  public:
    enum { width = T::width * 3 };

    //typedef T base_type; The derived classes are not able to use the base_type in Catapult. But this code passes in gcc.

    ac_int<width, false> get_data() const {
      return data;
    }

    void set_data(ac_int<width, false> data_in) {
      data = data_in;
    }

    template<int WS>
    ac_int<WS, false> slc(int index) const {
      return data.template slc<WS>(index);
    }

    template<int W2>
    Color_pv &set_slc(int lsb, ac_int<W2, false> slc_bits) {
      data.set_slc(lsb, slc_bits);
      return *this;
    }

    void reset() {
      data = 0;
    }

    Color_pv() { }

    Color_pv(T p) {
      set_data(p);
    }

    Color_pv(int p) {
      set_data(p);
    }

    Color_pv(double p) {
      set_data(p);
    }

    template <class T2> Color_pv(Color_pv<T2> p) {
      set_data(p.get_data());
    }

    // Comparison function, used to enable "==" and "!=" operator overloading.
    template<class T2>
    bool cmp(Color_pv<T2> op2) const {
      return data == op2.get_data();
    }

    // See whether two Color_pv objects have the same elements.
    template<class T2>
    bool operator == (Color_pv<T2> op2) const {
      return cmp(op2);
    }

    // See whether two Color_pv objects do not have the same elements.
    template<class T2>
    bool operator != (Color_pv<T2> op2) const {
      return !cmp(op2);
    }

    // Multiply two Color_pv objects together.
    template<class T2>
    Color_pv<typename ac::rt_2T<T, T2>::mult> operator * (Color_pv<T2> p) const {
      Color_pv<typename ac::rt_2T<T, T2>::mult> res;
      res.set_data(data * p.get_data());
      return res;
    }

    // Addition assignment operator for accumulation with other Color_pv value.
    template<class T2>
    Color_pv<T> &operator += (Color_pv<T2> p) {
      set_data(data + p.get_data());
      return *this;
    }

    // RGB data is concatenated into a single ac_int value. The lower indices of this value correspond
    // to the red color component, the higher indices correspond to the blue component, and the middle
    // indices correspond to the green component.
    //
    // For instance, if we assume a 24 bpp color value, the color components are arranged as follows:
    //   Color:  r  r  r  r  r  r  r  r  g  g  g  g  g  g  g  g  b  b  b  b  b  b  b  b
    // Bit idx:  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23
    ac_int<width, false> data;

  };

  template <class T>
  class RGB_pv : public Color_pv<T>
  {
  public:

    typedef T base_type;

    T get_R() const {
      T R;
      #pragma hls_waive UMR
      R.set_slc(0, this->data.template slc<T::width>(0));
      return R;
    }

    T get_G() const {
      T G;
      #pragma hls_waive UMR
      G.set_slc(0, this->data.template slc<T::width>(T::width));
      return G;
    }

    T get_B() const {
      T B;
      #pragma hls_waive UMR
      B.set_slc(0, this->data.template slc<T::width>(T::width * 2));
      return B;
    }

    template <class T2> void set_R(T2 R) {
      T color_comp = R;
      #pragma hls_waive UMR
      this->data.set_slc(0, color_comp.template slc<T::width>(0));
    }

    template <class T2> void set_G(T2 G) {
      T color_comp = G;
      #pragma hls_waive UMR
      this->data.set_slc(T::width, color_comp.template slc<T::width>(0));
    }

    template <class T2> void set_B(T2 B) {
      T color_comp = B;
      #pragma hls_waive UMR
      this->data.set_slc(T::width * 2, color_comp.template slc<T::width>(0));
    }

    RGB_pv() { }

    RGB_pv(T p) {
      set_R(p);
      set_G(p);
      set_B(p);
    }

    RGB_pv(int p) {
      set_R(p);
      set_G(p);
      set_B(p);
    }

    RGB_pv(double p) {
      set_R(p);
      set_G(p);
      set_B(p);
    }

    template <class T2> RGB_pv(RGB_pv<T2> p) {
      set_R(p.get_R());
      set_G(p.get_G());
      set_B(p.get_B());
    }

    // Multiply two RGB_pv objects together.
    template<class T2>
    RGB_pv<typename ac::rt_2T<T, T2>::mult> operator * (RGB_pv<T2> p) const {
      RGB_pv<typename ac::rt_2T<T, T2>::mult> res;
      res.set_R(get_R() * p.get_R());
      res.set_G(get_G() * p.get_G());
      res.set_B(get_B() * p.get_B());
      return res;
    }

    // Addition assignment operator for accumulation with other RGB_pv value.
    template<class T2>
    RGB_pv<T> &operator += (RGB_pv<T2> p) {
      set_R(get_R() + p.get_R());
      set_G(get_G() + p.get_G());
      set_B(get_B() + p.get_B());
      return *this;
    }

    // Comparison function, used to enable "==" and "!=" operator overloading.
    template<class T2>
    bool cmp(RGB_pv<T2> op2) const {
      return get_R() == op2.get_R() && get_G() == op2.get_G() && get_B() == op2.get_B();
    }

    // See whether two RGB_pv objects have the same elements.
    template<class T2>
    bool operator == (RGB_pv<T2> op2) const {
      return cmp(op2);
    }

    // See whether two RGB_pv objects do not have the same elements.
    template<class T2>
    bool operator != (RGB_pv<T2> op2) const {
      return !cmp(op2);
    }

    #ifndef __SYNTHESIS__
    // Prints out type information for the RGB_imd datatype. Is modeled after the type_name() functions
    // in AC datatypes.
    static std::string type_name() {
      std::string r = "RGB_pv<";
      r += T::type_name();
      r += ">";
      return r;
    }
    #endif
  };

  template <class T>
  class YCbCr_pv : public Color_pv<T>
  {
  public:
    typedef T base_type;

    T get_Y() const {
      T Y;
      #pragma hls_waive UMR
      Y.set_slc(0, this->data.template slc<T::width>(0));
      return Y;
    }

    T get_Cb() const {
      T Cb;
      #pragma hls_waive UMR
      Cb.set_slc(0, this->data.template slc<T::width>(T::width));
      return Cb;
    }

    T get_Cr() const {
      T Cr;
      #pragma hls_waive UMR
      Cr.set_slc(0, this->data.template slc<T::width>(T::width * 2));
      return Cr;
    }

    template <class T2> void set_Y(T2 Y) {
      T color_comp = Y;
      #pragma hls_waive UMR
      this->data.set_slc(0, color_comp.template slc<T::width>(0));
    }

    template <class T2> void set_Cb(T2 Cb) {
      T color_comp = Cb;
      #pragma hls_waive UMR
      this->data.set_slc(T::width, color_comp.template slc<T::width>(0));
    }

    template <class T2> void set_Cr(T2 Cr) {
      T color_comp = Cr;
      #pragma hls_waive UMR
      this->data.set_slc(T::width * 2, color_comp.template slc<T::width>(0));
    }

    YCbCr_pv() { }

    YCbCr_pv(T p) {
      set_Y(p);
      set_Cb(p);
      set_Cr(p);
    }

    YCbCr_pv(int p) {
      set_Y(p);
      set_Cb(p);
      set_Cr(p);
    }

    YCbCr_pv(double p) {
      set_Y(p);
      set_Cb(p);
      set_Cr(p);
    }

    template <class T2> YCbCr_pv(YCbCr_pv<T2> p) {
      set_Y(p.get_Y());
      set_Cb(p.get_Cb());
      set_Cr(p.get_Cr());
    }

    #ifndef __SYNTHESIS__
    // Prints out type information for the YCbCr_pv class. Is modeled after the type_name() functions
    // in AC datatypes.
    static std::string type_name() {
      std::string r = "YCbCr_pv<";
      r += T::type_name();
      r += ">";
      return r;
    }
    #endif
  };

  // YUV - 1 Pixel Per Clock
  template <unsigned CDEPTH>
  struct YUV_1PPC {
    ac_int<CDEPTH,false> Y; // TDATA = ZeroPad + Cr + Cb + Y
    ac_int<CDEPTH,false> Cb;
    ac_int<CDEPTH,false> Cr;
    bool                 TUSER; // Start-of-Frame
    bool                 TLAST; // End-of-Line
  };

  // YUV 4:2:2 - 2 Pixels Per Clock
  template <unsigned CDEPTH>
  struct YUV422_2PPC {
    ac_int<CDEPTH,false> Y0; // TDATA = ZeroPad + Cr0 + Y1 + Cb0 + Y0
    ac_int<CDEPTH,false> Cb0;
    ac_int<CDEPTH,false> Y1;
    ac_int<CDEPTH,false> Cr0;
    bool                 TUSER; // Start-of-Frame
    bool                 TLAST; // End-of-Line
  };

  // YUV 4:4:4 - 2 Pixels Per Clock
  template <unsigned CDEPTH>
  struct YUV444_2PPC {
    ac_int<CDEPTH,false> Y0; // TDATA = ZeroPad + Cr1 + Cb1 + Y1 + Cr0 + Cb0 + Y0
    ac_int<CDEPTH,false> Cb0;
    ac_int<CDEPTH,false> Cr0;
    ac_int<CDEPTH,false> Y1;
    ac_int<CDEPTH,false> Cb1;
    ac_int<CDEPTH,false> Cr1;
    bool                 TUSER; // Start-of-Frame
    bool                 TLAST; // End-of-Line
  };

  template <int FractBits>
  class BT601 {
    public:
      static constexpr int ID = 601; // Unique identifier for BT601
    
      typedef ac_matrix<ac_fixed<1 + FractBits, 1, true>, 3, 3> constMatrix_type;
    
      // Full swing equation
      static constMatrix_type getFullSwingMatrix() {
        return constMatrix_type{{
            0.299,                                                      0.587,                                                      0.114,
           -0.16873589164785551819392139805131591856479644775390625,   -0.331264108352144426294927370690857060253620147705078125,   0.5,
            0.5,                                                       -0.418687589158345196960198109081829898059368133544921875,  -0.08131241084165478916201408310371334664523601531982421875
        }};
      }
    };

  template <int FractBits>
  class BT709 {
  public:
    static constexpr int ID = 709; // Unique identifier for BT709
  
    typedef ac_matrix<ac_fixed<1 + FractBits, 1, true>, 3, 3> constMatrix_type;
  
    // Full swing equation
    static constMatrix_type getFullSwingMatrix() {
      return constMatrix_type{{
          0.2126,                                                       0.7152,                                                      0.0722,
         -0.11457210605733995911759137698027188889682292938232421875,  -0.385427893942660027004620815205271355807781219482421875,    0.5,
          0.5,                                                         -0.45415290830581656056352812811383046209812164306640625,    -0.045847091694183390864214544535570894367992877960205078125    
      }};
    }
  };

  template <int FractBits>
  class BT2020 {
  public:
    static constexpr int ID = 2020; // Unique identifier for BT2020
  
    typedef ac_matrix<ac_fixed<1 + FractBits, 1, true>, 3, 3> constMatrix_type;
  
    // Full swing equation
    static constMatrix_type getFullSwingMatrix() {
      return constMatrix_type{{
          0.2627,                                                     0.6780,                                                     0.0593,
         -0.1396300627192516297103708211579942144453525543212890625, -0.36036993728074839804520479447091929614543914794921875,    0.5,
          0.5,                                                       -0.459785704597857114439563019914203323423862457275390625,  -0.04021429540214295494937601915808045305311679840087890625     
      }};
    }
  };

  template <int FractBits>
  class BT2100 {
  public:
    static constexpr int ID = 2100; // Unique identifier for BT2020
  
    typedef ac_matrix<ac_fixed<1 + FractBits, 1, true>, 3, 3> constMatrix_type;
  
    // Full swing equation
    static constMatrix_type getFullSwingMatrix() {
      return constMatrix_type{{
          0.2627,                                                     0.6780,                                                     0.0593,
         -0.1396300627192516297103708211579942144453525543212890625, -0.36036993728074839804520479447091929614543914794921875,    0.5,
          0.5,                                                       -0.459785704597857114439563019914203323423862457275390625,  -0.04021429540214295494937601915808045305311679840087890625     
      }};
    }
  };

  template<typename Standard, bool StudioSwing, unsigned CDEPTH, int FractBits, ac_q_mode Q, typename YcbcrMatrix_type, typename T1, typename T2>
  YCbCr_pv<ac_int<CDEPTH, false> > ac_rgb_2_ycbcr(const T1 &rgb2ycbcrMatrix,
                                                  const T2 &rgbMatrix)
  {
    //Fixed to integer point quantization
    typedef ac_fixed<CDEPTH, CDEPTH, false, Q, AC_SAT> pixFixed_type;
    
    typedef ac_fixed<32, 0, false> constFixed_type;

    //Declare constants
    ac_int<CDEPTH-3, false> constY;
    ac_int<CDEPTH, false> constCb, constCr;

    ac_int<CDEPTH, false> Y, CB, CR;
    YCbCr_pv<ac_int<CDEPTH, false> > YCBCR;
    YcbcrMatrix_type ycbcrMatrix;

    #pragma hls_waive CNS
    if(CDEPTH == 8) {
      constY = 16;
      constCb = 128;
      constCr = 128;
    }
    #pragma hls_waive CNS
    else if(CDEPTH == 10) {
      constY = 64;
      constCb = 512;
      constCr = 512;
    }
    #pragma hls_waive CNS
    else if(CDEPTH == 12) {
      constY = 256;
      constCb = 2048;
      constCr = 2048;
    }

    #pragma hls_design ccore
    #pragma hls_ccore_type combinational
    SCOPE_matrixMul:
    {
      ycbcrMatrix = rgb2ycbcrMatrix * rgbMatrix;
    }  

    #pragma hls_waive CNS
    if(!StudioSwing) {
      constY = 0;
    }
    #pragma hls_waive CNS
    if(Standard::ID == BT601<FractBits>::ID) {
      #pragma hls_waive CNS
      if(!StudioSwing) {
        // Y value
        Y = ((pixFixed_type)(ycbcrMatrix(0,0) + constY)).to_ac_int();
        YCBCR.set_Y(Y);

        //CB value
        CB = ((pixFixed_type)(ycbcrMatrix(1,0) + constCb)).to_ac_int();
        YCBCR.set_Cb(CB);  

        //CR value
        CR = ((pixFixed_type)(ycbcrMatrix(2,0) + constCr)).to_ac_int();
        YCBCR.set_Cr(CR);
      }
      else {
        #pragma hls_waive CNS
        if(CDEPTH == 8) {
          // Y value
          Y = ((pixFixed_type)((ycbcrMatrix(0,0)*(constFixed_type)(.8588)) + constY)).to_ac_int();
          YCBCR.set_Y(Y);

          //CB value
          CB = ((pixFixed_type)((ycbcrMatrix(1,0)*(constFixed_type)(.8784)) + constCb)).to_ac_int();
          YCBCR.set_Cb(CB);  

          //CR value
          CR = ((pixFixed_type)((ycbcrMatrix(2,0)*(constFixed_type)(.8784)) + constCr)).to_ac_int();
          YCBCR.set_Cr(CR);
        }
        #pragma hls_waive CNS
        else if(CDEPTH == 10) {
          // Y value
          Y = ((pixFixed_type)((ycbcrMatrix(0,0)*(constFixed_type)(.8565)) + constY)).to_ac_int();
          YCBCR.set_Y(Y);

          //CB value
          CB = ((pixFixed_type)((ycbcrMatrix(1,0)*(constFixed_type)(.8756)) + constCb)).to_ac_int();
          YCBCR.set_Cb(CB);  

          //CR value
          CR = ((pixFixed_type)((ycbcrMatrix(2,0)*(constFixed_type)(.8756)) + constCr)).to_ac_int();
          YCBCR.set_Cr(CR);
        }
      }
    }
    #pragma hls_waive CNS
    else if(Standard::ID == BT709<FractBits>::ID) {
      #pragma hls_waive CNS
      if(!StudioSwing) {
        // Y value
        Y = ((pixFixed_type)(ycbcrMatrix(0,0) + constY)).to_ac_int();
        YCBCR.set_Y(Y);

        //CB value
        CB = ((pixFixed_type)(ycbcrMatrix(1,0) + constCb)).to_ac_int();
        YCBCR.set_Cb(CB);  

        //CR value
        CR = ((pixFixed_type)(ycbcrMatrix(2,0) + constCr)).to_ac_int();
        YCBCR.set_Cr(CR);
      }
      else {
        #pragma hls_waive CNS
        if(CDEPTH == 8) {
          // Y value
          Y = ((pixFixed_type)((ycbcrMatrix(0,0)*(constFixed_type)(.8588)) + constY)).to_ac_int();
          YCBCR.set_Y(Y);

          //CB value
          CB = ((pixFixed_type)((ycbcrMatrix(1,0)*(constFixed_type)(.8784)) + constCb)).to_ac_int();
          YCBCR.set_Cb(CB);  

          //CR value
          CR = ((pixFixed_type)((ycbcrMatrix(2,0)*(constFixed_type)(.8784)) + constCr)).to_ac_int();
          YCBCR.set_Cr(CR);
        }
        #pragma hls_waive CNS
        else if(CDEPTH == 10) {
          // Y value
          Y = ((pixFixed_type)((ycbcrMatrix(0,0)*(constFixed_type)(.8565)) + constY)).to_ac_int();
          YCBCR.set_Y(Y);

          //CB value
          CB = ((pixFixed_type)((ycbcrMatrix(1,0)*(constFixed_type)(.8756)) + constCb)).to_ac_int();
          YCBCR.set_Cb(CB);  

          //CR value
          CR = ((pixFixed_type)((ycbcrMatrix(2,0)*(constFixed_type)(.8756)) + constCr)).to_ac_int();
          YCBCR.set_Cr(CR);
        }
      }
    }    
    #pragma hls_waive CNS
    else if(Standard::ID == BT2020<FractBits>::ID || Standard::ID == BT2100<FractBits>::ID) {
      #pragma hls_waive CNS
      if(!StudioSwing) {
        #pragma hls_waive CNS
        if(CDEPTH == 10) {
          //Y value
          Y  = ((pixFixed_type)(ycbcrMatrix(0,0))).to_ac_int();
          YCBCR.set_Y(Y);

          //CB value
          CB = ((pixFixed_type)((ycbcrMatrix(1,0)) + constCb)).to_ac_int();
          YCBCR.set_Cb(CB);

          //CR value
          CR = ((pixFixed_type)((ycbcrMatrix(2,0)) + constCr)).to_ac_int();
          YCBCR.set_Cr(CR); 
        }
        #pragma hls_waive CNS
        else if(CDEPTH == 12) {
          //Y value
          Y  = ((pixFixed_type)(ycbcrMatrix(0,0))).to_ac_int();
          YCBCR.set_Y(Y);

          //CB value
          CB = ((pixFixed_type)((ycbcrMatrix(1,0)) + constCb)).to_ac_int();
          YCBCR.set_Cb(CB);

          //CR value
          CR = ((pixFixed_type)((ycbcrMatrix(1,0)) + constCr)).to_ac_int();
          YCBCR.set_Cr(CR);          
        }
      }
      else {
        #pragma hls_waive CNS
        if(CDEPTH == 10) {
          // Y value
          Y = ((pixFixed_type)((ycbcrMatrix(0,0)*(constFixed_type)(.876)) + constY)).to_ac_int();
          YCBCR.set_Y(Y);

          //CB value
          CB = ((pixFixed_type)((ycbcrMatrix(1,0)*(constFixed_type)(.896)) + constCb)).to_ac_int();
          YCBCR.set_Cb(CB);  

          //CR value
          CR = ((pixFixed_type)((ycbcrMatrix(2,0)*(constFixed_type)(.896)) + constCr)).to_ac_int();
          YCBCR.set_Cr(CR);
        }
        #pragma hls_waive CNS
        else if(CDEPTH == 12) {
          // Y value
          Y = ((pixFixed_type)((ycbcrMatrix(0,0)*(constFixed_type)(.3504)) + constY)).to_ac_int();
          YCBCR.set_Y(Y);

          //CB value
          CB = ((pixFixed_type)((ycbcrMatrix(1,0)*(constFixed_type)(.3584)) + constCb)).to_ac_int();
          YCBCR.set_Cb(CB);  

          //CR value
          CR = ((pixFixed_type)((ycbcrMatrix(2,0)*(constFixed_type)(.3584)) + constCr)).to_ac_int();
          YCBCR.set_Cr(CR);         
        }
      }
    }
    
    return YCBCR;
  }

  // RGB -> YUV (Implied 1 Pixel Per Clock)
  template <unsigned CDEPTH>
  inline YUV_1PPC<CDEPTH> ac_rgb_2_yuv(const RGB_1PPC<CDEPTH> rgbin)
  {
    ac_fixed<16,1,true> RGB2YUV_BT601[3][3] = {
      { 0.299,  0.587,  0.114    },
      {-0.168935, -0.331655,  0.50059  },
      { 0.499813, -0.418531, -0.08128  }
    };
    YUV_1PPC<CDEPTH> yuv;
    ac_fixed<16,8,true> tmp;
    tmp  = RGB2YUV_BT601[0][0]*rgbin.R + RGB2YUV_BT601[0][1]*rgbin.G + RGB2YUV_BT601[0][2]*rgbin.B +   0;
    yuv.Y = tmp.to_uint();
    tmp  = RGB2YUV_BT601[1][0]*rgbin.R + RGB2YUV_BT601[1][1]*rgbin.G + RGB2YUV_BT601[1][2]*rgbin.B + 128;
    yuv.Cb = tmp.to_uint();
    tmp  = RGB2YUV_BT601[2][0]*rgbin.R + RGB2YUV_BT601[2][1]*rgbin.G + RGB2YUV_BT601[2][2]*rgbin.B + 128;
    yuv.Cr = tmp.to_uint();
    yuv.TUSER = rgbin.TUSER;
    yuv.TLAST = rgbin.TLAST;
    return yuv;
  }

  // RGB -> YUV 4:4:4 (Implied 2 Pixel Per Clock)
  template <unsigned CDEPTH>
  inline YUV444_2PPC<CDEPTH> ac_rgb_2_yuv444(const RGB_2PPC<CDEPTH> rgbin)
  {
    ac_fixed<16,1,true> RGB2YUV_BT601[3][3] = {
      { 0.299,  0.587,  0.114    },
      {-0.168935, -0.331655,  0.50059  },
      { 0.499813, -0.418531, -0.08128  }
    };
    YUV444_2PPC<CDEPTH> yuv;
    ac_fixed<16,8,true> tmp;
    tmp  = RGB2YUV_BT601[0][0]*rgbin.R0 + RGB2YUV_BT601[0][1]*rgbin.G0 + RGB2YUV_BT601[0][2]*rgbin.B0 +   0;
    yuv.Y0  = tmp.to_uint();
    tmp  = RGB2YUV_BT601[1][0]*rgbin.R0 + RGB2YUV_BT601[1][1]*rgbin.G0 + RGB2YUV_BT601[1][2]*rgbin.B0 + 128;
    yuv.Cb0 = tmp.to_uint();
    tmp  = RGB2YUV_BT601[2][0]*rgbin.R0 + RGB2YUV_BT601[2][1]*rgbin.G0 + RGB2YUV_BT601[2][2]*rgbin.B0 + 128;
    yuv.Cr0 = tmp.to_uint();
    tmp  = RGB2YUV_BT601[0][0]*rgbin.R1 + RGB2YUV_BT601[0][1]*rgbin.G1 + RGB2YUV_BT601[0][2]*rgbin.B1 +   0;
    yuv.Y1  = tmp.to_uint();
    tmp  = RGB2YUV_BT601[1][0]*rgbin.R1 + RGB2YUV_BT601[1][1]*rgbin.G1 + RGB2YUV_BT601[1][2]*rgbin.B1 + 128;
    yuv.Cb1 = tmp.to_uint();
    tmp  = RGB2YUV_BT601[2][0]*rgbin.R1 + RGB2YUV_BT601[2][1]*rgbin.G1 + RGB2YUV_BT601[2][2]*rgbin.B1 + 128;
    yuv.Cr1 = tmp.to_uint();
    yuv.TUSER = rgbin.TUSER;
    yuv.TLAST = rgbin.TLAST;
    return yuv;
  }


  // RGB -> YUV 4:2:2 (Implied 2 Pixel Per Clock)
  template <unsigned CDEPTH, bool average=true>
  inline YUV422_2PPC<CDEPTH> ac_rgb_2_yuv422(const RGB_2PPC<CDEPTH> rgbin)
  {
    ac_fixed<16,1,true> RGB2YUV_BT601[3][3] = {
      { 0.299,  0.587,  0.114    },
      {-0.168935, -0.331655,  0.50059  },
      { 0.499813, -0.418531, -0.08128  }
    };
    YUV444_2PPC<CDEPTH> yuv444;
    YUV422_2PPC<CDEPTH> yuv;
    yuv444 = ac_rgb_2_yuv444(rgbin);
    yuv.Y0 = yuv444.Y0;
    yuv.Y1 = yuv444.Y1;
    if (average) {
      yuv.Cb0 = (yuv444.Cb0 + yuv444.Cb1) >> 2;
      yuv.Cr0 = (yuv444.Cr0 + yuv444.Cr1) >> 2;
    } else {
      yuv.Cb0 = yuv444.Cb0;
      yuv.Cr0 = yuv444.Cr0;
    }
    yuv.TUSER = rgbin.TUSER;
    yuv.TLAST = rgbin.TLAST;
    return yuv;
  }

  // YUV -> RGB (Implied 1 Pixel Per Clock)
  template <unsigned CDEPTH>
  inline RGB_1PPC<CDEPTH> ac_yuv_2_rgb(const YUV_1PPC<CDEPTH> yuvin)
  {
    ac_fixed<16,2,true> YUV2RGB_BT601[3][3] = {
      { 1.0,  0.0,  1.403    },
      { 1.0, -0.344, -0.714    },
      { 1.0,  1.770,  0.0      }
    };
    RGB_1PPC<CDEPTH> rgb;
    ac_fixed<16,8,true> tmp;
    ac_fixed<16,8,true> tmpY = yuvin.Y;
    ac_fixed<16,8,true> tmpU = yuvin.Cb-128;
    ac_fixed<16,8,true> tmpV = yuvin.Cr-128;
    tmp  = YUV2RGB_BT601[0][0]*tmpY + YUV2RGB_BT601[0][1]*tmpU + YUV2RGB_BT601[0][2]*tmpV;
    rgb.R = tmp.to_uint();
    tmp  = YUV2RGB_BT601[1][0]*tmpY + YUV2RGB_BT601[1][1]*tmpU + YUV2RGB_BT601[1][2]*tmpV;
    rgb.G = tmp.to_uint();
    tmp  = YUV2RGB_BT601[2][0]*tmpY + YUV2RGB_BT601[2][1]*tmpU + YUV2RGB_BT601[2][2]*tmpV;
    rgb.B = tmp.to_uint();
    rgb.TUSER = yuvin.TUSER;
    rgb.TLAST = yuvin.TLAST;
    return rgb;
  }

  // YUV 4:2:2 -> RGB (Implied 2 Pixel Per Clock)
  template <unsigned CDEPTH>
  inline RGB_2PPC<CDEPTH> ac_yuv422_2_rgb(const YUV422_2PPC<CDEPTH> yuvin)
  {
    ac_fixed<16,2,true> YUV2RGB_BT601[3][3] = {
      { 1.0,  0.0,  1.403    },
      { 1.0, -0.344, -0.714    },
      { 1.0,  1.770,  0.0      }
    };
    RGB_2PPC<CDEPTH> rgb;
    ac_fixed<16,8,true> tmp;
    ac_fixed<16,8,true> tmpY0 = yuvin.Y0;
    ac_fixed<16,8,true> tmpU = yuvin.Cb0-128;
    ac_fixed<16,8,true> tmpV = yuvin.Cr0-128;
    ac_fixed<16,8,true> tmpY1 = yuvin.Y1;
    tmp  = YUV2RGB_BT601[0][0]*tmpY0 + YUV2RGB_BT601[0][1]*tmpU + YUV2RGB_BT601[0][2]*tmpV;
    rgb.R0 = tmp.to_uint();
    tmp  = YUV2RGB_BT601[1][0]*tmpY0 + YUV2RGB_BT601[1][1]*tmpU + YUV2RGB_BT601[1][2]*tmpV;
    rgb.G0 = tmp.to_uint();
    tmp  = YUV2RGB_BT601[2][0]*tmpY0 + YUV2RGB_BT601[2][1]*tmpU + YUV2RGB_BT601[2][2]*tmpV;
    rgb.B0 = tmp.to_uint();
    tmp  = YUV2RGB_BT601[0][0]*tmpY1 + YUV2RGB_BT601[0][1]*tmpU + YUV2RGB_BT601[0][2]*tmpV;
    rgb.R1 = tmp.to_uint();
    tmp  = YUV2RGB_BT601[1][0]*tmpY1 + YUV2RGB_BT601[1][1]*tmpU + YUV2RGB_BT601[1][2]*tmpV;
    rgb.G1 = tmp.to_uint();
    tmp  = YUV2RGB_BT601[2][0]*tmpY1 + YUV2RGB_BT601[2][1]*tmpU + YUV2RGB_BT601[2][2]*tmpV;
    rgb.B1 = tmp.to_uint();
    rgb.TUSER = yuvin.TUSER;
    rgb.TLAST = yuvin.TLAST;
    return rgb;
  }

  // YUV 4:4:4 -> RGB (Implied 2 Pixel Per Clock)
  template <unsigned CDEPTH>
  inline RGB_2PPC<CDEPTH> ac_yuv422_2_rgb(const YUV444_2PPC<CDEPTH> yuvin)
  {
    ac_fixed<16,2,true> YUV2RGB_BT601[3][3] = {
      { 1.0,  0.0,  1.403    },
      { 1.0, -0.344, -0.714    },
      { 1.0,  1.770,  0.0      }
    };
    RGB_2PPC<CDEPTH> rgb;
    ac_fixed<16,8,true> tmp;
    ac_fixed<16,8,true> tmpY0 = yuvin.Y0;
    ac_fixed<16,8,true> tmpU0 = yuvin.Cb0-128;
    ac_fixed<16,8,true> tmpV0 = yuvin.Cr0-128;
    ac_fixed<16,8,true> tmpU1 = yuvin.Cb1-128;
    ac_fixed<16,8,true> tmpV1 = yuvin.Cr1-128;
    ac_fixed<16,8,true> tmpY1 = yuvin.Y1;
    tmp  = YUV2RGB_BT601[0][0]*tmpY0 + YUV2RGB_BT601[0][1]*tmpU0 + YUV2RGB_BT601[0][2]*tmpV0;
    rgb.R0 = tmp.to_uint();
    tmp  = YUV2RGB_BT601[1][0]*tmpY0 + YUV2RGB_BT601[1][1]*tmpU0 + YUV2RGB_BT601[1][2]*tmpV0;
    rgb.G0 = tmp.to_uint();
    tmp  = YUV2RGB_BT601[2][0]*tmpY0 + YUV2RGB_BT601[2][1]*tmpU0 + YUV2RGB_BT601[2][2]*tmpV0;
    rgb.B0 = tmp.to_uint();
    tmp  = YUV2RGB_BT601[0][0]*tmpY1 + YUV2RGB_BT601[0][1]*tmpU1 + YUV2RGB_BT601[0][2]*tmpV1;
    rgb.R1 = tmp.to_uint();
    tmp  = YUV2RGB_BT601[1][0]*tmpY1 + YUV2RGB_BT601[1][1]*tmpU1 + YUV2RGB_BT601[1][2]*tmpV1;
    rgb.G1 = tmp.to_uint();
    tmp  = YUV2RGB_BT601[2][0]*tmpY1 + YUV2RGB_BT601[2][1]*tmpU1 + YUV2RGB_BT601[2][2]*tmpV1;
    rgb.B1 = tmp.to_uint();
    rgb.TUSER = yuvin.TUSER;
    rgb.TLAST = yuvin.TLAST;
    return rgb;
  }

};

//Printing out ac_pixels objects.
#ifndef __SYNTHESIS__
//Print out an RGB_1PPC struct.
template<unsigned CDEPTH>
std::ostream &operator << (std::ostream &os, const ac_ipl::RGB_1PPC<CDEPTH> &input)
{
  // Print out RGB_1PPC struct in an "{R, G, B}" format
  os << "{" << input.R << ", " << input.G << ", " << input.B << "}";
  return os;
}

//Print out an RGB_2PPC struct.
template<unsigned CDEPTH>
std::ostream &operator << (std::ostream &os, const ac_ipl::RGB_2PPC<CDEPTH> &input)
{
  // Print out RGB_2PPC struct in an "{{R0, G0, B0}, {R1, G1, B1}}" format
  os << "{{" << input.R0 << ", " << input.G0 << ", " << input.B0 << "}, {" << input.R1 << ", " << input.G1 << ", " << input.B1 << "}}";
  return os;
}

//Print out an RGB_imd struct.
template<class T_imd>
std::ostream &operator << (std::ostream &os, const ac_ipl::RGB_imd<T_imd> &input)
{
  // Print out RGB_imd struct in an "{R, G, B}" format
  os << "{" << input.R << ", " << input.G << ", " << input.B << "}";
  return os;
}

//Print out an RGB_pv struct.
template<class T>
std::ostream &operator << (std::ostream &os, const ac_ipl::RGB_pv<T> &input)
{
  // Print out RGB_pv struct in an "{R, G, B}" format
  os << "{" << input.get_R() << ", " << input.get_G() << ", " << input.get_B() << "}";
  return os;
}

//Print out a YUV_1PPC struct.
template<unsigned CDEPTH>
std::ostream &operator << (std::ostream &os, const ac_ipl::YUV_1PPC<CDEPTH> &input)
{
  // Print out YUV_1PPC struct in an "{Y, Cb, Cr}" format
  os << "{" << input.Y << ", " << input.Cb << ", " << input.Cr << "}";
  return os;
}

//Print out a YUV422_2PPC struct
template<unsigned CDEPTH>
std::ostream &operator << (std::ostream &os, const ac_ipl::YUV422_2PPC<CDEPTH> &input)
{
  // Print out YUV422_2PPC struct in an "{Y0, Cb0, Y1, Cr0}" format
  os << "{" << input.Y0 << ", " << input.Cb0 << ", " << input.Y1 <<  ", " << input.Cr0 << "}";
  return os;
}

//Print out a YUV444_2PPC struct
template<unsigned CDEPTH>
std::ostream &operator << (std::ostream &os, const ac_ipl::YUV444_2PPC<CDEPTH> &input)
{
  // Print out YUV444_2PPC struct in an "{{Y0, Cb0, Cr0}, {Y1, Cb1, Cr1}}" format
  os << "{{" << input.Y0 << ", " << input.Cb0 << ", " << input.Cr0 << "}, {" << input.Y1 << ", " << input.Cb1 << ", " << input.Cr1 << "}}";
  return os;
}
#endif // #ifndef __SYNTHESIS__

#endif // #ifndef _INCLUDED_AC_PIXELS_H_
