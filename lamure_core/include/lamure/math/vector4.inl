
namespace lamure {
namespace math {

template<typename scalar_t>
inline vector_t<scalar_t, 4>::vector_t() {

}

template<typename scalar_t>
inline vector_t<scalar_t, 4>::vector_t(const vector_t<scalar_t, 4>& v)
 : x_(v.x_), y_(v.y_), z_(v.z_), w_(v.w_) {

}

template<typename scalar_t>
inline vector_t<scalar_t, 4>::vector_t(const scalar_t s)
 : x_(s), y_(s), z_(s), w_(s) {

}

template<typename scalar_t>
inline vector_t<scalar_t, 4>::vector_t(const scalar_t _x,
                                       const scalar_t _y,
                                       const scalar_t _z,
                                       const scalar_t _w)
 : x_(_x), y_(_y), z_(_z), w_(_w) {

}

template<typename scalar_t>
inline vector_t<scalar_t, 4>::vector_t(const vector_t<scalar_t, 3>& _v,
                                       const scalar_t _s)
 : x_(_v.x_), y_(_v.y_), z_(_v.z_), w_(_s) {

}

template<typename scalar_t>
template<typename rhs_scal_t>
inline vector_t<scalar_t, 4>::vector_t(const vector_t<rhs_scal_t, 4>& v)
 : x_(static_cast<scalar_t>(v.x_)),
   y_(static_cast<scalar_t>(v.y_)),
   z_(static_cast<scalar_t>(v.z_)),
   w_(static_cast<scalar_t>(v.w_)) {

}

template<typename scalar_t>
inline vector_t<scalar_t, 4>& vector_t<scalar_t, 4>::operator=(const vector_t<scalar_t, 4>& rhs) {
  x_ = rhs.x_;
  y_ = rhs.y_;
  z_ = rhs.z_;
  w_ = rhs.w_;
  return (*this);
}

template<typename scalar_t>
template<typename rhs_scal_t>
inline vector_t<scalar_t, 4>&
vector_t<scalar_t, 4>::operator=(const vector_t<rhs_scal_t, 4>& rhs) {
  x_ = rhs.x_;
  y_ = rhs.y_;
  z_ = rhs.z_;
  w_ = rhs.w_;
  return (*this);
}

template<typename scalar_t>
inline vector_t<scalar_t, 4>&
vector_t<scalar_t, 4>::operator+=(const scalar_t s) {
  x_ += s;
  y_ += s;
  z_ += s;
  w_ += s;
  return (*this);
}

template<typename scalar_t>
inline vector_t<scalar_t, 4>&
vector_t<scalar_t, 4>::operator+=(const vector_t<scalar_t, 4>& v) {
  x_ += v.x_;
  y_ += v.y_;
  z_ += v.z_;
  w_ += v.w_;
  return (*this);
}

template<typename scalar_t>
inline vector_t<scalar_t, 4>&
vector_t<scalar_t, 4>::operator-=(const scalar_t s) {
  x_ -= s;
  y_ -= s;
  z_ -= s;
  w_ -= s;
  return (*this);
}

template<typename scalar_t>
inline vector_t<scalar_t, 4>&
vector_t<scalar_t, 4>::operator-=(const vector_t<scalar_t, 4>& v) {
  x_ -= v.x_;
  y_ -= v.y_;
  z_ -= v.z_;
  w_ -= v.w_;
  return (*this);
}

template<typename scalar_t>
inline vector_t<scalar_t, 4>&
vector_t<scalar_t, 4>::operator*=(const scalar_t s) {
  x_ *= s;
  y_ *= s;
  z_ *= s;
  w_ *= s;
  return (*this);
}

template<typename scalar_t>
inline vector_t<scalar_t, 4>&
vector_t<scalar_t, 4>::operator*=(const vector_t<scalar_t, 4>& v) {
  x_ *= v.x_;
  y_ *= v.y_;
  z_ *= v.z_;
  w_ *= v.w_;
  return (*this);
}

template<typename scalar_t>
inline vector_t<scalar_t, 4>&
vector_t<scalar_t, 4>::operator/=(const scalar_t s) {
  x_ /= s;
  y_ /= s;
  z_ /= s;
  w_ /= s;
  return (*this);
}

template<typename scalar_t>
inline vector_t<scalar_t, 4>&
vector_t<scalar_t, 4>::operator/=(const vector_t<scalar_t, 4>& v) {
  x_ /= v.x_;
  y_ /= v.y_;
  z_ /= v.z_;
  w_ /= v.w_;
  return (*this);
}

template<typename scalar_t>
template<typename rhs_scal_t>
inline vector_t<scalar_t, 4>&
vector_t<scalar_t, 4>::operator+=(const rhs_scal_t s) {
  x_ += s;
  y_ += s;
  z_ += s;
  w_ += s;
  return (*this);
}

template<typename scalar_t>
template<typename rhs_scal_t>
inline vector_t<scalar_t, 4>&
vector_t<scalar_t, 4>::operator+=(const vector_t<rhs_scal_t, 4>& v) {
  x_ += v.x_;
  y_ += v.y_;
  z_ += v.z_;
  w_ += v.w_;
  return (*this);
}

template<typename scalar_t>
template<typename rhs_scal_t>
inline vector_t<scalar_t, 4>&
vector_t<scalar_t, 4>::operator-=(const rhs_scal_t s) {
  x_ -= s;
  y_ -= s;
  z_ -= s;
  w_ -= s;
  return (*this);
}

template<typename scalar_t>
template<typename rhs_scal_t>
inline vector_t<scalar_t, 4>&
vector_t<scalar_t, 4>::operator-=(const vector_t<rhs_scal_t, 4>& v) {
  x_ -= v.x_;
  y_ -= v.y_;
  z_ -= v.z_;
  w_ -= v.w_;
  return (*this);
}

template<typename scalar_t>
template<typename rhs_scal_t>
inline vector_t<scalar_t, 4>&
vector_t<scalar_t, 4>::operator*=(const rhs_scal_t s) {
  x_ *= s;
  y_ *= s;
  z_ *= s;
  w_ *= s;
  return (*this);
}

template<typename scalar_t>
template<typename rhs_scal_t>
inline vector_t<scalar_t, 4>&
vector_t<scalar_t, 4>::operator*=(const vector_t<rhs_scal_t, 4>& v) {
  x_ *= v.x_;
  y_ *= v.y_;
  z_ *= v.z_;
  w_ *= v.w_;
  return (*this);
}

template<typename scalar_t>
template<typename rhs_scal_t>
inline vector_t<scalar_t, 4>&
vector_t<scalar_t, 4>::operator/=(const rhs_scal_t s) {
  x_ /= s;
  y_ /= s;
  z_ /= s;
  w_ /= s;
  return (*this);
}

template<typename scalar_t>
template<typename rhs_scal_t>
inline vector_t<scalar_t, 4>&
vector_t<scalar_t, 4>::operator/=(const vector_t<rhs_scal_t, 4>& v) {
  x_ /= v.x_;
  y_ /= v.y;
  z_ /= v.z_;
  w_ /= v.w_;
  return (*this);
}

template<typename scalar_t>
inline bool
vector_t<scalar_t, 4>::operator==(const vector_t<scalar_t, 4>& v) const {
  return ((x_ == v.x_) && (y_ == v.y_) && (z_ == v.z_) && (w_ == v.w_));
}

template<typename scalar_t>
inline
bool vector_t<scalar_t, 4>::operator!=(const vector_t<scalar_t, 4>& v) const {
  return ((x_ != v.x_) || (y_ != v.y_) || (z_ != v.z_) || (w_ != v.w_));
}

template<typename scalar_t>
inline scalar_t
dot(const vector_t<scalar_t, 4>& lhs,
    const vector_t<scalar_t, 4>& rhs) {
  return (  lhs.x_ * rhs.x_
          + lhs.y_ * rhs.y_
          + lhs.z_ * rhs.z_
          + lhs.w_ * rhs.w_);
}

template<typename scalar_t>
inline const vector_t<scalar_t, 4>
cross(const vector_t<scalar_t, 4>& lhs,
      const vector_t<scalar_t, 4>& rhs) {
  return (vector_t<scalar_t, 4>(lhs.y_ * rhs.z_ - lhs.z_ * rhs.y_,
                                lhs.z_ * rhs.x_ - lhs.x_ * rhs.z_,
                                lhs.x_ * rhs.y_ - lhs.y_ * rhs.x_,
                                scalar_t(0)));
}

}
}
