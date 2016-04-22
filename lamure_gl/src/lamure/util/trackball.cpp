// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/util/trackball.h>

namespace lamure {
namespace util {

trackball_t::trackball_t()
 : transform_(lamure::math::mat4d_t::identity()),
   radius_(1.0),
   dolly_(0.0) {

}

trackball_t::~trackball_t() {

}

float64_t trackball_t::
project_to_sphere(float64_t x, float64_t y) const {
    float64_t len_sqr = x*x + y*y;
    float64_t len = lamure::math::sqrt(len_sqr);

    if (len < radius_ / lamure::math::sqrt(2.0)) {
        return (lamure::math::sqrt(radius_ * radius_ - len_sqr));
    } else {
        return ((radius_ * radius_) / (2.0 * len));
    }
}

void trackball_t::
rotate(float64_t fx, float64_t fy, float64_t tx, float64_t ty) {
    lamure::math::vec3d_t start(fx, fy, project_to_sphere(fx, fy));
    lamure::math::vec3d_t end(tx, ty, project_to_sphere(tx, ty));

    lamure::math::vec3d_t diff(end - start);
    float64_t diff_len = lamure::math::length(diff);

    lamure::math::vec3d_t rot_axis(cross(start, end));

    float64_t rot_angle = 2.0 * asin(lamure::math::clamp(diff_len/(2.0 * radius_), -1.0, 1.0));

    lamure::math::mat4d_t tmp(lamure::math::mat4d_t::identity());

    lamure::math::mat4d_t tmp_dolly(lamure::math::mat4d_t::identity());
    lamure::math::mat4d_t tmp_dolly_inv(lamure::math::mat4d_t::identity());
    lamure::math::translate(tmp_dolly, 0.0, 0.0, dolly_);
    lamure::math::translate(tmp_dolly_inv, 0.0, 0.0, -dolly_);

    lamure::math::rotate(tmp, lamure::math::rad2deg(rot_angle), rot_axis);

    transform_ = tmp_dolly * tmp * tmp_dolly_inv * transform_;
}


void trackball_t::
translate(float64_t x, float64_t y) {
    float64_t dolly_abs = abs(dolly_);
    float64_t near_dist = 1.0;

    lamure::math::mat4d_t tmp(lamure::math::mat4d_t::identity());

    lamure::math::translate(tmp,
              x * (near_dist + dolly_abs),
              y * (near_dist + dolly_abs),
              0.0);

    lamure::math::mat4d_t tmp_dolly(lamure::math::mat4d_t::identity());
    lamure::math::mat4d_t tmp_dolly_inv(lamure::math::mat4d_t::identity());
    lamure::math::translate(tmp_dolly, 0.0, 0.0, dolly_);
    lamure::math::translate(tmp_dolly_inv, 0.0, 0.0, -dolly_);

    transform_ = tmp_dolly * tmp * tmp_dolly_inv * transform_;
}


void trackball_t::
dolly(float64_t y) {
    lamure::math::mat4d_t tmp_dolly(lamure::math::mat4d_t::identity());
    lamure::math::mat4d_t tmp_dolly_inv(lamure::math::mat4d_t::identity());
    lamure::math::translate(tmp_dolly, 0.0, 0.0, dolly_);
    lamure::math::translate(tmp_dolly_inv, 0.0, 0.0, -dolly_);

    dolly_ -= y;
    lamure::math::mat4d_t tmp(lamure::math::mat4d_t::identity());
    lamure::math::translate(tmp, 0.0, 0.0, dolly_);

    transform_ = tmp * tmp_dolly_inv * transform_;
}



}
}
