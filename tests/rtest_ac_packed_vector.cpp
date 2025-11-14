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
 *  Copyright  Siemens                                                *
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
// To compile stand-alone and run:
// $MGC_HOME/bin/c++ -std=c++11 -Wall -Wno-unknown-pragmas -Wno-unused-label -fsanitize=address -I$MGC_HOME/shared/include rtest_ac_packed_vector.cpp -o design
// ./design

#include <ac_array.h>
#include <ac_ipl/ac_packed_vector.h>
#include <ac_fixed.h>
#include <ac_math/ac_random.h>

#include <iostream>
using namespace std;

// Returns true if packed vector doesn't match ac_array.
template <class pv_base_type, int pv_words>
bool compare_arrs(
  ac_packed_vector<pv_base_type, pv_words> pv,
  ac_array<pv_base_type, (unsigned)pv_words> ac_arr
) {
  for (int i = 0; i < pv_words; i++) {
    if (ac_arr[i] != pv_base_type(pv[i])) {
      cerr << "Test FAILED. AC array does not match packed vector." << endl;
      #ifdef DEBUG
      cerr << "ac_arr = " << ac_arr << endl;
      cerr << "pv = " << pv << endl;
      #endif
      return true;
    }
  }
  
  return false;
}

// Returns true if packed vector doesn't match C++ array.
template <class pv_base_type, int pv_words>
bool compare_arrs(
  ac_packed_vector<pv_base_type, pv_words> pv,
  const pv_base_type cpp_arr[pv_words]
) {
  for (int i = 0; i < pv_words; i++) {
    if (cpp_arr[i] != pv_base_type(pv[i])) {
      cerr << "Test FAILED. C++ array does not match packed vector." << endl;
      #ifdef DEBUG
      cerr << "cpp_arr = {";
      for (int j = 0; j < pv_words; j++) {
        cerr << cpp_arr[j];
        if (j != pv_words - 1) {
          cerr << ", ";
        }
      }
      cerr << "}" << endl;
      cerr << "pv = " << pv << endl;
      #endif
      return true;
    }
  }
  
  return false;
}

// Returns true if all elements of packed vector don't match given scalar value.
template <class pv_base_type, int pv_words, class scalar_type>
bool compare_with_scalar (
  ac_packed_vector<pv_base_type, pv_words> pv,
  const scalar_type scalar
) {
  pv_base_type scalar_ = scalar;

  for (int i = 0; i < pv_words; i++) {
    if (scalar_ != pv_base_type(pv[i])) {
      cerr << "Test FAILED. All packed vector elements are not equal to scalar input." << endl;
      #ifdef DEBUG
      cerr << "scalar = " << scalar << endl;
      cerr << "scalar_ = " << scalar_ << endl;
      cerr << "pv = " << pv << endl;
      #endif
      return true;
    }
  }

  return false;
}

// Returns true if bit slicing does not produce expected results.
template<int slc_W, class pv_type>
bool slc_test() {
  constexpr int pv_W = pv_type::width;

  for (int i = 0; i < 100; i++) {
    constexpr int lsb_max = pv_W - slc_W;

    int lsb = rand()%(lsb_max + 1);
  
    ac_int<slc_W, false> set_slc_bits;
    ac_random(set_slc_bits);

    ac_int<pv_W, false> ref_data = 0;
    pv_type pv = 0;

    ref_data.set_slc(lsb, set_slc_bits);
    pv.set_slc(lsb, set_slc_bits);

    if (pv.get_data() != ref_data) {
      cerr << "Test FAILED. set_slc() operation did not produce expected results." << endl;
      #ifdef DEBUG
      cerr << "pv.get_data() = " << pv.get_data() << endl;
      cerr << "ref_data = " << ref_data << endl;
      cerr << "pv = " << pv << endl;
      cerr << "lsb = " << lsb << endl;
      #endif
      return true;
    }

    lsb = rand()%(lsb_max + 1);
    ac_int<slc_W, false> slc_bits = pv.template slc<slc_W>(lsb);
    ac_int<slc_W, false> ref_slc_bits = ref_data.template slc<slc_W>(lsb);

    if (slc_bits != ref_slc_bits) {
      cerr << "Test FAILED. slc() operation did not produce expected results." << endl;
      #ifdef DEBUG
      cerr << "slc_bits = " << slc_bits << endl;
      cerr << "ref_slc_bits = " << ref_slc_bits << endl;
      cerr << "pv.get_data() = " << pv.get_data() << endl;
      cerr << "ref_data = " << ref_data << endl;
      cerr << "pv = " << pv << endl;
      cerr << "lsb = " << lsb << endl;
      #endif
      return true;
    }
  }

  return false;
}

