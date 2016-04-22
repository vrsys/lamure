// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef LAMURE_MATH_MATRIX4_H_
#define LAMURE_MATH_MATRIX4_H_

#include <lamure/math/matrix.h>
#include <lamure/math/vector.h>

namespace lamure {
namespace math {

template<typename scalar_t>
class matrix_t<scalar_t, 4, 4> {
public:
  matrix_t();
  matrix_t(const matrix_t<scalar_t, 4, 4>& m);

  explicit matrix_t(const scalar_t a00, const scalar_t a01, const scalar_t a02, const scalar_t a03,
                    const scalar_t a04, const scalar_t a05, const scalar_t a06, const scalar_t a07,
                    const scalar_t a08, const scalar_t a09, const scalar_t a10, const scalar_t a11,
                    const scalar_t a12, const scalar_t a13, const scalar_t a14, const scalar_t a15);

  template<typename rhs_scalar_t>
  explicit matrix_t(const matrix_t<rhs_scalar_t, 4, 4>& m);
  template<typename rhs_scalar_t>
  explicit matrix_t(const rhs_scalar_t _s);

  matrix_t<scalar_t, 4, 4>& operator=(const matrix_t<scalar_t, 4, 4>& rhs);
  template<typename rhs_scalar_t>
  matrix_t<scalar_t, 4, 4>& operator=(const matrix_t<rhs_scalar_t, 4, 4>& rhs);

  //template<typename rhs_scalar_t>
  //vector_t<scalar_t, 3> operator*(const vector_t<rhs_scalar_t, 3>& rhs);

  inline scalar_t& operator[](const int i);
  inline scalar_t operator[](const int i) const;

  inline vector_t<scalar_t, 4> column(const int i) const;
  inline vector_t<scalar_t, 4> row(const int i) const;

  static matrix_t<scalar_t, 4, 4> zero();
  static matrix_t<scalar_t, 4, 4> identity();
  
  union {
    struct {
      scalar_t m00;
      scalar_t m01;
      scalar_t m02;
      scalar_t m03;
      scalar_t m04;
      scalar_t m05;
      scalar_t m06;
      scalar_t m07;
      scalar_t m08;
      scalar_t m09;
      scalar_t m10;
      scalar_t m11;
      scalar_t m12;
      scalar_t m13;
      scalar_t m14;
      scalar_t m15;
    };
    scalar_t data_[16];
  };

};



}
}

#include <lamure/math/matrix4.inl>

#endif
