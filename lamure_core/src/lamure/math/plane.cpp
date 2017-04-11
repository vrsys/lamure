// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/math/plane.h>
#include <iostream>

namespace lamure {
namespace math {

plane_t::plane_t()
: a_(0.0f), b_(0.0f), c_(0.0f), d_(0.0f), origin_(vec3r_t(0.f, 0.f, 0.f)) {

}

plane_t::plane_t(const vec3r_t& _normal, const vec3r_t& _origin)
: origin_(_origin) {
  vec3r_t n = vec3r_t(lamure::math::normalize(_normal));
  a_ = n.x_;
  b_ = n.y_;
  c_ = n.z_;
  d_ = -lamure::math::dot(_origin, n);
}

vec3r_t plane_t::get_normal() const {
  return vec3r_t(a_, b_, c_);
}

vec3r_t plane_t::get_origin() const {
  return origin_;
}

real_t plane_t::signed_distance(const plane_t& _p, const vec3r_t& _v) {
  return std::abs(_p.a_ * _v.x_ + _p.b_ * _v.y_ + _p.c_ * _v.z_ + _p.d_) / sqrt(_p.a_*_p.a_ + _p.b_*_p.b_ + _p.c_*_p.c_);
}

vec3r_t plane_t::get_right() const {
  vec3r_t normal = get_normal();
  vec3r_t up = vec3r_t(0.0f, 1.0f, 0.0f);
  vec3r_t right = vec3r_t(1.0f, 0.0f, 0.0f);
  if (normal != up) {
      right = lamure::math::normalize(lamure::math::cross(normal, up));
  }
  return right;
}

vec3r_t plane_t::get_up() const {
  return lamure::math::cross( get_normal(), get_right() );
}

vec3r_t plane_t::get_point_on_plane(vec2r_t const& plane_coords) const {
  return  origin_ 
        + vec3r_t(get_right()) * plane_coords[0] 
        + vec3r_t(get_up()) * plane_coords[1];
}

void plane_t::transform(const mat4r_t& _t) {
  mat4r_t it = lamure::math::transpose(lamure::math::inverse(_t));
  vec3r_t n = lamure::math::normalize(it * get_normal());
  a_ = n.x_;
  b_ = n.y_;
  c_ = n.z_;
}

plane_t::classification_result_t plane_t::classify(const vec3r_t& _p) const {
  double d = (a_ * _p.x_ + b_ * _p.y_ + c_ * _p.z_ + d_);
  double epsilon = -10.0;
  if (d > epsilon) {
    return classification_result_t::FRONT;
  }
  else if (d < epsilon) {
    return classification_result_t::BACK;
  }

  return classification_result_t::FRONT;
}

vec2r_t plane_t::project(const plane_t& _p, const vec3r_t& right, const vec3r_t& _v) {
  real_t s = lamure::math::dot(_v - _p.origin_, right);
  real_t t = lamure::math::dot(_v - _p.origin_, _p.get_up());
  return vec2r_t(s, t);
}

vec2r_t plane_t::project(const plane_t& _p, const vec3r_t& right, const vec3r_t& up, const vec3r_t& _v) {
  real_t s = lamure::math::dot(_v - _p.origin_, right);
  real_t t = lamure::math::dot(_v - _p.origin_, up);
  return vec2r_t(s, t);
}

void plane_t::fit_plane(
  std::vector<vec3r_t> const& neighbour_positions,
  plane_t& plane) {
  
  vec3r_t centroid = vec3r_t{0.0};
  for (const auto& neighbour_pos : neighbour_positions) {
      centroid += neighbour_pos;
  }
  
  centroid /= (real_t)neighbour_positions.size();

  //calc covariance matrix
  mat3r_t covariance_mat = lamure::math::mat3d_t::zero();

  for (const auto& c : neighbour_positions) {
    covariance_mat.m00 += (c[0]-centroid[0]) * (c[0]-centroid[0]);
    covariance_mat.m01 += (c[0]-centroid[0]) * (c[1] - centroid[1]);
    covariance_mat.m02 += (c[0]-centroid[0]) * (c[2] - centroid[2]);

    covariance_mat.m03 += (c[1]-centroid[1]) * (c[0] - centroid[0]);
    covariance_mat.m04 += (c[1]-centroid[1]) * (c[1]-centroid[1]);
    covariance_mat.m05 += (c[1]-centroid[1]) * (c[2] - centroid[2]);

    covariance_mat.m06 += (c[2]-centroid[2]) * (c[0] - centroid[0]);
    covariance_mat.m07 += (c[2]-centroid[2]) * (c[1] - centroid[1]);
    covariance_mat.m08 += (c[2]-centroid[2]) * (c[2]-centroid[2]);
  }

  //calculate normal
  mat3r_t inv_covariance_mat = lamure::math::inverse(covariance_mat);

  vec3r_t first = vec3r_t{1.0f};
  vec3r_t second = lamure::math::normalize(first*inv_covariance_mat);

  unsigned int iteration = 0;
  while (iteration++ < 512 && first != second) {
      first = second;
      second = lamure::math::normalize(first*inv_covariance_mat);
  }
  
  plane = plane_t{second, centroid};
}

} // namespace math
} // namespace lamure