template<class pv_base_type, int pv_words>
bool test_driver()
{
  typedef ac_packed_vector<pv_base_type, pv_words> pv_type;

  cout << "TEST: ac_packed_vector pv_type: ";
  cout.width(58);
  cout << left << pv_type::type_name();
  cout << "pv_words: ";
  cout.width(3);
  cout << left << pv_words;
  cout << "RESULT: ";

  pv_type pv;
  
  pv_base_type cpp_arr[pv_words]; // Standard C++ Array.
  
  for (int i = 0; i < pv_words; i++) {
    ac_random(cpp_arr[i]); // Randomly initialize C++ array.
  }
  
  pv.pack_data(cpp_arr); // Pack C++ array data into packed vector.
  
  if (compare_arrs(pv, cpp_arr)) {
    return false;
  }
  
  #ifdef DEBUG
  cout << "Comparison with C++ array after pack_data() call passed." << endl;
  #endif
  
  ac_array<pv_base_type, pv_words> ac_arr; // AC Array.
  
  for (int i = 0; i < pv_words; i++) {
    pv_base_type random_val;
    ac_random(random_val);
    ac_arr[i] = random_val;
  }
  
  pv.pack_data(ac_arr); // Pack AC array data into packed vector.
  
  if (compare_arrs(pv, ac_arr)) {
    return false;
  }
  
  #ifdef DEBUG
  cout << "Comparison with AC array after pack_data() call passed." << endl;
  #endif
  
  for (int i = 0; i < pv_words; i++) {
    pv_base_type pv_elem_val;
    ac_random(pv_elem_val);
    pv[i] = pv_elem_val; // Index random element into ac_packed_vector.
  }
  
  pv.unpack_data(cpp_arr);
  pv.unpack_data(ac_arr);
  
  if (compare_arrs(pv, cpp_arr)) {
    return false;
  }
  
  #ifdef DEBUG
  cout << "Comparison with C++ array after unpack_data() call passed." << endl;
  #endif
  
  if (compare_arrs(pv, ac_arr)) {
    return false;
  }
 
  #ifdef DEBUG
  cout << "Comparison with AC array after unpack_data() call passed." << endl;
  #endif
  
  constexpr int base_W = pv_base_type::width;

  static_assert(base_W*pv_words == pv_type::width, "Value of pv_type::width is incorrect.");

  ac_int<base_W*pv_words, false> random_data;
  ac_random(random_data);
  pv.set_data(random_data);
  
  if (pv.get_data() != random_data) {
    cerr << "set_data() + get_data() test FAILED." << endl;
    return false;
  }
  
  #ifdef DEBUG
  cout << "set_data() + get_data() test passed." << endl;
  #endif
    
  ac_int<base_W*pv_words, false> max_data;
  max_data.template set_val<AC_VAL_MAX>();
  
  pv.set_max();
  
  if (pv.get_data() != max_data) {
    cerr << "set_max() test FAILED." << endl;
    return false;
  }
  
  #ifdef DEBUG
  cout << "set_max() test passed." << endl;
  #endif
  
  pv.reset();
  
  if (pv.get_data() != 0) {
    cerr << "reset() test FAILED." << endl;
    return false;
  }
  
  #ifdef DEBUG
  cout << "reset() test passed." << endl;
  #endif
  
  if (pv.get_data() != 0) {
    cerr << "Default construction test FAILED." << endl;
    return false;
  }
  
  #ifdef DEBUG
  cout << "Default construction test passed." << endl;
  #endif
  
  pv_base_type base_scalar;
  ac_random(base_scalar);
  int int_scalar = base_scalar.to_int();
  double double_scalar = base_scalar.to_double();

  const pv_type pv_base_scalar(base_scalar);
  const pv_type pv_int_scalar(int_scalar);
  const pv_type pv_double_scalar(double_scalar);

  if (compare_with_scalar(pv_base_scalar, base_scalar)) {
    return false;
  }
  #ifdef DEBUG
  cout << "Comparison with base type scalar passed." << endl;
  #endif

  if (compare_with_scalar(pv_int_scalar, int_scalar)) {
    return false;
  }
  #ifdef DEBUG
  cout << "Comparison with integer type scalar passed." << endl;
  #endif

  if (compare_with_scalar(pv_double_scalar, double_scalar)) {
    return false;
  }
  #ifdef DEBUG
  cout << "Comparison with double type scalar passed." << endl;
  #endif
  
  for (int i = 0; i < pv_words; i++) {
    ac_random(cpp_arr[i]);
    ac_random(ac_arr[i]);
  }
  
  pv_type pv_4 = cpp_arr;
  
  if (compare_arrs(pv_4, cpp_arr)) {
    return false;
  }
  
  #ifdef DEBUG
  cout << "Comparison with C++ array after constructor call passed." << endl;
  #endif
  
  pv_type pv_5 = ac_arr;
  
  if (compare_arrs(pv_5, ac_arr)) {
    return false;
  }
   
  #ifdef DEBUG
  cout << "Comparison with AC array after constructor call passed." << endl;
  #endif
  
  const pv_type pv_6 = pv_5;
  
  for (int i = 0; i < pv_words; i++) {
    if (ac_arr[i] != pv_6[i]) {
      cerr << "Comparison of AC array with const packed vector FAILED." << endl;
      #ifdef DEBUG
      cerr << "ac_arr = " << ac_arr << endl;
      cerr << "pv_6 = " << pv_6 << endl;
      #endif
      return false;
    }
  }
  
  #ifdef DEBUG
  cout << "Comparison of AC array with const packed vector passed." << endl;
  #endif
  
  for (int i = 0; i < pv_words; i++) {
    ac_random(cpp_arr[i]);
    ac_random(ac_arr[i]);
  }
  
  pv = cpp_arr;
  
  if (compare_arrs(pv, cpp_arr)) {
    return false;
  }
  
  #ifdef DEBUG
  cout << "Comparison of C++ array with packed vector after assignment passed." << endl;
  #endif
  
  pv = ac_arr;
  
  if (compare_arrs(pv, ac_arr)) {
    return false;
  }
  
  #ifdef DEBUG
  cout << "Comparison of AC array with packed vector after assignment passed." << endl;
  #endif

  constexpr int pv_W = pv_type::width;

  static_assert(pv_W >= 6, "Packed vector must have at least 6 bits.");

  if (slc_test<pv_W/2, pv_type>()) { return false; }
  if (slc_test<pv_W, pv_type>()) { return false; }
  if (slc_test<pv_W - 1, pv_type>()) { return false; }
  if (slc_test<pv_W - 5, pv_type>()) { return false; }
  if (slc_test<1, pv_type>()) { return false; }

  cout << "PASSED." << endl;
  
  return true;
}

