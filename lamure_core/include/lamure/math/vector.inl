// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

namespace lamure {
namespace math {

template<typename scalar_t, const uint32_t dim>
inline const vector_t<scalar_t, dim>
operator++(vector_t<scalar_t, dim>& v, int32_t) {
  vector_t<scalar_t, dim> tmp(v);
  v += scalar_t(1);
  return (tmp);
}

template<typename scalar_t, const uint32_t dim>
inline vector_t<scalar_t, dim>&
operator++(vector_t<scalar_t, dim>& v) {
  v += scalar_t(1);
  return (v);
}

template<typename scalar_t, const uint32_t dim>
inline const vector_t<scalar_t, dim>
operator--(vector_t<scalar_t, dim>& v, int32_t) {
  vector_t<scalar_t, dim> tmp(v);
  v -= scalar_t(1);
  return (tmp);
}

template<typename scalar_t, const uint32_t dim>
inline vector_t<scalar_t, dim>&
operator--(vector_t<scalar_t, dim>& v) {
  v -= scalar_t(1);
  return (v);
}

template<typename scalar_t, const uint32_t dim>
inline const vector_t<scalar_t, dim>
operator-(const vector_t<scalar_t, dim>& rhs) {
  vector_t<scalar_t, dim> tmp(rhs);
  tmp *= scalar_t(-1);
  return (tmp);
}

template<typename scalar_t, const uint32_t dim>
inline const vector_t<scalar_t, dim>
operator+(const vector_t<scalar_t, dim>& lhs,
          const vector_t<scalar_t, dim>& rhs) {
  vector_t<scalar_t, dim> tmp(lhs);
  tmp += rhs;
  return (tmp);
}

template<typename scalar_t, const uint32_t dim>
inline const vector_t<scalar_t, dim>
operator+(const vector_t<scalar_t, dim>& lhs,
          const scalar_t rhs) {
  vector_t<scalar_t, dim> tmp(lhs);
  tmp += rhs;
  return (tmp);
}

template<typename scalar_t, const uint32_t dim>
inline const vector_t<scalar_t, dim>
operator-(const vector_t<scalar_t, dim>& lhs,
          const vector_t<scalar_t, dim>& rhs) {
  vector_t<scalar_t, dim> tmp(lhs);
  tmp -= rhs;
  return (tmp);
}

template<typename scalar_t, const uint32_t dim>
inline const vector_t<scalar_t, dim>
operator-(const vector_t<scalar_t, dim>& lhs,
          const scalar_t rhs) {
  vector_t<scalar_t, dim> tmp(lhs);
  tmp -= rhs;
  return (tmp);
}

template<typename scalar_t, const uint32_t dim>
inline const vector_t<scalar_t, dim>
operator*(const vector_t<scalar_t, dim>& lhs,
          const vector_t<scalar_t, dim>& rhs) {
  vector_t<scalar_t, dim> tmp(lhs);
  tmp *= rhs;
  return (tmp);
}

template<typename scalar_t, const uint32_t dim>
inline const vector_t<scalar_t, dim>
operator*(const scalar_t lhs,
          const vector_t<scalar_t, dim>& rhs) {
  vector_t<scalar_t, dim> tmp(rhs);
  tmp *= lhs;
  return (tmp);
}

template<typename scalar_t, const uint32_t dim>
inline const vector_t<scalar_t, dim>
operator*(const vector_t<scalar_t, dim>& lhs,
          const scalar_t rhs) {
  vector_t<scalar_t, dim> tmp(lhs);
  tmp *= rhs;
  return (tmp);
}

template<typename scalar_t, const uint32_t dim>
inline const vector_t<scalar_t, dim>
operator/(const vector_t<scalar_t, dim>& lhs,
          const vector_t<scalar_t, dim>& rhs) {
  vector_t<scalar_t, dim> tmp(lhs);
  tmp /= rhs;
  return (tmp);
}

template<typename scalar_t, const uint32_t dim>
inline const vector_t<scalar_t, dim>
operator/(const vector_t<scalar_t, dim>& lhs,
          const scalar_t rhs) {
  vector_t<scalar_t, dim> tmp(lhs);
  tmp /= rhs;
  return (tmp);
}

template<typename scalar_t, typename rhs_scal_t, const uint32_t dim>
inline const vector_t<scalar_t, dim>
operator+(const vector_t<scalar_t, dim>& lhs,
          const vector_t<rhs_scal_t, dim>& rhs) {
  vector_t<scalar_t, dim> tmp(lhs);
  tmp += rhs;
  return (tmp);
}

template<typename scalar_t, typename rhs_scal_t, const uint32_t dim>
inline const vector_t<scalar_t, dim>
operator+(const vector_t<scalar_t, dim>& lhs,
          const rhs_scal_t rhs) {
  vector_t<scalar_t, dim> tmp(lhs);
  tmp += rhs;
  return (tmp);
}

template<typename scalar_t, typename rhs_scal_t, const uint32_t dim>
inline const vector_t<scalar_t, dim>
operator-(const vector_t<scalar_t, dim>& lhs,
          const vector_t<rhs_scal_t, dim>& rhs) {
  vector_t<scalar_t, dim> tmp(lhs);
  tmp -= rhs;
  return (tmp);
}

template<typename scalar_t, typename rhs_scal_t, const uint32_t dim>
inline const vector_t<scalar_t, dim>
operator-(const vector_t<scalar_t, dim>& lhs,
          const rhs_scal_t rhs) {
  vector_t<scalar_t, dim> tmp(lhs);
  tmp -= rhs;
  return (tmp);
}

template<typename scalar_t, typename rhs_scal_t, const uint32_t dim>
inline const vector_t<scalar_t, dim>
operator*(const vector_t<scalar_t, dim>& lhs,
          const vector_t<rhs_scal_t, dim>& rhs) {
  vector_t<scalar_t, dim> tmp(lhs);
  tmp *= rhs;
  return (tmp);
}

template<typename scalar_t, typename rhs_scal_t, const uint32_t dim>
inline const vector_t<scalar_t, dim>
operator*(const rhs_scal_t lhs,
          const vector_t<scalar_t, dim>& rhs) {
  vector_t<scalar_t, dim> tmp(rhs);
  tmp *= lhs;
  return (tmp);
}

template<typename scalar_t, typename rhs_scal_t, const uint32_t dim>
inline const vector_t<scalar_t, dim>
operator*(const vector_t<scalar_t, dim>& lhs,
          const rhs_scal_t rhs) {
  vector_t<scalar_t, dim> tmp(lhs);
  tmp *= rhs;
  return (tmp);
}

template<typename scalar_t, typename rhs_scal_t, const uint32_t dim>
inline const vector_t<scalar_t, dim>
operator/(const vector_t<scalar_t, dim>&  lhs,
          const vector_t<rhs_scal_t, dim>& rhs) {
  vector_t<scalar_t, dim> tmp(lhs);
  tmp /= rhs;
  return (tmp);
}

template<typename scalar_t, typename rhs_scal_t, const uint32_t dim>
inline const vector_t<scalar_t, dim>
operator/(const vector_t<scalar_t, dim>& lhs,
          const rhs_scal_t rhs) {
  vector_t<scalar_t, dim> tmp(lhs);
  tmp /= rhs;
  return (tmp);
}

template<typename scalar_t, const uint32_t dim>
inline scalar_t length(const vector_t<scalar_t, dim>& lhs) {
  return (sqrt(dot(lhs, lhs)));
}


template<typename scalar_t, const uint32_t dim>
inline scalar_t length_sqr(const vector_t<scalar_t, dim>& lhs) {
    return (dot(lhs, lhs));
}


template<typename scalar_t, const uint32_t dim>
inline const vector_t<scalar_t, dim> normalize(const vector_t<scalar_t, dim>& lhs) {
  return (lhs / length(lhs));
}

}
}
