
#ifndef LAMURE_MATH_GL_MATH_H_
#define LAMURE_MATH_GL_MATH_H_

#include <lamure/math/matrix.h>
#include <lamure/math/vector.h>
#include <lamure/math/std_math.h>
#include <lamure/platform_gl.h>

namespace lamure {
namespace math {

template<typename scalar_t> void translate(matrix_t<scalar_t, 4, 4>& m, const vector_t<scalar_t, 3>& t);
template<typename scalar_t> void translate(matrix_t<scalar_t, 4, 4>& m, const scalar_t x, const scalar_t y, const scalar_t z);
template<typename scalar_t> void rotate(matrix_t<scalar_t, 4, 4>& m, const scalar_t angl, const vector_t<scalar_t, 3>& axis);
template<typename scalar_t> void rotate(matrix_t<scalar_t, 4, 4>& m, const scalar_t angl, const scalar_t axis_x, const scalar_t axis_y, const scalar_t axis_z);
template<typename scalar_t> void scale(matrix_t<scalar_t, 4, 4>& m, const vector_t<scalar_t, 3>& s);
template<typename scalar_t> void scale(matrix_t<scalar_t, 4, 4>& m, const scalar_t x, const scalar_t y, const scalar_t z);
template<typename scalar_t> void ortho_matrix(matrix_t<scalar_t, 4, 4>& m, scalar_t left, scalar_t right, scalar_t bottom, scalar_t top, scalar_t near_z, scalar_t far_z); 
template<typename scalar_t> void frustum_matrix(matrix_t<scalar_t, 4, 4>& m, scalar_t left, scalar_t right, scalar_t bottom, scalar_t top, scalar_t near_z, scalar_t far_z);
template<typename scalar_t> void perspective_matrix(matrix_t<scalar_t, 4, 4>& m, scalar_t fovy = 45, scalar_t aspect = 4.0f/3.0f, scalar_t near_z = 0.1f, scalar_t far_z  = 100);
template<typename scalar_t> void look_at_matrix(matrix_t<scalar_t, 4, 4>& m, const vector_t<scalar_t, 3>& eye, const vector_t<scalar_t, 3>& center, const vector_t<scalar_t, 3>& up);
template<typename scalar_t> void look_at_matrix_inv(matrix_t<scalar_t, 4, 4>& m, const vector_t<scalar_t, 3>& eye, const vector_t<scalar_t, 3>& center, const vector_t<scalar_t, 3>& up);

template<typename scalar_t> const matrix_t<scalar_t, 4, 4> make_translation(const vector_t<scalar_t, 3>& t);
template<typename scalar_t> const matrix_t<scalar_t, 4, 4> make_translation(const scalar_t x, const scalar_t y, const scalar_t z);
template<typename scalar_t> const matrix_t<scalar_t, 4, 4> make_rotation(const scalar_t angl, const vector_t<scalar_t, 3>& axis);
template<typename scalar_t> const matrix_t<scalar_t, 4, 4> make_rotation(const scalar_t angl, const scalar_t axis_x, const scalar_t axis_y, const scalar_t axis_z);
template<typename scalar_t> const matrix_t<scalar_t, 4, 4> make_scale(const vector_t<scalar_t, 3>& s);
template<typename scalar_t> const matrix_t<scalar_t, 4, 4> make_scale(const scalar_t x, const scalar_t y, const scalar_t z);
template<typename scalar_t> const matrix_t<scalar_t, 4, 4> make_ortho_matrix(scalar_t left, scalar_t right, scalar_t bottom, scalar_t top, scalar_t near_z, scalar_t far_z); 
template<typename scalar_t> const matrix_t<scalar_t, 4, 4> make_frustum_matrix(scalar_t left, scalar_t right, scalar_t bottom, scalar_t top, scalar_t near_z, scalar_t far_z);
template<typename scalar_t> const matrix_t<scalar_t, 4, 4> make_perspective_matrix(scalar_t fovy = 45, scalar_t aspect = 4.0f/3.0f, scalar_t near_z = 0.1f, scalar_t far_z  = 100);
template<typename scalar_t> const matrix_t<scalar_t, 4, 4> make_look_at_matrix(const vector_t<scalar_t, 3>& eye, const vector_t<scalar_t, 3>& center, const vector_t<scalar_t, 3>& up);
template<typename scalar_t> const matrix_t<scalar_t, 4, 4> make_look_at_matrix_inv(const vector_t<scalar_t, 3>& eye, const vector_t<scalar_t, 3>& center, const vector_t<scalar_t, 3>& up);

}
}

#include <lamure/math/gl_math.inl>

#endif