template <class T, int N1, int N2, int N3>
bool test_driver_nested() {
  cout << "TEST: ac_packed_vector, nested. T: ";
  cout.width(37);
  cout << left << T::type_name();
  cout << "N1: ";
  cout.width(2);
  cout << left << N1;
  cout << "N2: ";
  cout.width(2);
  cout << left << N2;
  cout << "N3: ";
  cout.width(2);
  cout << left << N3;
  cout << "RESULT: ";

  typedef ac_packed_vector<T, N3> pv1_type;
  pv1_type pv1;
  pv1.set_max();

  typedef ac_packed_vector<pv1_type, N2> pv2_type;
  pv2_type pv2;
  pv2.reset();

  typedef ac_packed_vector<pv2_type, N1> pv3_type;
  pv3_type pv3;
  pv3.reset();
  pv3_type pv3_dyn_write;

  #ifdef DEBUG
  cout << "pv1.type_name() : " << pv1.type_name() << endl;
  cout << "pv2.type_name() : " << pv2.type_name() << endl;
  cout << "pv3.type_name() : " << pv3.type_name() << endl;
  #endif

  // set_k and set_i influence when the values for pv2 and pv1 are set.
  int set_k = rand()%N1;
  int set_i = rand()%N2;

  #ifdef DEBUG
  std::cout << "set_k = " << set_k << std::endl;
  std::cout << "set_i = " << set_i << std::endl;
  #endif

  for (int k = 0; k < N1; ++k) {
    for (int i = 0; i < N2; ++i) {
      for (int j = 0; j < N3; ++j) {
        T rand_val;
        ac_random(rand_val);
        if (k == set_k && i == set_i) { pv1(j) = rand_val; }
        if (k == set_k) { pv2(i, j) = rand_val; }
        pv3(k, i, j) = rand_val;
        pv3_dyn_write.dyn_write(rand_val, k, i, j);
        T dyn_read_res;
        pv3.dyn_read(dyn_read_res, k, i, j);
        if (dyn_read_res != rand_val) {
          cerr << "Test FAILED. Dynamically reading thrice into nested packed vector did not provide expected results." << endl;
          #ifdef DEBUG
          cerr << "pv3 = " << pv3 << std::endl;
          cerr << "dyn_read_res = " << dyn_read_res << std::endl;
          #endif
          return false;
        }
      }
    }
  }
  
  #ifdef DEBUG
  cout << "Dynamically reading thrice into nested packed vector passed." << endl;
  #endif

  if (pv3_dyn_write != pv3) {
    cerr << "Test FAILED. Dynamically writing into nested packed vector by indexing thrice did not provide expected results." << endl;
    #ifdef DEBUG
    cerr << "pv1_comp = " << pv1_comp << endl;
    cerr << "pv1 = " << pv1 << endl;
    #endif
    return false;
  }
  
  #ifdef DEBUG
  cout << "Dynamically writing into nested packed vector by indexing thrice passed." << endl;
  #endif

  // The change of getting all-0s for pv3 is very low, so if it does happen, setting the elements for the 
  // vector was probably not done correctly.
  if (pv3.get_data() == 0) {
    cerr << "Test FAILED. pv3 was not correctly set and all values are still 0." << endl;
    #ifdef DEBUG
    cerr << "pv3 = " << pv3 << std::endl;
    #endif
    return false;
  }

  pv1_type pv1_comp = pv3(set_k, set_i);
  pv2_type pv2_comp = pv3(set_k);

  if (pv1_comp != pv1) {
    cerr << "Test FAILED. Indexing twice into nested packed vector did not provide expected results." << endl;
    #ifdef DEBUG
    cerr << "pv1_comp = " << pv1_comp << endl;
    cerr << "pv1 = " << pv1 << endl;
    #endif
    return false;
  }
  
  #ifdef DEBUG
  cout << "Indexing twice into nested packed vector passed." << endl;
  #endif

  if (pv2_comp != pv2) {
    cerr << "Test FAILED. Indexing once into nested packed vector did not provide expected results." << endl;
    #ifdef DEBUG
    cerr << "pv2_comp = " << pv2_comp << endl;
    cerr << "pv2 = " << pv2 << endl;
    #endif
    return false;
  }
  
  #ifdef DEBUG
  cout << "Indexing once into nested packed vector passed." << endl;
  #endif

  pv3.dyn_read(pv1_comp, set_k, set_i);

  if (pv1_comp != pv1) {
    cerr << "Test FAILED. Dynamically reading by indexing twice into nested packed vector did not provide expected results." << endl;
    #ifdef DEBUG
    cerr << "pv1_comp = " << pv1_comp << endl;
    cerr << "pv1 = " << pv1 << endl;
    #endif
    return false;
  }
  
  #ifdef DEBUG
  cout << "Dynamically reading by indexing twice into nested packed vector passed." << endl;
  #endif

  pv3.dyn_read(pv2_comp, set_k);

  if (pv2_comp != pv2) {
    cerr << "Test FAILED. Dynamically reading by indexing once into nested packed vector did not provide expected results." << endl;
    #ifdef DEBUG
    cerr << "pv2_comp = " << pv2_comp << endl;
    cerr << "pv2 = " << pv2 << endl;
    #endif
    return false;
  }
  
  #ifdef DEBUG
  cout << "Dynamically reading by indexing once into nested packed vector passed." << endl;
  #endif

  pv1_type pv1_set_by_elem;

  for (int j = 0; j < N3; ++j) {
    pv1_set_by_elem(j) = pv3(set_k, set_i, j); 
  }

  if (pv1_comp != pv1_set_by_elem) {
    cerr << "Test FAILED. Setting 1D packed vector element-wise from data in 3D packed vector did not provide expected results." << endl;
    #ifdef DEBUG
    cerr << "pv1_comp = " << pv1_comp << endl;
    cerr << "pv1 = " << pv1 << endl;
    #endif
    return false;
  }
  
  #ifdef DEBUG
  cout << "Setting 1D packed vector element-wise from data in 3D packed vector passed." << endl;
  #endif

  pv2_type pv2_set_by_elem;

  for (int i = 0; i < N2; ++i) {
    for (int j = 0; j < N3; ++j) {
      pv2_set_by_elem(i, j) = pv3(set_k, i, j); 
    }
  }

  if (pv2_comp != pv2_set_by_elem) {
    cerr << "Test FAILED. Setting 2D packed vector element-wise from data in 3D packed vector did not provide expected results." << endl;
    #ifdef DEBUG
    cerr << "pv2_comp = " << pv2_comp << endl;
    cerr << "pv2 = " << pv2 << endl;
    #endif
    return false;
  }
  
  #ifdef DEBUG
  cout << "Setting 2D packed vector element-wise from data in 3D packed vector passed." << endl;
  #endif

  pv3_type pv3_set_by_elem;

  for (int k = 0; k < N1; ++k) {
    for (int i = 0; i < N2; ++i) {
      for (int j = 0; j < N3; ++j) {
        pv3_set_by_elem(k, i, j) = pv3(k, i, j);
      }
    }
  }

  if (pv3 != pv3_set_by_elem) {
    cerr << "Test FAILED. Setting 3D packed vector element-wise from data in other 3D packed vector did not provide expected results." << endl;
    #ifdef DEBUG
    cerr << "pv3_set_by_elem = " << pv3_set_by_elem << endl;
    cerr << "pv3 = " << pv3 << endl;
    #endif
    return false;
  }
  
  #ifdef DEBUG
  cout << "Setting 3D packed vector element-wise from data in other 3D packed vector passed." << endl;
  #endif

  const pv3_type const_pv3 = pv3;

  for (int k = 0; k < N1; ++k) {
    for (int i = 0; i < N2; ++i) {
      for (int j = 0; j < N3; ++j) {
        T const_elem = const_pv3(k, i, j);
        T other_elem = pv3(k, i, j);
        if (const_elem != other_elem) {
          cerr << "Test FAILED. Indexing thrice into constant nested packed vector did not provide expected results." << endl;
          #ifdef DEBUG
          cerr << "const_pv3 = " << const_pv3 << endl;
          cerr << "pv3 = " << pv3 << endl;
          #endif
          return false;
        }
      }
    }
  }
  
  #ifdef DEBUG
  cout << "Indexing thrice into constant nested packed vector passed." << endl;
  #endif

  pv1_comp = const_pv3(set_k, set_i);
  pv2_comp = const_pv3(set_k);

  if (pv1_comp != pv1) {
    cerr << "Test FAILED. Indexing twice into constant nested packed vector did not provide expected results." << endl;
    #ifdef DEBUG
    cerr << "pv1_comp = " << pv1_comp << endl;
    cerr << "pv1 = " << pv1 << endl;
    #endif
    return false;
  }
  
  #ifdef DEBUG
  cout << "Indexing twice into constant nested packed vector passed." << endl;
  #endif

  if (pv2_comp != pv2) {
    cerr << "Test FAILED. Indexing once into constant nested packed vector did not provide expected results." << endl;
    #ifdef DEBUG
    cerr << "pv2_comp = " << pv2_comp << endl;
    cerr << "pv2 = " << pv2 << endl;
    #endif
    return false;
  }
  
  #ifdef DEBUG
  cout << "Indexing once into constant nested packed vector passed." << endl;
  #endif

  cout << "PASSED." << endl;
  
  return true;
}

