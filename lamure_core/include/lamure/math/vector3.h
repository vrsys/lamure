// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef LAMURE_MATH_VECTOR3_H_
#define LAMURE_MATH_VECTOR3_H_

#include <lamure/math/vector.h>

namespace lamure {
namespace math {

template<typename scalar_t>
class vector_t<scalar_t, 3> {
public:
  vector_t();
  vector_t(const vector_t<scalar_t, 3>& v);

  explicit vector_t(const scalar_t s);
  explicit vector_t(const scalar_t s, const scalar_t t, const scalar_t u);
  explicit vector_t(const vector_t<scalar_t, 4>& _v);

  template<typename rhs_scal_t> explicit vector_t(const vector_t<rhs_scal_t, 3>& v);

  vector_t<scalar_t, 3>&              operator=(const vector_t<scalar_t, 3>& rhs);
  template<typename rhs_scal_t>
  vector_t<scalar_t, 3>&              operator=(const vector_t<rhs_scal_t, 3>& rhs);

  inline scalar_t&                    operator[](const int i)         { return data_[i]; };
  inline scalar_t                     operator[](const int i) const   { return data_[i]; };

  vector_t<scalar_t, 3>&              operator+=(const scalar_t          s);
  vector_t<scalar_t, 3>&              operator+=(const vector_t<scalar_t, 3>& v);
  vector_t<scalar_t, 3>&              operator-=(const scalar_t          s);
  vector_t<scalar_t, 3>&              operator-=(const vector_t<scalar_t, 3>& v);
  vector_t<scalar_t, 3>&              operator*=(const scalar_t          s);
  vector_t<scalar_t, 3>&              operator*=(const vector_t<scalar_t, 3>& v);
  vector_t<scalar_t, 3>&              operator/=(const scalar_t          s);
  vector_t<scalar_t, 3>&              operator/=(const vector_t<scalar_t, 3>& v);
  bool                                operator==(const vector_t<scalar_t, 3>& v) const;
  bool                                operator!=(const vector_t<scalar_t, 3>& v) const;

  template<typename rhs_scal_t>
  vector_t<scalar_t, 3>&              operator+=(const rhs_scal_t          s);
  template<typename rhs_scal_t>
  vector_t<scalar_t, 3>&              operator+=(const vector_t<rhs_scal_t, 3>& v);
  template<typename rhs_scal_t>
  vector_t<scalar_t, 3>&              operator-=(const rhs_scal_t          s);
  template<typename rhs_scal_t>
  vector_t<scalar_t, 3>&              operator-=(const vector_t<rhs_scal_t, 3>& v);
  template<typename rhs_scal_t>
  vector_t<scalar_t, 3>&              operator*=(const rhs_scal_t          s);
  template<typename rhs_scal_t>
  vector_t<scalar_t, 3>&              operator*=(const vector_t<rhs_scal_t, 3>& v);
  template<typename rhs_scal_t>
  vector_t<scalar_t, 3>&              operator/=(const rhs_scal_t          s);
  template<typename rhs_scal_t>
  vector_t<scalar_t, 3>&              operator/=(const vector_t<rhs_scal_t, 3>& v);

  union {
      struct {scalar_t x_, y_, z_;};
      scalar_t data_[3];
  };

};

template<typename scalar_t> scalar_t dot(const vector_t<scalar_t, 3>& lhs, const vector_t<scalar_t, 3>& rhs);
template<typename scalar_t> const vector_t<scalar_t, 3> cross(const vector_t<scalar_t, 3>& lhs, const vector_t<scalar_t, 3>& rhs);

}
}

#include <lamure/math/vector3.inl>

#endif 
