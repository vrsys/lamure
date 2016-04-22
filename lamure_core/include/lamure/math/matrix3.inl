// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

namespace lamure {
namespace math {

template<typename scalar_t>
inline matrix_t<scalar_t, 3, 3>::matrix_t() {
}

template<typename scalar_t>
inline matrix_t<scalar_t, 3, 3>::matrix_t(const matrix_t<scalar_t, 3, 3>& m)
 : m00(m.m00), m01(m.m01), m02(m.m02), 
  m03(m.m03), m04(m.m04), m05(m.m05), 
  m06(m.m06), m07(m.m07), m08(m.m08) {
}

template<typename scalar_t>
inline matrix_t<scalar_t, 3, 3>::matrix_t(
const scalar_t a00, const scalar_t a01, const scalar_t a02, 
const scalar_t a03, const scalar_t a04, const scalar_t a05, 
const scalar_t a06, const scalar_t a07, const scalar_t a08)
 : m00(a00), m01(a01), m02(a02), 
   m03(a03), m04(a04), m05(a05), 
   m06(a06), m07(a07), m08(a08) {
}

template<typename scalar_t>
template<typename rhs_scal_t>
inline matrix_t<scalar_t, 3, 3>::matrix_t(const matrix_t<rhs_scal_t, 3, 3>& m)
 : m00(static_cast<scalar_t>(m.m00)), m01(static_cast<scalar_t>(m.m01)), m02(static_cast<scalar_t>(m.m02)), 
   m03(static_cast<scalar_t>(m.m03)), m04(static_cast<scalar_t>(m.m04)), m05(static_cast<scalar_t>(m.m05)), 
   m06(static_cast<scalar_t>(m.m06)), m07(static_cast<scalar_t>(m.m07)), m08(static_cast<scalar_t>(m.m08)) {
}


template<typename scalar_t>
template<typename rhs_scal_t>
inline matrix_t<scalar_t, 3, 3>::matrix_t(const rhs_scal_t _s)
 : m00(static_cast<scalar_t>(_s)), m01(static_cast<scalar_t>(_s)), m02(static_cast<scalar_t>(_s)), 
   m03(static_cast<scalar_t>(_s)), m04(static_cast<scalar_t>(_s)), m05(static_cast<scalar_t>(_s)), 
   m06(static_cast<scalar_t>(_s)), m07(static_cast<scalar_t>(_s)), m08(static_cast<scalar_t>(_s)) {
}


template<typename scalar_t>
inline matrix_t<scalar_t, 3, 3>& matrix_t<scalar_t, 3, 3>::operator=(const matrix_t<scalar_t, 3, 3>& rhs) {
  std::copy(rhs.data_, rhs.data_ + 9, data_);
  return (*this);
}

template<typename scalar_t>
template<typename rhs_scal_t>
inline matrix_t<scalar_t, 3, 3>& matrix_t<scalar_t, 3, 3>::operator=(const matrix_t<rhs_scal_t, 3, 3>& rhs) {
  for (uint32_t i = 0; i < 9; ++i) {
      data_[i] = rhs.data_[i];
  }
  return (*this);
}

template<typename scalar_t>
inline scalar_t& matrix_t<scalar_t, 3, 3>::operator[](const int i) {
  LAMURE_ASSERT(i < 9, "index " + lamure::util::to_string(i) + "is out of bounds");
  return (data_[i]);
}

template<typename scalar_t>
inline scalar_t  matrix_t<scalar_t, 3, 3>::operator[](const int i) const {
  LAMURE_ASSERT(i < 9, "index " + lamure::util::to_string(i) + "is out of bounds");
  return (data_[i]);
}

template<typename scalar_t>
inline vector_t<scalar_t, 3> matrix_t<scalar_t, 3, 3>::column(const int i) const {
  return (vector_t<scalar_t, 3>(data_[i * 3],
                                data_[i * 3 + 1],
                                data_[i * 3 + 2]));
}

template<typename scalar_t>
inline vector_t<scalar_t, 3> matrix_t<scalar_t, 3, 3>::row(const int i) const {
  return (vector_t<scalar_t, 3>(data_[i],
                                data_[i + 3],
                                data_[i + 6]));
}

template<typename scalar_t>
inline matrix_t<scalar_t, 3, 3> matrix_t<scalar_t, 3, 3>::zero() {
  matrix_t<scalar_t, 3, 3> m;
  for (uint32_t i = 0; i < 9; ++i) {
    m.data_[i] = static_cast<scalar_t>(0.0);
  }
  return m;
}

template<typename scalar_t>
inline matrix_t<scalar_t, 3, 3> matrix_t<scalar_t, 3, 3>::identity() {
  matrix_t<scalar_t, 3, 3> m = zero();
  m.data_[0] = 1.0;
  m.data_[4] = 1.0;
  m.data_[8] = 1.0;
  return m;
}


}
}