int main(int argv, char **argc) {
  cout << "===================================================================================" << endl;
  cout << "--------------------- Running rtest_ac_packed_vector.cpp --------------------------" << endl;
  cout << "===================================================================================" << endl;
  
  bool all_tests_pass = true;
  
  all_tests_pass = test_driver<ac_fixed<16, 8, false>, 17>() && all_tests_pass;
  all_tests_pass = test_driver<ac_fixed<16, 8, false>, 16>() && all_tests_pass;
  all_tests_pass = test_driver<ac_fixed<16, 8, false>, 15>() && all_tests_pass;
  all_tests_pass = test_driver<ac_fixed<16, 8, false>, 10>() && all_tests_pass;
  
  all_tests_pass = test_driver<ac_fixed<32, 16, false>, 17>() && all_tests_pass;
  all_tests_pass = test_driver<ac_fixed<32, 16, false>, 16>() && all_tests_pass;
  all_tests_pass = test_driver<ac_fixed<32, 16, false>, 15>() && all_tests_pass;
  all_tests_pass = test_driver<ac_fixed<32, 16, false>, 10>() && all_tests_pass;
  
  all_tests_pass = test_driver<ac_int<16, false>, 17>() && all_tests_pass;
  all_tests_pass = test_driver<ac_int<16, false>, 16>() && all_tests_pass;
  all_tests_pass = test_driver<ac_int<16, false>, 15>() && all_tests_pass;
  all_tests_pass = test_driver<ac_int<16, false>, 10>() && all_tests_pass;

  all_tests_pass = test_driver_nested<ac_int<16, false>, 3, 5, 4>() && all_tests_pass;
  all_tests_pass = test_driver_nested<ac_fixed<32, 16, false>, 3, 5, 4>() && all_tests_pass;
  
  cout << "  Testbench finished." << endl;
  
  if (!all_tests_pass) {
    cerr << "  ac_packed_vector - FAILED" << endl;
    return -1;
  }
  
  return 0;
}
