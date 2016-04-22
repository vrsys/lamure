
#ifndef LAMURE_MATH_H_
#define LAMURE_MATH_H_

#include <lamure/math/vector.h>
#include <lamure/math/vector2.h>
#include <lamure/math/vector3.h>
#include <lamure/math/vector4.h>

#include <lamure/math/matrix.h>
#include <lamure/math/matrix3.h>
#include <lamure/math/matrix4.h>

#include <lamure/math/std_math.h>

namespace lamure {
namespace math {

typedef vector_t<float, 2> vec2f_t;
typedef vector_t<double, 2> vec2d_t;
typedef vector_t<float, 3> vec3f_t;
typedef vector_t<double, 3> vec3d_t;
typedef vector_t<float, 4> vec4f_t;
typedef vector_t<double, 4> vec4d_t;

typedef vector_t<char, 3> vec3c_t;

typedef matrix_t<float, 4, 4> mat4f_t;
typedef matrix_t<double, 4, 4> mat4d_t;
typedef matrix_t<float, 3, 3> mat3f_t;
typedef matrix_t<double, 3, 3> mat3d_t;

}
}

#endif
