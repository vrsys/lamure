
namespace lamure {
namespace math {

template<typename scalar_t>
inline void translate(matrix_t<scalar_t, 4, 4>&    m,
                      const vector_t<scalar_t, 3>& t) {
  m.m12 = m.m00 * t.x_ + m.m04 * t.y_ + m.m08 * t.z_ + m.m12;
  m.m13 = m.m01 * t.x_ + m.m05 * t.y_ + m.m09 * t.z_ + m.m13;
  m.m14 = m.m02 * t.x_ + m.m06 * t.y_ + m.m10 * t.z_ + m.m14;
  m.m15 = m.m03 * t.x_ + m.m07 * t.y_ + m.m11 * t.z_ + m.m15;
}

template<typename scalar_t>
inline void translate(matrix_t<scalar_t, 4, 4>& m,
                      const scalar_t x,
                      const scalar_t y,
                      const scalar_t z) {
  translate(m, vector_t<scalar_t, 3>(x, y, z));
}

template<typename scalar_t>
inline void rotate(matrix_t<scalar_t, 4, 4>& m,
                   const scalar_t angl,
                   const vector_t<scalar_t, 3>& axis) {
  if (length(axis) == scalar_t(0)) {
      return;
  }

  matrix_t<scalar_t, 4, 4> tmp_rot;
  scalar_t s = sin(deg2rad(angl));
  scalar_t c = cos(deg2rad(angl));
  scalar_t one_c = scalar_t(1) - c;
  scalar_t xx, yy, zz, xy, yz, zx, xs, ys, zs;

  vector_t<scalar_t, 3> axis_n = normalize(axis);

  xx = axis_n.x_ * axis_n.x_;
  yy = axis_n.y_ * axis_n.y_;
  zz = axis_n.z_ * axis_n.z_;
  xy = axis_n.x_ * axis_n.y_;
  yz = axis_n.y_ * axis_n.z_;
  zx = axis_n.z_ * axis_n.x_;
  xs = axis_n.x_ * s;
  ys = axis_n.y_ * s;
  zs = axis_n.z_ * s;

  tmp_rot.m00 = (one_c * xx) + c;
  tmp_rot.m04 = (one_c * xy) - zs;
  tmp_rot.m08 = (one_c * zx) + ys;
  tmp_rot.m12 = scalar_t(0);

  tmp_rot.m01 = (one_c * xy) + zs;
  tmp_rot.m05 = (one_c * yy) + c;
  tmp_rot.m09 = (one_c * yz) - xs;
  tmp_rot.m13 = scalar_t(0);

  tmp_rot.m02 = (one_c * zx) - ys;
  tmp_rot.m06 = (one_c * yz) + xs;
  tmp_rot.m10 = (one_c * zz) + c;
  tmp_rot.m14 = scalar_t(0);

  tmp_rot.m03 = scalar_t(0);
  tmp_rot.m07 = scalar_t(0);
  tmp_rot.m11 = scalar_t(0);
  tmp_rot.m15 = scalar_t(1);

  m *= tmp_rot;
}

template<typename scalar_t>
inline void rotate(matrix_t<scalar_t, 4, 4>& m,
                   const scalar_t angl,
                   const scalar_t axis_x,
                   const scalar_t axis_y,
                   const scalar_t axis_z) {
  rotate(m, angl, vector_t<scalar_t, 3>(axis_x, axis_y, axis_z));
}

template<typename scalar_t>
inline void scale(matrix_t<scalar_t, 4, 4>& m,
                  const vector_t<scalar_t, 3>& s) {
  m.m00 *= s.x_;
  m.m05 *= s.y_;
  m.m10 *= s.z_;
}

template<typename scalar_t>
inline void scale(matrix_t<scalar_t, 4, 4>& m,
                  const scalar_t x,
                  const scalar_t y,
                  const scalar_t z) {
  scale(m, vector_t<scalar_t, 3>(x, y, z));
}

template<typename scalar_t>
inline void
ortho_matrix(matrix_t<scalar_t, 4, 4>& m,
             scalar_t left, scalar_t right,
             scalar_t bottom, scalar_t top,
             scalar_t near_z, scalar_t far_z) {
  scalar_t A,B,C,D,E,F;

  A=-(right+left)/(right-left);
  B=-(top+bottom)/(top-bottom);
  C=-(far_z+near_z)/(far_z-near_z);

  D=-2.0f/(far_z-near_z);
  E=2.0f/(top-bottom);
  F=2.0f/(right-left);

  m = matrix_t<scalar_t, 4, 4>( F, 0, 0, 0,
                                0, E, 0, 0,
                                0, 0, D, 0,
                                A, B, C, 1);
}

template<typename scalar_t>
inline void
frustum_matrix(matrix_t<scalar_t, 4, 4>& m,
               scalar_t left, scalar_t right,
               scalar_t bottom, scalar_t top,
               scalar_t near_z, scalar_t far_z) {
  scalar_t A,B,C,D,E,F;

  A=(right+left)/(right-left);
  B=(top+bottom)/(top-bottom);
  C=-(far_z+near_z)/(far_z-near_z);
  D=-(2.0f*far_z*near_z)/(far_z-near_z);
  E=2.0f*near_z/(top-bottom);
  F=2.0f*near_z/(right-left);

  m = matrix_t<scalar_t, 4, 4>(F, 0, 0, 0,
                               0, E, 0, 0,
                               A, B, C,-1,
                               0, 0, D, 0);
}

template<typename scalar_t>
inline void
perspective_matrix(matrix_t<scalar_t, 4, 4>& m,
                   scalar_t fovy,
                   scalar_t aspect,
                   scalar_t near_z,
                   scalar_t far_z) {
  scalar_t maxy = lamure::math::tan(deg2rad(fovy * scalar_t(.5))) * near_z;
  scalar_t maxx = maxy*aspect;

  frustum_matrix(m, -maxx, maxx, -maxy, maxy, near_z, far_z);
}

template<typename scalar_t>
inline void
look_at_matrix(matrix_t<scalar_t, 4, 4>& m,
               const vector_t<scalar_t, 3>& eye,
               const vector_t<scalar_t, 3>& center,
               const vector_t<scalar_t, 3>& up) {
  vector_t<scalar_t, 3> z_axis = normalize(center - eye);
  vector_t<scalar_t, 3> y_axis = normalize(up);
  vector_t<scalar_t, 3> x_axis = normalize(cross(z_axis, y_axis));
  y_axis = normalize(cross(x_axis, z_axis));

  m.data_[0]  =  x_axis.x_;
  m.data_[1]  =  y_axis.x_;
  m.data_[2]  = -z_axis.x_;
  m.data_[3]  = scalar_t(0.0);

  m.data_[4]  =  x_axis.y_;
  m.data_[5]  =  y_axis.y_;
  m.data_[6]  = -z_axis.y_;
  m.data_[7]  = scalar_t(0.0);

  m.data_[8]  =  x_axis.z_;
  m.data_[9]  =  y_axis.z_;
  m.data_[10] = -z_axis.z_;
  m.data_[11] = scalar_t(0.0);

  m.data_[12] = scalar_t(0.0);
  m.data_[13] = scalar_t(0.0);
  m.data_[14] = scalar_t(0.0);
  m.data_[15] = scalar_t(1.0);

  translate(m, -eye);
}

template<typename scalar_t>
inline void
look_at_matrix_inv(matrix_t<scalar_t, 4, 4>& m,
                   const vector_t<scalar_t, 3>& eye,
                   const vector_t<scalar_t, 3>& center,
                   const vector_t<scalar_t, 3>& up) {
  vector_t<scalar_t, 3> z_axis = normalize(center - eye);
  vector_t<scalar_t, 3> y_axis = normalize(up);
  vector_t<scalar_t, 3> x_axis = normalize(cross(z_axis, y_axis));
  y_axis = normalize(cross(x_axis, z_axis));

  m.data_[0]  =  x_axis.x_;
  m.data_[1]  =  x_axis.y_;
  m.data_[2]  =  x_axis.z_;
  m.data_[3]  = scalar_t(0.0);

  m.data_[4]  =  y_axis.x_;
  m.data_[5]  =  y_axis.y_;
  m.data_[6]  =  y_axis.z_;
  m.data_[7]  = scalar_t(0.0);

  m.data_[8]  = -z_axis.x_;
  m.data_[9]  = -z_axis.y_;
  m.data_[10] = -z_axis.z_;
  m.data_[11] = scalar_t(0.0);

  m.data_[12] = eye.x_;
  m.data_[13] = eye.y_;
  m.data_[14] = eye.z_;
  m.data_[15] = scalar_t(1.0);
}

template<typename scalar_t>
inline const matrix_t<scalar_t, 4, 4>
make_translation(const vector_t<scalar_t, 3>& t) {
  matrix_t<scalar_t, 4, 4> ret = identity<scalar_t, 4>();
  translate(ret, t);
  return ret;
}

template<typename scalar_t>
inline const matrix_t<scalar_t, 4, 4>
make_translation(const scalar_t x, const scalar_t y, const scalar_t z) {
  matrix_t<scalar_t, 4, 4> ret = identity<scalar_t, 4>();
  translate(ret, x, y, z);
  return ret;
}

template<typename scalar_t>
inline const matrix_t<scalar_t, 4, 4>
make_rotation(const scalar_t angl, const vector_t<scalar_t, 3>& axis) {
  matrix_t<scalar_t, 4, 4> ret = identity<scalar_t, 4>();
  rotate(ret, angl, axis);
  return ret;
}

template<typename scalar_t>
inline const matrix_t<scalar_t, 4, 4>
make_rotation(const scalar_t angl, const scalar_t axis_x, const scalar_t axis_y, const scalar_t axis_z) {
  matrix_t<scalar_t, 4, 4> ret = identity<scalar_t, 4>();
  rotate(ret, angl, axis_x, axis_y, axis_z);
  return ret;
}

template<typename scalar_t>
inline const matrix_t<scalar_t, 4, 4>
make_scale(const vector_t<scalar_t, 3>& s) {
  matrix_t<scalar_t, 4, 4> ret = identity<scalar_t, 4>();
  scale(ret, s);
  return ret;
}

template<typename scalar_t>
inline const matrix_t<scalar_t, 4, 4>
make_scale(const scalar_t x, const scalar_t y, const scalar_t z) {
  matrix_t<scalar_t, 4, 4> ret = identity<scalar_t, 4>();
  scale(ret, x, y, z);
  return ret;
}

template<typename scalar_t>
inline const matrix_t<scalar_t, 4, 4>
make_ortho_matrix(scalar_t left, scalar_t right, scalar_t bottom, scalar_t top, scalar_t near_z, scalar_t far_z) {
  matrix_t<scalar_t, 4, 4> ret = identity<scalar_t, 4>();
  ortho_matrix(ret, left, right, bottom, top, near_z, far_z);
  return ret;
}

template<typename scalar_t>
inline const matrix_t<scalar_t, 4, 4>
make_frustum_matrix(scalar_t left, scalar_t right, scalar_t bottom, scalar_t top, scalar_t near_z, scalar_t far_z) {
  matrix_t<scalar_t, 4, 4> ret = identity<scalar_t, 4>();
  frustum_matrix(ret, left, right, bottom, top, near_z, far_z);
  return ret;
}

template<typename scalar_t>
inline const matrix_t<scalar_t, 4, 4>
make_perspective_matrix(scalar_t fovy, scalar_t aspect, scalar_t near_z, scalar_t far_z) {
  matrix_t<scalar_t, 4, 4> ret = identity<scalar_t, 4>();
  perspective_matrix(ret, fovy, aspect, near_z, far_z);
  return ret;
}

template<typename scalar_t>
inline const matrix_t<scalar_t, 4, 4>
make_look_at_matrix(const vector_t<scalar_t, 3>& eye, const vector_t<scalar_t, 3>& center, const vector_t<scalar_t, 3>& up) {
  matrix_t<scalar_t, 4, 4> ret = identity<scalar_t, 4>();
  look_at_matrix(ret, eye, center, up);
  return ret;
}

template<typename scalar_t>
inline const matrix_t<scalar_t, 4, 4>
make_look_at_matrix_inv(const vector_t<scalar_t, 3>& eye, const vector_t<scalar_t, 3>& center, const vector_t<scalar_t, 3>& up) {
  matrix_t<scalar_t, 4, 4> ret = identity<scalar_t, 4>();
  look_at_matrix_inv(ret, eye, center, up);
  return ret;
}

}
}
