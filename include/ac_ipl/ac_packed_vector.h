/**************************************************************************
 *                                                                        *
 *  Algorithmic C (tm) Image Processing Library                           *
 *                                                                        *
 *  Software Version: 2026.2                                              *
 *                                                                        *
 *  Release Date    : Tue Jun 30 15:08:22 PDT 2026                        *
 *  Release Type    : Production Release                                  *
 *  Release Build   : 2026.2.1                                            *
 *                                                                        *
 *  Copyright 2023 Siemens                                                *
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
//  File : ac_packed_vector.h
//
//  Description:
//    Container class which has where all the data is concatenated in a
//    single ac_int<N, false> member variable where
//    N = Bitwidth of base type*number of words.
//
//    The class is instantiated with two template parameters:
//    - T: Base data-type. T must (a) Have a member variable "width" which
//        returns the bitwidth of the type, is a compile-time constant and
//        can be scoped into using T::width and (b) Support the slc() and
//        set_slc() methods in a manner similar to ac_int and ac_fixed
//        variables.
//    - AC_WORDS: Specifies how many words are packed in parallel.
//
//    The class provides the following publicly available methods:
//    - void pack_data(const T cpp_arr_in[AC_WORDS]) :
//        Packs a standard C++ input of type T and size AC_WORDS into the
//        underlying data.
//    - void pack_data(ac_array<T, AC_WORDS> ac_arr_in) :
//        Packs AC array of type T and size AC_WORDS into the underlying data.
//    - void pack_data(ac_bank_array_vary<T, AC_WORDS> bank_arr_in) :
//        Packs an ac_bank_array_vary of type T and size AC_WORDS into the underlying data.
//    - void unpack_data(T cpp_arr_out[AC_WORDS]) const :
//        Unpacks underlying data to a standard C++ array of type T and
//        size AC_WORDS.
//    - void unpack_data(ac_array<T, AC_WORDS> &ac_arr_out) const :
//        Unpacks underlying data to an ac_array of type T and size AC_WORDS.
//    - void unpack_data(ac_bank_array_vary<T, AC_WORDS> &bank_arr_out) const :
//        Unpacks underlying data to an ac_bank_array_vary of type T and size AC_WORDS.
//    - void set_data(ac_int<T::width *AC_WORDS, false> data_in) :
//        Sets underlying data to data_in.
//    - ac_int<T::width *AC_WORDS, false> get_data() const :
//        Gets underlying data.
//    - void set_max() :
//        Sets underlying data to maximum possible value, i.e. all 1s.
//    - template<int WS> inline const ac_int<WS, false> slc(signed index) const :
//        Slice read method, operates on underlying data.
//    - template<int W2, bool S2>
//      inline ac_packed_vector &set_slc(signed lsb, const ac_int<W2,S2> &slc) :
//        Slice write method, operates on underlying data.
//    - template <typename T2> void cpy_from_scalar(const T2 &scalar) :
//        Copies the input scalar to all elements of the packed vector.
//    - void reset() :
//        Resets underlying data, i.e. sets it to 0.
//    - ac_packed_vector() :
//        Default constructor.
//    - ac_packed_vector(T cpp_arr_in[AC_WORDS]) :
//        Constructs with standard C++ array. Uses pack_data() method.
//    - ac_packed_vector(const T cpp_arr_in[AC_WORDS]) :
//        Constructs with const standard C++ array. Uses pack_data() method.
//    - ac_packed_vector(ac_array<T, AC_WORDS> ac_arr_in) :
//        Constructs with ac_array. Uses pack_data() method.
//    - ac_packed_vector(ac_bank_array_vary<T, AC_WORDS> bank_arr_in) :
//        Constructs with ac_bank_array_vary. Uses pack_data() method.
//    - ac_packed_vector(const T &scalar) :
//        Sets all elements to input scalar of type T.
//    - ac_packed_vector(const c_type scalar) :
//        Sets all elements to input scalar of a C type (int, double, etc)
//    - pv_helper_type operator[] (unsigned idx) :
//        Used to access elements of a non-const ac_packed_vector object.
//        pv_helper_type is a ac_packed_vector_helper type which handles
//        accessing the packed vector at the relevant index.
//    - const T operator[] (unsigned idx) const :
//        Used to read elements from a const ac_packed_vector object.
//    - template<typename... T_args>
//      pv_helper_type operator() (T_args... args) :
//        Used to access elemenets of a non-const ac_packed_vector object.
//        The parentheses operator supports multiple index values via
//        variadic templates, which lets the user index into a nested
//        packed vector.
//        pv_helper_type is a ac_packed_vector_helper type which handles
//        accessing the packed vector at the relevant index/indices.
//    - template<typename... T_args>
//      nested_base_type operator() (T_args... args) :
//        Used to access elements of a const ac_packed_vector object.
//        Like the parentheses operator above, this too supports multiple
//        index values.
//        nested_base_type depends on how far you're indexing into the
//        packed vector.
//    - void operator= (T cpp_arr_in[AC_WORDS]) :
//        Assigns from standard C++ array. Uses pack_data() method.
//    - void operator= (const T cpp_arr_in[AC_WORDS]) :
//        Assigns from standard const C++ array. Uses pack_data() method.
//    - void operator= (ac_array<T, AC_WORDS> ac_arr_in) :
//        Assigns from ac_array. Uses pack_data() method.
//    - void operator= (ac_bank_array_vary<T, AC_WORDS> bank_arr_in) :
//        Assigns from ac_bank_array_vary. Uses pack_data() method.
//    - void operator= (const T &scalar) :
//        Assigns an input scalar of type T to all elements.
//    - void operator= (const c_type scalar) :
//        Assigns an input scalar of a C type (int, double, etc) to all elements.
//    - static std::string type_name() :
//        Non-synthesis only. Prints out type information. Modeled after
//        the type_name() functions of other AC Datatypes. T must have a
//        type_name() method too.
//
//  Usage:
//    Below is a sample design and C++ testbench
//
//    #include <ac_ipl/ac_packed_vector.h>
//
//    typedef ac_int<8, false> pv_base_type;
//    const int pv_words = 5;
//    typedef ac_packed_vector<pv_base_type, pv_words> pv_type;
//    typedef ac_int<5, false> scale_type;
//
//    #pragma hls_design top
//    void design(pv_type pv_in, scale_type scale, pv_type &pv_out)
//    {
//      #pragma hls_unroll yes
//      for (int i = 0; i < pv_words; i++) {
//        pv_base_type pv_in_val = pv_in[i];
//        // Multiply packed vector elements with scale.
//        pv_out[i] = pv_base_type(pv_in_val*scale);
//      }
//    }
//
//    #ifndef __SYNTHESIS__
//    #include <mc_scverify.h>
//    #include <iostream>
//    using namespace std;
//    
//    CCS_MAIN(int argc, char *argv[])
//    {
//      pv_type pv_in, pv_out;
//      scale_type scale = 3;
//
//      for (int i = 0; i < pv_words; i++) {
//        pv_in[i] = pv_base_type(i); // Initialize input packed vector.
//      }
//
//      CCS_DESIGN(design)(pv_in, scale, pv_out);
//
//      // Pretty-print input and output packed vectors.
//      cout << "pv_in = " << pv_in << endl;
//      cout << "pv_out = " << pv_out << endl;
//
//      CCS_RETURN(0);
//    }
//    #endif
//
//***************************************************************************

#ifndef _INCLUDED_AC_PACKED_VECTOR_H_
#define _INCLUDED_AC_PACKED_VECTOR_H_
#include <ac_int.h>
#include <ac_bank_array.h>
#include <ac_array.h>

#ifndef __SYNTHESIS__
#include <string>
#include <iostream>
#endif

// The code uses static assertions, which are only supported by C++11 or later compiler
// standards. Hence, the user should be informed if they are not using those standards.
#if (defined(__GNUC__) && (__cplusplus < 201103L))
#error Please use C++11 or a later standard for compilation.
#endif
#if (defined(_MSC_VER) && (_MSC_VER < 1920) && !defined(__EDG__))
#error Please use Microsoft VS 2019 or a later standard for compilation.
#endif

template <typename T, int data_width>
class ac_packed_vector_helper {
  public:
  typedef ac_int<data_width, false> T_data;

  // ac_packed_vector is a friend class and can access the ctor.
  template <typename T_, int AC_WORDS>
  friend class ac_packed_vector;

  operator T() const {
    T tmp;
    tmp.set_slc(0, data.template slc<T::width>(bit_index));
    return tmp;
  }

  ac_packed_vector_helper& operator= (const T& val) {
    #pragma hls_waive UMR
    data.set_slc(bit_index, val.template slc<T::width>(0));
    return *this;
  }

  ac_packed_vector_helper& operator= (const ac_packed_vector_helper& other) {
    #pragma hls_waive UMR
    data.set_slc(bit_index, other.data.template slc<T::width>(other.bit_index));
    return *this;
  }

  private:
  // Made private so that only ac_packed_vector can instantiate this class.
  ac_packed_vector_helper(T_data *c, const unsigned& idx) : data(*c), bit_index(idx) {}

  T_data &data;
  unsigned bit_index;
};

// Type T must define T::width, as well as support the slc() and set_slc() methods.
template<typename T, int AC_WORDS>
class ac_packed_vector
{
  template <int DUMMY, typename T_, int N_DIMS>
  struct internal_helper {
    static_assert(N_DIMS > 0, "N_DIMS must be positive.");
    static constexpr bool match = false;
  };

  template <int DUMMY, typename T_, int AC_WORDS_>
  struct internal_helper<DUMMY, ac_packed_vector<T_, AC_WORDS_>, 1> {
    static constexpr bool match = true;
    typedef T_ nested_base_type;
    static constexpr int total_elems = AC_WORDS_;

    static void get_flat_idx(unsigned &flat_idx, unsigned final_arg) {
      AC_ASSERT(final_arg < AC_WORDS_, "Out of bounds index access.");
      flat_idx += final_arg;
      flat_idx *= T_::width;
    }

    template<typename T_data, typename... T_args>
    static void dyn_read(const T_data &data, unsigned offset, nested_base_type &nested_data, const unsigned final_arg) {
      AC_ASSERT(final_arg < AC_WORDS_, "Out of bounds index access.");
      #pragma hls_unroll yes
      DYN_READ_FROM_PACKED_VECTOR: for (unsigned i = 0; i < AC_WORDS_; ++i) {
        if (i == final_arg) {
          unsigned bit_idx = (offset + i)*T_::width;
          nested_data.set_slc(0, data.template slc<T_::width>(bit_idx));
        }
      }
    }

    #pragma hls_design ccore combinational
    template <typename T_data>
    static nested_base_type dyn_read_wrapper(const T_data &data, const unsigned arg) {
      nested_base_type nested_data = 0;
      dyn_read(data, 0, nested_data, arg);
      return nested_data;
    }

    template<typename T_data>
    static void dyn_write(const nested_base_type &nested_data, T_data &data, unsigned offset, const unsigned final_arg) {
      AC_ASSERT(final_arg < AC_WORDS_, "Out of bounds index access.");
      #pragma hls_unroll yes
      DYN_WRITE_TO_PACKED_VECTOR: for (unsigned i = 0; i < AC_WORDS_; ++i) {
        if (i == final_arg) {
          unsigned bit_idx = (offset + i)*T_::width;
          data.set_slc(bit_idx, nested_data.template slc<T_::width>(0));
        }
      }
    }

    template<typename T_data>
    static void dyn_write_wrapper(
      const nested_base_type &nested_data,
      T_data &data,
      const unsigned arg
    ) {
      dyn_write(nested_data, data, 0, arg);
    }
  };

  template <int DUMMY, typename T_, int AC_WORDS_, int N_DIMS>
  struct internal_helper<DUMMY, ac_packed_vector<T_, AC_WORDS_>, N_DIMS> {
    static_assert(N_DIMS > 0, "N_DIMS must be positive.");
    typedef internal_helper<DUMMY, T_, N_DIMS - 1> nested_helper_type;
    static constexpr bool match = nested_helper_type::match;
    static constexpr int nested_base_width = nested_helper_type::nested_base_width;
    typedef typename nested_helper_type::nested_base_type nested_base_type;
    static constexpr int total_elems = AC_WORDS_*nested_helper_type::total_elems;

    template<typename... T_args>
    static void get_flat_idx(unsigned &flat_idx, unsigned first_arg, T_args... rest_args) {
      AC_ASSERT(first_arg < AC_WORDS_, "Out of bounds index access.");
      flat_idx += first_arg*nested_helper_type::total_elems;
      nested_helper_type::get_flat_idx(flat_idx, rest_args...);
    }

    template<typename T_data, typename... T_args>
    static void dyn_read(const T_data &data, unsigned offset, nested_base_type &nested_data, const unsigned first_arg, T_args... rest_args) {
      AC_ASSERT(first_arg < AC_WORDS_, "Out of bounds index access.");
      unsigned offset_ = offset;
      #pragma hls_unroll yes
      DYN_READ_FROM_PACKED_VECTOR: for (unsigned i = 0; i < AC_WORDS_; ++i) {
        if (i == first_arg) {
          offset_ += i*nested_helper_type::total_elems;
          nested_helper_type::dyn_read(data, offset_, nested_data, rest_args...);
        }
      }
    }

    #pragma hls_design ccore combinational
    template<typename T_data, typename... T_args>
    static nested_base_type dyn_read_wrapper(
      const T_data &data,
      const unsigned first_arg,
      T_args... rest_args
    ) {
      nested_base_type nested_data = 0;
      dyn_read(data, 0, nested_data, first_arg, rest_args...);
      return nested_data;
    }

    template<typename T_data, typename... T_args>
    static void dyn_write(const nested_base_type &nested_data, T_data &data, unsigned offset, const unsigned first_arg, T_args... rest_args) {
      AC_ASSERT(first_arg < AC_WORDS_, "Out of bounds index access.");
      unsigned offset_ = offset;
      #pragma hls_unroll yes
      DYN_WRITE_TO_PACKED_VECTOR: for (unsigned i = 0; i < AC_WORDS_; ++i) {
        if (i == first_arg) {
          offset_ += i*nested_helper_type::total_elems;
          nested_helper_type::dyn_write(nested_data, data, offset_, rest_args...);
        }
      }
    }

    template<typename T_data, typename... T_args>
    static void dyn_write_wrapper(
      const nested_base_type &nested_data,
      T_data &data,
      const unsigned first_arg,
      T_args... rest_args
    ) {
      dyn_write(nested_data, data, 0, first_arg, rest_args...);
    }
  };

  template <unsigned DIM1, unsigned DIM2>
  void check_dims_2d() {
    #ifndef __SYNTHESIS__
    typedef internal_helper<0, ac_packed_vector, 2> helper_type;
    constexpr bool match = helper_type::match;
    static_assert(match, "Cannot assign a 2D ac_array to a packed vector which doesn't have at least two dimensions.");
    
    const ac_array<unsigned, 2> dim_arr = helper_type::get_dims();
    bool dims_match = (dim_arr[0] == DIM1) && (dim_arr[1] == DIM2);
    AC_ASSERT(dims_match, "Dimensions of nested packed vector and input ac_array do not match.");
    #endif
  }
  
public:
  static_assert(AC_WORDS > 0, "AC_WORDS must be positive.");
  // The typedef and constexpr values below can be accessed by a user for use in their code.
  typedef T base_type;
  static constexpr int packed_words = AC_WORDS;
  static constexpr int width = T::width*AC_WORDS;
  
  void pack_data(const T cpp_arr_in[AC_WORDS]) {
    #pragma hls_unroll yes
    PACK_FROM_CPP_ARRAY: for (int idx = 0; idx < AC_WORDS; idx++) {
      data.set_slc(idx*T::width, cpp_arr_in[idx].template slc<T::width>(0));
    }
  }

  void pack_data(ac_array<T, AC_WORDS> ac_arr_in) {
    #pragma hls_unroll yes
    PACK_FROM_AC_ARRAY: for (int idx = 0; idx < AC_WORDS; idx++) {
      #pragma hls_waive UMR
      data.set_slc(idx*T::width, ac_arr_in[idx].template slc<T::width>(0));
    }
  }

  void pack_data(ac_bank_array_vary<T, AC_WORDS> bank_arr_in) {
    #pragma hls_unroll yes
    PACK_FROM_BANK_ARRAY: for (int idx = 0; idx < AC_WORDS; idx++) {
      #pragma hls_waive UMR
      data.set_slc(idx*T::width, bank_arr_in[idx].template slc<T::width>(0));
    }
  }

  void unpack_data(T cpp_arr_out[AC_WORDS]) const {
    #pragma hls_unroll yes
    UNPACK_TO_CPP_ARRAY: for (int idx = 0; idx < AC_WORDS; idx++) {
      cpp_arr_out[idx].set_slc(0, data.template slc<T::width>(idx*T::width));
    }
  }

  void unpack_data(ac_array<T, AC_WORDS> &ac_arr_out) const {
    #pragma hls_unroll yes
    UNPACK_TO_AC_ARRAY: for (int idx = 0; idx < AC_WORDS; idx++) {
      ac_arr_out[idx].set_slc(0, data.template slc<T::width>(idx*T::width));
    }
  }

  void unpack_data(ac_bank_array_vary<T, AC_WORDS> &bank_arr_out) const {
    #pragma hls_unroll yes
    UNPACK_TO_BANK_ARRAY: for (int idx = 0; idx < AC_WORDS; idx++) {
      bank_arr_out[idx].set_slc(0, data.template slc<T::width>(idx*T::width));
    }
  }

  void set_data(ac_int<T::width *AC_WORDS, false> data_in) {
    data = data_in;
  }
  
  ac_int<T::width *AC_WORDS, false> get_data() const {
    return data;
  }

  void set_dc() {
    data.template set_val<AC_VAL_DC>();
  }

  void set_max() {
    data.template set_val<AC_VAL_MAX>();
  }

  template<int WS>
  inline const ac_int<WS, false> slc(signed index) const {
    return data.template slc<WS>(index);
  }

  template<int W2, bool S2>
  inline ac_packed_vector &set_slc(signed lsb, const ac_int<W2,S2> &slc) {
    #pragma hls_waive UMR
    data.set_slc(lsb, slc);
    return *this;
  }

  template <typename T2>
  void cpy_from_scalar(const T2 &scalar) {
    T scalar_ = scalar; // T2 -> T conversion must be supported.
    ac_int<T::width, false> scalar_bits = scalar_.template slc<T::width>(0);
    #pragma hls_unroll yes
    INIT_WITH_SCALAR: for (int idx = 0; idx < AC_WORDS; idx++) {
      data.set_slc(idx*T::width, scalar_bits);
    }
  }

  void reset() {
    data = 0;
  }

  ac_packed_vector() { }

  ac_packed_vector(T cpp_arr_in[AC_WORDS]) {
    pack_data(cpp_arr_in);
  }

  ac_packed_vector(const T cpp_arr_in[AC_WORDS]) {
    pack_data(cpp_arr_in);
  }

  ac_packed_vector(ac_array<T, AC_WORDS> ac_arr_in) {
    pack_data(ac_arr_in);
  }

  ac_packed_vector(ac_bank_array_vary<T, AC_WORDS> bank_arr_in) {
    pack_data(bank_arr_in);
  }

  // Initialization with scalar of base type.
  ac_packed_vector(const T &scalar) { cpy_from_scalar(scalar); }
  
  // Initialization with scalars of various C++ types.
  ac_packed_vector(const bool scalar) { cpy_from_scalar(scalar); }
  ac_packed_vector(const char scalar) { cpy_from_scalar(scalar); }
  ac_packed_vector(const unsigned char scalar) { cpy_from_scalar(scalar); }
  ac_packed_vector(const short scalar) { cpy_from_scalar(scalar); }
  ac_packed_vector(const unsigned short scalar) { cpy_from_scalar(scalar); }
  ac_packed_vector(const int scalar) { cpy_from_scalar(scalar); }
  ac_packed_vector(const unsigned scalar) { cpy_from_scalar(scalar); }
  ac_packed_vector(const long scalar) { cpy_from_scalar(scalar); }
  ac_packed_vector(const unsigned long scalar) { cpy_from_scalar(scalar); }
  ac_packed_vector(const float scalar) { cpy_from_scalar(scalar); }
  ac_packed_vector(const double scalar) { cpy_from_scalar(scalar); }

  public:
  template<typename... T_args>
  unsigned get_flat_idx(T_args... args) const {
    constexpr int N_DIMS = sizeof...(args);
    typedef internal_helper<0, ac_packed_vector, N_DIMS> helper_type;
    constexpr bool match = helper_type::match;
    static_assert(match, "Number of index parameters must not exceed number of dimensions in nested packed vector.");

    unsigned flat_idx = 0;
    helper_type::get_flat_idx(flat_idx, args...);
    return flat_idx;
  }

  template<typename... T_args>
  ac_packed_vector_helper<typename internal_helper<0, ac_packed_vector, sizeof...(T_args)>::nested_base_type, width> operator() (T_args... args) {
    unsigned flat_idx = get_flat_idx(args...);
    typedef typename internal_helper<0, ac_packed_vector, sizeof...(T_args)>::nested_base_type nested_base_type;
    ac_packed_vector_helper<nested_base_type, width> tmp(&data, flat_idx);
    return tmp;
  }

  template<typename... T_args>
  typename internal_helper<0, ac_packed_vector, sizeof...(T_args)>::nested_base_type operator() (T_args... args) const {
    unsigned flat_idx = get_flat_idx(args...);
    typedef typename internal_helper<0, ac_packed_vector, sizeof...(T_args)>::nested_base_type nested_base_type;
    nested_base_type tmp;
    tmp.set_slc(0, data.template slc<nested_base_type::width>(flat_idx));
    return tmp;
  }

  ac_packed_vector_helper<T, width> operator[] (unsigned idx) {
    #pragma hls_waive CCC
    AC_ASSERT(idx < AC_WORDS, "idx must be lesser than AC_WORDS.");

    ac_packed_vector_helper<T, width> tmp(&data, idx*T::width);
    return tmp;
  }

  const T operator[] (unsigned idx) const {
    #pragma hls_waive CCC
    AC_ASSERT(idx < AC_WORDS, "idx must be lesser than AC_WORDS.");
    T tmp;
    tmp.set_slc(0, data.template slc<T::width>(idx*T::width));
    return tmp;
  }

  // Use this function if you want to dynamically read from a nested ac_packed_vector. It gives a better area than using
  // indexing with the () operator at the cost of C++ runtime.
  template<typename... T_args>
  void dyn_read (
    typename internal_helper<0, ac_packed_vector, sizeof...(T_args)>::nested_base_type &nested_data,
    T_args... args
  ) const {
    constexpr int N_DIMS = sizeof...(args);
    typedef internal_helper<0, ac_packed_vector, N_DIMS> helper_type;
    constexpr bool match = helper_type::match;
    static_assert(match, "Number of index parameters must not exceed number of dimensions in nested packed vector.");
    
    nested_data = helper_type::dyn_read_wrapper(data, args...);
  }

  // Use this function if you want to dynamically write to a nested ac_packed_vector. It gives a better area than using
  // indexing with the () operator at the cost of C++ runtime.
  template<typename... T_args>
  void dyn_write (
    const typename internal_helper<0, ac_packed_vector, sizeof...(T_args)>::nested_base_type &nested_data,
    T_args... args
  ) {
    constexpr int N_DIMS = sizeof...(args);
    typedef internal_helper<0, ac_packed_vector, N_DIMS> helper_type;
    constexpr bool match = helper_type::match;
    static_assert(match, "Number of index parameters must not exceed number of dimensions in nested packed vector.");
    
    helper_type::dyn_write_wrapper(nested_data, data, args...);
  }

  void operator= (T cpp_arr_in[AC_WORDS]) {
    pack_data(cpp_arr_in);
  }

  void operator= (const T cpp_arr_in[AC_WORDS]) {
    pack_data(cpp_arr_in);
  }

  void operator= (ac_array<T, AC_WORDS> ac_arr_in) {
    pack_data(ac_arr_in);
  }

  void operator= (ac_bank_array_vary<T, AC_WORDS> bank_arr_in) {
    pack_data(bank_arr_in);
  }

  // Assignment with scalar of base type.
  void operator= (const T &scalar) { cpy_from_scalar(scalar); }

  // Assigment with scalars of various C++ types.
  void operator= (const bool scalar) { cpy_from_scalar(scalar); }
  void operator= (const char scalar) { cpy_from_scalar(scalar); }
  void operator= (const unsigned char scalar) { cpy_from_scalar(scalar); }
  void operator= (const short scalar) { cpy_from_scalar(scalar); }
  void operator= (const unsigned short scalar) { cpy_from_scalar(scalar); }
  void operator= (const int scalar) { cpy_from_scalar(scalar); }
  void operator= (const unsigned scalar) { cpy_from_scalar(scalar); }
  void operator= (const long scalar) { cpy_from_scalar(scalar); }
  void operator= (const unsigned long scalar) { cpy_from_scalar(scalar); }
  void operator= (const float scalar) { cpy_from_scalar(scalar); }
  void operator= (const double scalar) { cpy_from_scalar(scalar); }

  bool operator== (const ac_packed_vector &rhs) const {
    return (data == rhs.data);
  }

  bool operator!= (const ac_packed_vector &rhs) const {
    return !(operator==(rhs));
  }
  
  static std::string type_name() {
    std::string r = "ac_packed_vector<";
    r += T::type_name();
    r += ",";
    r += ac_int<32, false>(AC_WORDS).to_string(AC_DEC);
    r += ">";
    return r;
  }

// "data" member is public if using SCVerify.
#ifdef CCS_SCVERIFY
public:
#else
private:
#endif
  ac_int<T::width *AC_WORDS, false> data;
};

//=======================================================================
// Pretty-print with ostream operator <<
//-----------------------------------------------------------------------
template<typename T, int AC_WORDS>
std::ostream &operator << (std::ostream &os, ac_packed_vector<T, AC_WORDS> input)
{
  ac_int<T::width*AC_WORDS, false> pv_data = input.get_data();

  // Print out contents as {input[0], input[1], input[2] ... input[AC_WORDS - 1]}
  os << "{";

  for (int idx = 0; idx < AC_WORDS; idx++) {
    T pv_val;
    pv_val.set_slc(0, pv_data.template slc<T::width>(idx*T::width));
    os << pv_val;
    if (idx != AC_WORDS - 1) {
      os << ", ";
    }
  }

  os << "}";

  return os;
}

// Get type information for 2D packed vector.
template <typename Data_type, int Dim1, int Dim2>
struct get_pv_2d_conf
{
  public:
  typedef ac_packed_vector<ac_packed_vector<Data_type, Dim2>, Dim1> type;
};

// Get type information for 3D packed vector.
template <typename Data_type, int Dim1, int Dim2, int Dim3>
struct get_pv_3d_conf
{
  public:
  typedef ac_packed_vector<ac_packed_vector<ac_packed_vector<Data_type, Dim3>, Dim2>, Dim1> type;
};

#endif
