
#ifndef LAMURE_MATH_MATRIX_H_
#define LAMURE_MATH_MATRIX_H_

#include <lamure/math/vector.h>
#include <lamure/assert.h>
#include <stdint.h>

namespace lamure {
namespace math {

template<typename scalar_t, const uint32_t row_dim, const uint32_t col_dim>
class matrix_t {
public:
  scalar_t data[row_dim * col_dim];
};


template<typename scalar_t, const uint32_t row_dim, const uint32_t col_dim> matrix_t<scalar_t, row_dim, col_dim>&      operator+=(matrix_t<scalar_t, row_dim, col_dim>& lhs, const matrix_t<scalar_t, row_dim, col_dim>& rhs);
template<typename scalar_t, const uint32_t row_dim, const uint32_t col_dim> matrix_t<scalar_t, row_dim, col_dim>&      operator-=(matrix_t<scalar_t, row_dim, col_dim>& lhs, const matrix_t<scalar_t, row_dim, col_dim>& rhs);
template<typename scalar_t, const uint32_t dim>                           matrix_t<scalar_t, dim, dim>&          operator*=(matrix_t<scalar_t, dim, dim>&     lhs, const matrix_t<scalar_t, dim, dim>&     rhs);
template<typename scalar_t, const uint32_t row_dim, const uint32_t col_dim> matrix_t<scalar_t, row_dim, col_dim>&      operator*=(matrix_t<scalar_t, row_dim, col_dim>& lhs, const scalar_t                         rhs);
template<typename scalar_t, const uint32_t row_dim, const uint32_t col_dim> matrix_t<scalar_t, row_dim, col_dim>&      operator/=(matrix_t<scalar_t, row_dim, col_dim>& lhs, const scalar_t                         rhs);
template<typename scalar_t, const uint32_t row_dim, const uint32_t col_dim> bool                                       operator==(const matrix_t<scalar_t, row_dim, col_dim>& lhs, const matrix_t<scalar_t, row_dim, col_dim>& rhs);
template<typename scalar_t, const uint32_t row_dim, const uint32_t col_dim> bool                                       operator!=(const matrix_t<scalar_t, row_dim, col_dim>& lhs, const matrix_t<scalar_t, row_dim, col_dim>& rhs);

template<typename scalar_t, const uint32_t dim>                           const matrix_t<scalar_t, dim, dim>     operator*(const matrix_t<scalar_t, dim, dim>& lhs, const matrix_t<scalar_t, dim, dim>& rhs);
template<typename scalar_t, const uint32_t row_dim, const uint32_t col_dim> const matrix_t<scalar_t, row_dim, col_dim> operator*(const matrix_t<scalar_t, row_dim, col_dim>& lhs, const scalar_t                 rhs);
template<typename scalar_t, const uint32_t row_dim, const uint32_t col_dim> const matrix_t<scalar_t, row_dim, col_dim> operator/(const matrix_t<scalar_t, row_dim, col_dim>& lhs, const scalar_t                         rhs);
template<typename scalar_t, const uint32_t row_dim, const uint32_t col_dim> const matrix_t<scalar_t, row_dim, col_dim> operator+(const matrix_t<scalar_t, row_dim, col_dim>& lhs, const matrix_t<scalar_t, row_dim, col_dim>& rhs);
template<typename scalar_t, const uint32_t row_dim, const uint32_t col_dim> const matrix_t<scalar_t, row_dim, col_dim> operator-(const matrix_t<scalar_t, row_dim, col_dim>& lhs, const matrix_t<scalar_t, row_dim, col_dim>& rhs);
template<typename scalar_t, const uint32_t dim>                           const vector_t<scalar_t, dim>            operator*(const matrix_t<scalar_t, dim, dim>& lhs, const vector_t<scalar_t, dim>&        rhs);
template<typename scalar_t, const uint32_t dim>                           const vector_t<scalar_t, dim - 1>            operator*(const matrix_t<scalar_t, dim, dim>& lhs, const vector_t<scalar_t, dim - 1>&    rhs);
template<typename scalar_t, const uint32_t dim>                           const vector_t<scalar_t, dim>            operator*(const vector_t<scalar_t, dim>&        lhs, const matrix_t<scalar_t, dim, dim>& rhs);

template<typename scalar_t, const uint32_t dim>                           const matrix_t<scalar_t, dim, dim>     identity();
template<typename scalar_t, const uint32_t row_dim, const uint32_t col_dim> const matrix_t<scalar_t, row_dim, col_dim> transpose(const matrix_t<scalar_t, row_dim, col_dim>& lhs);
template<typename scalar_t, const uint32_t dim>                           const matrix_t<scalar_t, dim, dim>     inverse(const matrix_t<scalar_t, dim, dim>& lhs);

}
}

#include <lamure/math/matrix.inl>

#endif
