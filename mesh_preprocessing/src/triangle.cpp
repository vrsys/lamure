
#include <lamure/mesh/triangle.h>

namespace lamure {
namespace mesh {


triangle_t::triangle_t()
: va_(0.0f), vb_(0.0f), vc_(0.0f),
  n0_(0.0f), n1_(0.0f), n2_(0.0f) {

}

triangle_t::~triangle_t() {


}

const vec3f triangle_t::get_centroid() const {
    return (va_ + vb_ + vc_) / 3.f;
}

const float triangle_t::get_area() const {
    return 0.5f * scm::math::length(scm::math::cross(vb_-va_, vc_-va_));
}

const vec3f triangle_t::get_normal() const {
    return scm::math::normalize(scm::math::cross(scm::math::normalize(vb_-va_), scm::math::normalize(vc_-va_)));
}

const bool triangle_t::is_adjacent_vertex(const vec3f& v0) const {
    bool b0 = v0 == va_;
    bool b1 = v0 == vb_;
    bool b2 = v0 == vc_;

    return b0 || b1 || b2;

}

const bool triangle_t::is_adjacent_edge(const vec3f& v0, const vec3f& v1) const {
    bool b0 = v0 == va_;
    bool b1 = v0 == vb_;
    bool b2 = v0 == vc_;
    bool b3 = v1 == va_;
    bool b4 = v1 == vb_;
    bool b5 = v1 == vc_;

    if (b0 && b4) return true;
    if (b0 && b5) return true;

    if (b1 && b3) return true;
    if (b1 && b5) return true;

    if (b2 && b3) return true;
    if (b2 && b4) return true;

    return false;

}


const bool triangle_t::is_adjacent_triangle(const triangle_t& t) const {
    bool b[9];
    b[0] = va_ == t.va_;
    b[1] = va_ == t.vb_;
    b[2] = va_ == t.vc_;

    b[3] = vb_ == t.va_;
    b[4] = vb_ == t.vb_;
    b[5] = vb_ == t.vc_;

    b[6] = vc_ == t.va_;
    b[7] = vc_ == t.vb_;
    b[8] = vc_ == t.vc_;

    int num_true = 0;
    for (int i = 0; i < 9; ++i) {
        num_true += b[i];
    }

    return num_true == 2;
}

const bool triangle_t::is_connected_triangle(const triangle_t& t) const {
    if (va_ == t.va_) return true;
    if (va_ == t.vb_) return true;
    if (va_ == t.vc_) return true;
    if (vb_ == t.va_) return true;
    if (vb_ == t.vb_) return true;
    if (vb_ == t.vc_) return true;
    if (vc_ == t.va_) return true;
    if (vc_ == t.vb_) return true;
    if (vc_ == t.vc_) return true;

    return false;
    
}

}
}
