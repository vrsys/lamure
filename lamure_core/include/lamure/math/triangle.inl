
namespace lamure {
namespace math {

template<typename vertex_data_t>
triangle_base_t<vertex_data_t>::triangle_base_t()
: data_{ {0.0}, {0.0}, {0.0} } {

}

template<typename vertex_data_t>
triangle_base_t<vertex_data_t>::triangle_base_t(const vec3r_t& _v0, const vec3r_t& _v1, const vec3r_t& _v2)
: data_{_v0, _v1, _v2} {

}

template<typename vertex_data_t>
triangle_base_t<vertex_data_t>::~triangle_base_t() {

}

template<typename vertex_data_t>
triangle_base_t<vertex_data_t>& triangle_base_t<vertex_data_t>::operator =(const triangle_base_t<vertex_data_t>& _t) {
  data_ = _t.data_;
  return *this;
}

template<typename vertex_data_t>
bool triangle_base_t<vertex_data_t>::operator ==(const triangle_base_t<vertex_data_t>& _t) const {
  return (data_.va_ == _t.data_.va_ && data_.vb_ == _t.data_.vb_ && data_.vc_ == _t.data_.vc_);
}

template<typename vertex_data_t>
bool triangle_base_t<vertex_data_t>::operator !=(const triangle_base_t<vertex_data_t>& _t) const {
  return (data_.va_ != _t.data_.va_ || data_.vb_ != _t.data_.vb_ || data_.vc_ != _t.data_.vc_);
}

template<typename vertex_data_t>
vec3r_t triangle_base_t<vertex_data_t>::get_centroid() const {
  return (data_.va_ + data_.vb_ + data_.vc_) / 3.0;
}

template<typename vertex_data_t>
float triangle_base_t<vertex_data_t>::get_area() const {
  return 0.5 * math::length(math::cross(data_.vb_-data_.va_, data_.vc_-data_.va_));
}

template<typename vertex_data_t>
vec3r_t triangle_base_t<vertex_data_t>::get_normal() const {
  return math::normalize(math::cross(math::normalize(data_.vb_-data_.va_), math::normalize(data_.vc_-data_.va_)));
}

template<typename vertex_data_t>
math::plane_t triangle_base_t<vertex_data_t>::get_plane() const {
  return math::plane_t(get_normal(), get_centroid());
}

template<typename vertex_data_t>
bool triangle_base_t<vertex_data_t>::is_adjacent_vertex(const vec3r_t& _v) const {
  bool b0 = _v == data_.va_;
  bool b1 = _v == data_.vb_;
  bool b2 = _v == data_.vc_;
  return b0 || b1 || b2; 
}

template<typename vertex_data_t>
bool triangle_base_t<vertex_data_t>::is_adjacent_edge(const vec3r_t& _v0, const vec3r_t& _v1) const {
  bool b0 = _v0 == data_.va_;
  bool b1 = _v0 == data_.vb_;
  bool b2 = _v0 == data_.vc_;
  bool b3 = _v1 == data_.va_;
  bool b4 = _v1 == data_.vb_;
  bool b5 = _v1 == data_.vc_;

  if (b0 && b4) return true;
  if (b0 && b5) return true;
  if (b1 && b3) return true;
  if (b1 && b5) return true;
  if (b2 && b3) return true;
  if (b2 && b4) return true;
  return false;

}

template<typename vertex_data_t>
bool triangle_base_t<vertex_data_t>::is_adjacent(const triangle_base_t<vertex_data_t>& _t) const {
  bool b[9];
  b[0] = data_.va_ == _t.data_.va_;
  b[1] = data_.va_ == _t.data_.vb_;
  b[2] = data_.va_ == _t.data_.vc_;
  b[3] = data_.vb_ == _t.data_.va_;
  b[4] = data_.vb_ == _t.data_.vb_;
  b[5] = data_.vb_ == _t.data_.vc_;
  b[6] = data_.vc_ == _t.data_.va_;
  b[7] = data_.vc_ == _t.data_.vb_;
  b[8] = data_.vc_ == _t.data_.vc_;
  int num_true = 0;
  for (int i = 0; i < 9; ++i) {
    num_true += b[i];
  }   
  return num_true == 2;
}

template<typename vertex_data_t>
bool triangle_base_t<vertex_data_t>::is_congruent(const triangle_base_t<vertex_data_t>& _t) const {
  bool b[9];
  b[0] = data_.va_ == _t.data_.va_;
  b[1] = data_.va_ == _t.data_.vb_;
  b[2] = data_.va_ == _t.data_.vc_;
  b[3] = data_.vb_ == _t.data_.va_;
  b[4] = data_.vb_ == _t.data_.vb_;
  b[5] = data_.vb_ == _t.data_.vc_;
  b[6] = data_.vc_ == _t.data_.va_;
  b[7] = data_.vc_ == _t.data_.vb_;
  b[8] = data_.vc_ == _t.data_.vc_;
  int num_true = 0;
  for (int i = 0; i < 9; ++i) {
    num_true += b[i];
  }
  return num_true > 2;
}

}
}
