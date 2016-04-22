
#include <lamure/util/frustum.h>

namespace lamure {
namespace util {

frustum_t::frustum_t(const mat4r_t& _mvp) 
: planes_(6) {
  update(_mvp);
}

frustum_t::~frustum_t() {

}

const lamure::math::plane_t& frustum_t::
get_plane(uint32_t _p) const {
  LAMURE_ASSERT(_p < 6, "frustum has 6 planes");
  return planes_[_p];
}

void frustum_t::
transform(const mat4r_t& _t) {
  for (uint32_t i = 0; i < 6; ++i) {
    planes_[i].transform(_t);
  }
}

frustum_t::classification_result_t frustum_t::
classify(const lamure::math::bounding_box_t& _box) const {
  
  //normals of planes point inward
  for (uint32_t i = 0; i < 6; ++i) {
    lamure::math::plane_t::classification_result_t min_result = planes_[i].classify(_box.min());
    lamure::math::plane_t::classification_result_t max_result = planes_[i].classify(_box.max());
    if (min_result == lamure::math::plane_t::classification_result_t::BACK
      && max_result == lamure::math::plane_t::classification_result_t::BACK) {
      return classification_result_t::OUTSIDE;
    }
    else if (min_result == lamure::math::plane_t::classification_result_t::FRONT
      && max_result == lamure::math::plane_t::classification_result_t::BACK) {
      return classification_result_t::INTERSECTING;
    }
    else if (min_result == lamure::math::plane_t::classification_result_t::BACK
      && max_result == lamure::math::plane_t::classification_result_t::FRONT) {
      return classification_result_t::INTERSECTING;
    }
  }

  return classification_result_t::INSIDE;

}


frustum_t::classification_result_t frustum_t::
classify(const lamure::math::bounding_sphere_t& _sphere) const {
  LAMURE_ASSERT(false, "not implemented");
  return classification_result_t::INTERSECTING;
}

void frustum_t::update(const mat4r_t& _mvp) {

  lamure::math::plane_t p;

  // left plane
  p.a_ = _mvp.m03 + _mvp.m00;
  p.b_ = _mvp.m07 + _mvp.m04;
  p.c_ = _mvp.m11 + _mvp.m08;
  p.d_ = _mvp.m15 + _mvp.m12;
 
  planes_[plane_id_t::LEFT] = p;
 
  // right plane
  p.a_ = _mvp.m03 - _mvp.m00;
  p.b_ = _mvp.m07 - _mvp.m04;
  p.c_ = _mvp.m11 - _mvp.m08;
  p.d_ = _mvp.m15 - _mvp.m12;
 
  planes_[plane_id_t::RIGHT] = p;
 
  // bottom plane
  p.a_ = _mvp.m03 + _mvp.m01;
  p.b_ = _mvp.m07 + _mvp.m05;
  p.c_ = _mvp.m11 + _mvp.m09;
  p.d_ = _mvp.m15 + _mvp.m13;
 
  planes_[plane_id_t::BOTTOM] = p;
 
  // top plane
  p.a_ = _mvp.m03 - _mvp.m01;
  p.b_ = _mvp.m07 - _mvp.m05;
  p.c_ = _mvp.m11 - _mvp.m09;
  p.d_ = _mvp.m15 - _mvp.m13;
 
  planes_[plane_id_t::TOP] = p;
 
  // near plane
  p.a_ = _mvp.m03 + _mvp.m02;
  p.b_ = _mvp.m07 + _mvp.m06;
  p.c_ = _mvp.m11 + _mvp.m10;
  p.d_ = _mvp.m15 + _mvp.m14;
 
  planes_[plane_id_t::NEAR] = p;
 
  // far plane
  p.a_ = _mvp.m03 - _mvp.m02;
  p.b_ = _mvp.m07 - _mvp.m06;
  p.c_ = _mvp.m11 - _mvp.m10;
  p.d_ = _mvp.m15 - _mvp.m14;
 
  planes_[plane_id_t::FAR] = p;

}
 
}
} 
