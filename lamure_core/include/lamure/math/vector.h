
#ifndef LAMURE_MATH_VECTOR_H_
#define LAMURE_MATH_VECTOR_H_

#include <lamure/assert.h>
#include <lamure/math/std_math.h>
#include <stdint.h>

namespace lamure {
namespace math {

template<typename scalar_t, const uint32_t dim>
class vector_t {
  
};

template<typename scalar_t, const uint32_t dim> const vector_t<scalar_t, dim>      operator++(vector_t<scalar_t, dim>& v, int);
template<typename scalar_t, const uint32_t dim> vector_t<scalar_t, dim>&           operator++(vector_t<scalar_t, dim>& v);

template<typename scalar_t, const uint32_t dim> const vector_t<scalar_t, dim>      operator--(vector_t<scalar_t, dim>& v, int);
template<typename scalar_t, const uint32_t dim> vector_t<scalar_t, dim>&           operator--(vector_t<scalar_t, dim>& v);

template<typename scalar_t, const uint32_t dim> const vector_t<scalar_t, dim>      operator-(const vector_t<scalar_t, dim>& rhs);

template<typename scalar_t, const uint32_t dim> const vector_t<scalar_t, dim>      operator+(const vector_t<scalar_t, dim>& lhs, const vector_t<scalar_t, dim>& rhs);
template<typename scalar_t, const uint32_t dim> const vector_t<scalar_t, dim>      operator+(const vector_t<scalar_t, dim>& lhs, const scalar_t            rhs);

template<typename scalar_t, const uint32_t dim> const vector_t<scalar_t, dim>      operator-(const vector_t<scalar_t, dim>& lhs, const vector_t<scalar_t, dim>& rhs);
template<typename scalar_t, const uint32_t dim> const vector_t<scalar_t, dim>      operator-(const vector_t<scalar_t, dim>& lhs, const scalar_t            rhs);

template<typename scalar_t, const uint32_t dim> const vector_t<scalar_t, dim>      operator*(const vector_t<scalar_t, dim>& lhs, const vector_t<scalar_t, dim>& rhs);
template<typename scalar_t, const uint32_t dim> const vector_t<scalar_t, dim>      operator*(const scalar_t            lhs, const vector_t<scalar_t, dim>& rhs);
template<typename scalar_t, const uint32_t dim> const vector_t<scalar_t, dim>      operator*(const vector_t<scalar_t, dim>& lhs, const scalar_t            rhs);

template<typename scalar_t, const uint32_t dim> const vector_t<scalar_t, dim>      operator/(const vector_t<scalar_t, dim>& lhs, const vector_t<scalar_t, dim>& rhs);
template<typename scalar_t, const uint32_t dim> const vector_t<scalar_t, dim>      operator/(const vector_t<scalar_t, dim>& lhs, const scalar_t            rhs);;

template<typename scalar_t, typename rhs_scal_t, const uint32_t dim> const vector_t<scalar_t, dim>     operator+(const vector_t<scalar_t, dim>& lhs, const vector_t<rhs_scal_t, dim>& rhs);
template<typename scalar_t, typename rhs_scal_t, const uint32_t dim> const vector_t<scalar_t, dim>     operator+(const vector_t<scalar_t, dim>& lhs, const rhs_scal_t            rhs);

template<typename scalar_t, typename rhs_scal_t, const uint32_t dim> const vector_t<scalar_t, dim>     operator-(const vector_t<scalar_t, dim>& lhs, const vector_t<rhs_scal_t, dim>& rhs);
template<typename scalar_t, typename rhs_scal_t, const uint32_t dim> const vector_t<scalar_t, dim>     operator-(const vector_t<scalar_t, dim>& lhs, const rhs_scal_t            rhs);

template<typename scalar_t, typename rhs_scal_t, const uint32_t dim> const vector_t<scalar_t, dim>     operator*(const vector_t<scalar_t, dim>& lhs, const vector_t<rhs_scal_t, dim>& rhs);
template<typename scalar_t, typename rhs_scal_t, const uint32_t dim> const vector_t<scalar_t, dim>     operator*(const rhs_scal_t           lhs, const vector_t<scalar_t, dim>&  rhs);
template<typename scalar_t, typename rhs_scal_t, const uint32_t dim> const vector_t<scalar_t, dim>     operator*(const vector_t<scalar_t, dim>& lhs, const rhs_scal_t            rhs);

template<typename scalar_t, typename rhs_scal_t, const uint32_t dim> const vector_t<scalar_t, dim>     operator/(const vector_t<scalar_t, dim>& lhs,  const vector_t<rhs_scal_t, dim>& rhs);
template<typename scalar_t, typename rhs_scal_t, const uint32_t dim> const vector_t<scalar_t, dim>     operator/(const vector_t<scalar_t, dim>& lhs,  const rhs_scal_t            rhs);

template<typename scalar_t, const uint32_t dim> scalar_t                           length(const vector_t<scalar_t, dim>& lhs);
template<typename scalar_t, const unsigned dim> scalar_t                           length_sqr(const vector_t<scalar_t, dim>& lhs);

template<typename scalar_t, const uint32_t dim> const vector_t<scalar_t, dim>      normalize(const vector_t<scalar_t, dim>& lhs);

}
}

#include <lamure/math/vector.inl>

#endif
