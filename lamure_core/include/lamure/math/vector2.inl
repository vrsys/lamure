// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

namespace lamure {
namespace math {

template<typename scalar_t>
inline vector_t<scalar_t, 2>::vector_t() {

}

template<typename scalar_t>
inline vector_t<scalar_t, 2>::vector_t(const vector_t<scalar_t, 2>& v)
 : x_(v.x_), y_(v.y_) {

}

template<typename scalar_t>
inline vector_t<scalar_t, 2>::vector_t(const scalar_t s)
 : x_(s), y_(s) {

}

template<typename scalar_t>
inline vector_t<scalar_t, 2>::vector_t(const scalar_t _x, const scalar_t _y)
 : x_(_x), y_(_y) {

}

template<typename scalar_t>
template<typename rhs_scal_t>
inline vector_t<scalar_t, 2>::vector_t(const vector_t<rhs_scal_t, 2>& v)
 : x_(static_cast<scalar_t>(v.x_)),
   y_(static_cast<scalar_t>(v.y_)) {

}

template<typename scalar_t>
inline vector_t<scalar_t, 2>&
vector_t<scalar_t, 2>::operator=(const vector_t<scalar_t, 2>& rhs) {
  x_ = rhs.x_;
  y_ = rhs.y_;
  return (*this);
}

template<typename scalar_t>
template<typename rhs_scal_t>
inline vector_t<scalar_t, 2>&
vector_t<scalar_t, 2>::operator=(const vector_t<rhs_scal_t, 2>& rhs) {
  x_ = rhs.x_;
  y_ = rhs.y_;
  return (*this);
}

template<typename scalar_t>
inline vector_t<scalar_t, 2>& 
vector_t<scalar_t, 2>::operator+=(const scalar_t s) {
  x_ += s;
  y_ += s;
  return (*this);
}

template<typename scalar_t>
inline vector_t<scalar_t, 2>&
vector_t<scalar_t, 2>::operator+=(const vector_t<scalar_t, 2>& v) {
  x_ += v.x_;
  y_ += v.y_;
  return (*this);
}

template<typename scalar_t>
inline vector_t<scalar_t, 2>&
vector_t<scalar_t, 2>::operator-=(const scalar_t s) {
  x_ -= s;
  y_ -= s;
  return (*this);
}

template<typename scalar_t>
inline vector_t<scalar_t, 2>&
vector_t<scalar_t, 2>::operator-=(const vector_t<scalar_t, 2>& v) {
  x_ -= v.x_;
  y_ -= v.y;
  return (*this);
}

template<typename scalar_t>
inline vector_t<scalar_t, 2>&
vector_t<scalar_t, 2>::operator*=(const scalar_t s) {
  x_ *= s;
  y_ *= s;
  return (*this);
}

template<typename scalar_t>
inline vector_t<scalar_t, 2>&
vector_t<scalar_t, 2>::operator*=(const vector_t<scalar_t, 2>& v) {
  x_ *= v.x_;
  y_ *= v.y_;
  return (*this);
}

template<typename scalar_t>
inline vector_t<scalar_t, 2>&
vector_t<scalar_t, 2>::operator/=(const scalar_t s) {
  x_ /= s;
  y_ /= s;
  return (*this);
}

template<typename scalar_t>
inline vector_t<scalar_t, 2>&
vector_t<scalar_t, 2>::operator/=(const vector_t<scalar_t, 2>& v) {
  x_ /= v.x_;
  y_ /= v.y_;
  return (*this);
}

template<typename scalar_t>
template<typename rhs_scal_t>
inline vector_t<scalar_t, 2>&
vector_t<scalar_t, 2>::operator+=(const rhs_scal_t s) {
  x_ += s;
  y_ += s;
  return (*this);
}

template<typename scalar_t>
template<typename rhs_scal_t>
inline vector_t<scalar_t, 2>&
vector_t<scalar_t, 2>::operator+=(const vector_t<rhs_scal_t, 2>& v) {
  x_ += v.x_;
  y_ += v.y_;
  return (*this);
}

template<typename scalar_t>
template<typename rhs_scal_t>
inline vector_t<scalar_t, 2>&
vector_t<scalar_t, 2>::operator-=(const rhs_scal_t s) {
  x_ -= s;
  y_ -= s;
  return (*this);
}

template<typename scalar_t>
template<typename rhs_scal_t>
inline vector_t<scalar_t, 2>&
vector_t<scalar_t, 2>::operator-=(const vector_t<rhs_scal_t, 2>& v) {
  x_ -= v.x_;
  y_ -= v.y_;
  return (*this);
}

template<typename scalar_t>
template<typename rhs_scal_t>
inline vector_t<scalar_t, 2>&
vector_t<scalar_t, 2>::operator*=(const rhs_scal_t s) {
  x_ *= s;
  y_ *= s;
  return (*this);
}

template<typename scalar_t>
template<typename rhs_scal_t>
inline vector_t<scalar_t, 2>&
vector_t<scalar_t, 2>::operator*=(const vector_t<rhs_scal_t, 2>& v) {
  x_ *= v.x_;
  y_ *= v.y_;
  return (*this);
}

template<typename scalar_t>
template<typename rhs_scal_t>
inline vector_t<scalar_t, 2>&
vector_t<scalar_t, 2>::operator/=(const rhs_scal_t s) {
  x_ /= s;
  y_ /= s;
  return (*this);
}

template<typename scalar_t>
template<typename rhs_scal_t>
inline vector_t<scalar_t, 2>&
vector_t<scalar_t, 2>::operator/=(const vector_t<rhs_scal_t, 2>& v) {
  x_ /= v.x_;
  y_ /= v.y_;
  return (*this);
}

template<typename scalar_t>
inline bool vector_t<scalar_t, 2>::operator==(const vector_t<scalar_t, 2>& v) const {
  return ((x_ == v.x_) && (y_ == v.y_));
}

template<typename scalar_t>
inline bool vector_t<scalar_t, 2>::operator!=(const vector_t<scalar_t, 2>& v) const {
  return ((x_ != v.x_) || (y_ != v.y_));
}

template<typename scalar_t>
inline scalar_t
dot(const vector_t<scalar_t, 2>& lhs,
    const vector_t<scalar_t, 2>& rhs) {
  return (  lhs.x_ * rhs.x_
          + lhs.y_ * rhs.y_);
}


}
}
