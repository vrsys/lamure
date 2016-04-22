// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef LAMURE_MATH_STD_MATH_H_
#define LAMURE_MATH_STD_MATH_H_

#include <cmath>
#include <limits>

namespace lamure {
namespace math {

using std::sin;
using std::asin;
using std::cos;
using std::acos;
using std::tan;
using std::atan;
using std::sqrt;
using std::log;
using std::log10;

using std::abs;
using std::fabs;
using std::floor;
using std::ceil;
using std::pow;

const double LAMURE_PI = 4.0*std::atan(1.0);
const double LAMURE_TWO_PI = 2.0*LAMURE_PI;
const double LAMURE_DEG_TO_RAD = LAMURE_PI/180.0;
const double LAMURE_RAD_TO_DEG = 180.0/LAMURE_PI;

template<typename scalar_t>
inline int32_t sign(const scalar_t val) {
  return ((val < scalar_t(0)) ? -1 : 1);
}

template<typename T> 
inline T max(const T a, const T b) {
  return ((a > b) ? a : b);
}

template<typename T>
inline T min(const T a, const T b) {
  return ((a < b) ? a : b);
}

template<typename T>
const T clamp(const T val, const T min_val, const T max_val) {
  return (val > max_val) ? max_val : (val < min_val) ? min_val : val;
}

template<typename scalar_t>
inline const scalar_t rad2deg(const scalar_t rad) {
  return (rad * scalar_t(LAMURE_RAD_TO_DEG));
}

template<typename scalar_t>
inline scalar_t deg2rad(const scalar_t deg) {
  return (deg * scalar_t(LAMURE_DEG_TO_RAD));
}

template<typename scalar_t>
inline scalar_t log2(const scalar_t val) {
  return (scalar_t)(log(val) / log(2));
}

}
}

#endif
