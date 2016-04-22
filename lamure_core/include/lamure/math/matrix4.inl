
namespace lamure {
namespace math {

template<typename scalar_t>
inline matrix_t<scalar_t, 4, 4>::matrix_t() {
}

template<typename scalar_t>
inline matrix_t<scalar_t, 4, 4>::matrix_t(const matrix_t<scalar_t, 4, 4>& m)
 : m00(m.m00), m01(m.m01), m02(m.m02), m03(m.m03),
  m04(m.m04), m05(m.m05), m06(m.m06), m07(m.m07),
  m08(m.m08), m09(m.m09), m10(m.m10), m11(m.m11),
  m12(m.m12), m13(m.m13), m14(m.m14), m15(m.m15) {
}

template<typename scalar_t>
inline matrix_t<scalar_t, 4, 4>::matrix_t(const scalar_t a00, const scalar_t a01, const scalar_t a02, const scalar_t a03,
                                          const scalar_t a04, const scalar_t a05, const scalar_t a06, const scalar_t a07,
                                          const scalar_t a08, const scalar_t a09, const scalar_t a10, const scalar_t a11,
                                          const scalar_t a12, const scalar_t a13, const scalar_t a14, const scalar_t a15)  
 : m00(a00), m01(a01), m02(a02), m03(a03),
   m04(a04), m05(a05), m06(a06), m07(a07),
   m08(a08), m09(a09), m10(a10), m11(a11),
   m12(a12), m13(a13), m14(a14), m15(a15) {
}

template<typename scalar_t>
template<typename rhs_scal_t>
inline matrix_t<scalar_t, 4, 4>::matrix_t(const matrix_t<rhs_scal_t, 4, 4>& m)
 : m00(static_cast<scalar_t>(m.m00)), m01(static_cast<scalar_t>(m.m01)), m02(static_cast<scalar_t>(m.m02)), m03(static_cast<scalar_t>(m.m03)),
   m04(static_cast<scalar_t>(m.m04)), m05(static_cast<scalar_t>(m.m05)), m06(static_cast<scalar_t>(m.m06)), m07(static_cast<scalar_t>(m.m07)),
   m08(static_cast<scalar_t>(m.m08)), m09(static_cast<scalar_t>(m.m09)), m10(static_cast<scalar_t>(m.m10)), m11(static_cast<scalar_t>(m.m11)),
   m12(static_cast<scalar_t>(m.m12)), m13(static_cast<scalar_t>(m.m13)), m14(static_cast<scalar_t>(m.m14)), m15(static_cast<scalar_t>(m.m15)) {
}


template<typename scalar_t>
template<typename rhs_scal_t>
inline matrix_t<scalar_t, 4, 4>::matrix_t(const rhs_scal_t _s)
 : m00(static_cast<scalar_t>(_s)), m01(static_cast<scalar_t>(_s)), m02(static_cast<scalar_t>(_s)), m03(static_cast<scalar_t>(_s)),
   m04(static_cast<scalar_t>(_s)), m05(static_cast<scalar_t>(_s)), m06(static_cast<scalar_t>(_s)), m07(static_cast<scalar_t>(_s)),
   m08(static_cast<scalar_t>(_s)), m09(static_cast<scalar_t>(_s)), m10(static_cast<scalar_t>(_s)), m11(static_cast<scalar_t>(_s)),
   m12(static_cast<scalar_t>(_s)), m13(static_cast<scalar_t>(_s)), m14(static_cast<scalar_t>(_s)), m15(static_cast<scalar_t>(_s)) {
}


template<typename scalar_t>
inline matrix_t<scalar_t, 4, 4>& matrix_t<scalar_t, 4, 4>::operator=(const matrix_t<scalar_t, 4, 4>& rhs) {
  std::copy(rhs.data_, rhs.data_ + 16, data_);
  return (*this);
}

template<typename scalar_t>
template<typename rhs_scal_t>
inline matrix_t<scalar_t, 4, 4>& matrix_t<scalar_t, 4, 4>::operator=(const matrix_t<rhs_scal_t, 4, 4>& rhs) {
  for (uint32_t i = 0; i < 16; ++i) {
      data_[i] = rhs.data_[i];
  }
  return (*this);
}
/*
template<typename scalar_t>
template<typename rhs_scalar_t>
inline vector_t<scalar_t, 3> matrix_t<scalar_t, 4, 4>::operator*(const vector_t<rhs_scalar_t, 3>& rhs) {
  vector_t<scalar_t, 4> tmp = (*this) * vector_t<scalar_t, 4>(rhs.x_, rhs.y_, rhs.z_, 1.0);
  return vector_t<scalar_t, 3>(tmp.x_, tmp.y_, tmp.z_);
}
*/


template<typename scalar_t>
inline scalar_t& matrix_t<scalar_t, 4, 4>::operator[](const int i) {
  LAMURE_ASSERT(i < 16, "index " + lamure::util::to_string(i) + "is out of bounds");
  return (data_[i]);
}

template<typename scalar_t>
inline scalar_t  matrix_t<scalar_t, 4, 4>::operator[](const int i) const {
  LAMURE_ASSERT(i < 16, "index " + lamure::util::to_string(i) + "is out of bounds");
  return (data_[i]);
}

template<typename scalar_t>
inline vector_t<scalar_t, 4> matrix_t<scalar_t, 4, 4>::column(const int i) const {
  return (vector_t<scalar_t, 4>(data_[i * 4],
                                data_[i * 4 + 1],
                                data_[i * 4 + 2],
                                data_[i * 4 + 3]));
}

template<typename scalar_t>
inline vector_t<scalar_t, 4> matrix_t<scalar_t, 4, 4>::row(const int i) const {
  return (vector_t<scalar_t, 4>(data_[i],
                                data_[i + 4],
                                data_[i + 8],
                                data_[i + 12]));
}

template<typename scalar_t>
inline matrix_t<scalar_t, 4, 4> matrix_t<scalar_t, 4, 4>::zero() {
  matrix_t<scalar_t, 4, 4> m;
  for (uint32_t i = 0; i < 16; ++i) {
    m.data_[i] = static_cast<scalar_t>(0.0);
  }
  return m;
}

template<typename scalar_t>
inline matrix_t<scalar_t, 4, 4> matrix_t<scalar_t, 4, 4>::identity() {
  matrix_t<scalar_t, 4, 4> m =  zero();
  m.data_[0] = 1.0;
  m.data_[5] = 1.0;
  m.data_[10] = 1.0;
  m.data_[15] = 1.0;
  return m;
}


}
}
