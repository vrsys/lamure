
namespace lamure {
namespace math {

template<typename scalar_t>
inline vector_t<scalar_t, 3>::vector_t() {

}

template<typename scalar_t>
inline vector_t<scalar_t, 3>::vector_t(const vector_t<scalar_t, 3>& v)
 : x_(v.x_), y_(v.y_), z_(v.z_) {

}

template<typename scalar_t>
inline vector_t<scalar_t, 3>::vector_t(const scalar_t s)
 : x_(s), y_(s), z_(s) {

}

template<typename scalar_t>
inline vector_t<scalar_t, 3>::vector_t(const scalar_t _x, const scalar_t _y, const scalar_t _z)
 : x_(_x), y_(_y), z_(_z) {

}

template<typename scalar_t>
template<typename rhs_scal_t>
inline vector_t<scalar_t, 3>::vector_t(const vector_t<rhs_scal_t, 3>& v)
 : x_(static_cast<scalar_t>(v.x_)),
   y_(static_cast<scalar_t>(v.y_)),
   z_(static_cast<scalar_t>(v.z_)) {

}

template<typename scalar_t>
inline vector_t<scalar_t, 3>::vector_t(const vector_t<scalar_t, 4>& _v)
 : x_(_v.x_), y_(_v.y_), z_(_v.z_) {

}

template<typename scalar_t>
inline vector_t<scalar_t, 3>&
vector_t<scalar_t, 3>::operator=(const vector_t<scalar_t, 3>& rhs) {
  x_ = rhs.x_;
  y_ = rhs.y_;
  z_ = rhs.z_;
  return (*this);
}

template<typename scalar_t>
template<typename rhs_scal_t>
inline vector_t<scalar_t, 3>&
vector_t<scalar_t, 3>::operator=(const vector_t<rhs_scal_t, 3>& rhs) {
  x_ = rhs.x_;
  y_ = rhs.y_;
  z_ = rhs.z_;
  return (*this);
}

template<typename scalar_t>
inline vector_t<scalar_t, 3>&
vector_t<scalar_t, 3>::operator+=(const scalar_t s) {
  x_ += s;
  y_ += s;
  z_ += s;
  return (*this);
}

template<typename scalar_t>
inline vector_t<scalar_t, 3>&
vector_t<scalar_t, 3>::operator+=(const vector_t<scalar_t, 3>& v) {
  x_ += v.x_;
  y_ += v.y_;
  z_ += v.z_;
  return (*this);
}

template<typename scalar_t>
inline vector_t<scalar_t, 3>&
vector_t<scalar_t, 3>::operator-=(const scalar_t s) {
  x_ -= s;
  y_ -= s;
  z_ -= s;
  return (*this);
}

template<typename scalar_t>
inline vector_t<scalar_t, 3>&
vector_t<scalar_t, 3>::operator-=(const vector_t<scalar_t, 3>& v) {
  x_ -= v.x_;
  y_ -= v.y_;
  z_ -= v.z_;
  return (*this);
}

template<typename scalar_t>
inline vector_t<scalar_t, 3>&
vector_t<scalar_t, 3>::operator*=(const scalar_t s) {
  x_ *= s;
  y_ *= s;
  z_ *= s;
  return (*this);
}

template<typename scalar_t>
inline vector_t<scalar_t, 3>&
vector_t<scalar_t, 3>::operator*=(const vector_t<scalar_t, 3>& v) {
  x_ *= v.x_;
  y_ *= v.y_;
  z_ *= v.z_;
  return (*this);
}

template<typename scalar_t>
inline vector_t<scalar_t, 3>&
vector_t<scalar_t, 3>::operator/=(const scalar_t s) {
  x_ /= s;
  y_ /= s;
  z_ /= s;
  return (*this);
}

template<typename scalar_t>
inline vector_t<scalar_t, 3>&
vector_t<scalar_t, 3>::operator/=(const vector_t<scalar_t, 3>& v) {
  x_ /= v.x_;
  y_ /= v.y_;
  z_ /= v.z_;
  return (*this);
}

template<typename scalar_t>
template<typename rhs_scal_t>
inline vector_t<scalar_t, 3>&
vector_t<scalar_t, 3>::operator+=(const rhs_scal_t s) {
  x_ += s;
  y_ += s;
  z_ += s;
  return (*this);
}

template<typename scalar_t>
template<typename rhs_scal_t>
inline vector_t<scalar_t, 3>&
vector_t<scalar_t, 3>::operator+=(const vector_t<rhs_scal_t, 3>& v) {
  x_ += v.x_;
  y_ += v.y_;
  z_ += v.z_;
  return (*this);
}

template<typename scalar_t>
template<typename rhs_scal_t>
inline vector_t<scalar_t, 3>&
vector_t<scalar_t, 3>::operator-=(const rhs_scal_t s) {
  x_ -= s;
  y_ -= s;
  z_ -= s;
  return (*this);
}

template<typename scalar_t>
template<typename rhs_scal_t>
inline vector_t<scalar_t, 3>&
vector_t<scalar_t, 3>::operator-=(const vector_t<rhs_scal_t, 3>& v) {
  x_ -= v.x_;
  y_ -= v.y_;
  z_ -= v.z_;
  return (*this);
}

template<typename scalar_t>
template<typename rhs_scal_t>
inline vector_t<scalar_t, 3>&
vector_t<scalar_t, 3>::operator*=(const rhs_scal_t s) {
  x_ *= s;
  y_ *= s;
  z_ *= s;
  return (*this);
}

template<typename scalar_t>
template<typename rhs_scal_t>
inline vector_t<scalar_t, 3>&
vector_t<scalar_t, 3>::operator*=(const vector_t<rhs_scal_t, 3>& v) {
  x_ *= v.x_;
  y_ *= v.y_;
  z_ *= v.z_;
  return (*this);
}

template<typename scalar_t>
template<typename rhs_scal_t>
inline vector_t<scalar_t, 3>&
vector_t<scalar_t, 3>::operator/=(const rhs_scal_t s) {
  x_ /= s;
  y_ /= s;
  z_ /= s;
  return (*this);
}

template<typename scalar_t>
template<typename rhs_scal_t>
inline vector_t<scalar_t, 3>&
vector_t<scalar_t, 3>::operator/=(const vector_t<rhs_scal_t, 3>& v) {
  x_ /= v.x_;
  y_ /= v.y_;
  z_ /= v.z_;
  return (*this);
}

template<typename scalar_t>
inline bool vector_t<scalar_t, 3>::operator==(const vector_t<scalar_t, 3>& v) const {
  return ((x_ == v.x_) && (y_ == v.y_) && (z_ == v.z_));
}

template<typename scalar_t>
inline bool vector_t<scalar_t, 3>::operator!=(const vector_t<scalar_t, 3>& v) const {
  return ((x_ != v.x_) || (y_ != v.y_) || (z_ != v.z_));
}

template<typename scalar_t>
inline scalar_t 
dot(const vector_t<scalar_t, 3>& lhs,
    const vector_t<scalar_t, 3>& rhs) {
  return (  lhs.x_ * rhs.x_
          + lhs.y_ * rhs.y_
          + lhs.z_ * rhs.z_);
}

template<typename scalar_t>
inline const vector_t<scalar_t, 3>
cross(const vector_t<scalar_t, 3>& lhs,
      const vector_t<scalar_t, 3>& rhs) {
  return (vector_t<scalar_t, 3>(lhs.y_ * rhs.z_ - lhs.z_ * rhs.y_,
                                lhs.z_ * rhs.x_ - lhs.x_ * rhs.z_,
                                lhs.x_ * rhs.y_ - lhs.y_ * rhs.x_));
}

}
}
