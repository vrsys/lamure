// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef LAMURE_MATH_VECTOR2_H_
#define LAMURE_MATH_VECTOR2_H_

#include <lamure/math/vector.h>

namespace lamure {
namespace math {

template<typename scalar_t>
class vector_t<scalar_t, 2> {

public:
  vector_t();
  vector_t(const vector_t<scalar_t, 2>& v);

  explicit vector_t(const scalar_t s);
  explicit vector_t(const scalar_t s, const scalar_t t);

  template<typename rhs_scal_t> explicit vector_t(const vector_t<rhs_scal_t, 2>& v);

  vector_t<scalar_t, 2>&              operator=(const vector_t<scalar_t, 2>& rhs);
  template<typename rhs_scal_t>
  vector_t<scalar_t, 2>&              operator=(const vector_t<rhs_scal_t, 2>& rhs);

  inline scalar_t&                    operator[](const int i)         { return data_[i]; };
  inline scalar_t                     operator[](const int i) const   { return data_[i]; };

  vector_t<scalar_t, 2>&              operator+=(const scalar_t          s);
  vector_t<scalar_t, 2>&              operator+=(const vector_t<scalar_t, 2>& v);
  vector_t<scalar_t, 2>&              operator-=(const scalar_t          s);
  vector_t<scalar_t, 2>&              operator-=(const vector_t<scalar_t, 2>& v);
  vector_t<scalar_t, 2>&              operator*=(const scalar_t          s);
  vector_t<scalar_t, 2>&              operator*=(const vector_t<scalar_t, 2>& v);
  vector_t<scalar_t, 2>&              operator/=(const scalar_t          s);
  vector_t<scalar_t, 2>&              operator/=(const vector_t<scalar_t, 2>& v);
  bool                                operator==(const vector_t<scalar_t, 2>& v) const;
  bool                                operator!=(const vector_t<scalar_t, 2>& v) const;

  template<typename rhs_scal_t>
  vector_t<scalar_t, 2>&              operator+=(const rhs_scal_t          s);
  template<typename rhs_scal_t>
  vector_t<scalar_t, 2>&              operator+=(const vector_t<rhs_scal_t, 2>& v);
  template<typename rhs_scal_t>
  vector_t<scalar_t, 2>&              operator-=(const rhs_scal_t          s);
  template<typename rhs_scal_t>
  vector_t<scalar_t, 2>&              operator-=(const vector_t<rhs_scal_t, 2>& v);
  template<typename rhs_scal_t>
  vector_t<scalar_t, 2>&              operator*=(const rhs_scal_t          s);
  template<typename rhs_scal_t>
  vector_t<scalar_t, 2>&              operator*=(const vector_t<rhs_scal_t, 2>& v);
  template<typename rhs_scal_t>
  vector_t<scalar_t, 2>&              operator/=(const rhs_scal_t          s);
  template<typename rhs_scal_t>
  vector_t<scalar_t, 2>&              operator/=(const vector_t<rhs_scal_t, 2>& v);

  union {
      struct {scalar_t x_, y_;};
      scalar_t data_[2];
  };

};

template<typename scalar_t> scalar_t dot(const vector_t<scalar_t, 2>& lhs, const vector_t<scalar_t, 2>& rhs);

}
}

#include <lamure/math/vector2.inl>

#endif
