
#include <lamure/mesh/triangle.h>

namespace lamure {
namespace mesh {


triangle_t::triangle_t()
: v0_{vec3f(0.0f), vec3f(0.0f), vec2f(0.f)},
  v1_{vec3f(0.0f), vec3f(0.0f), vec2f(0.f)},
  v2_{vec3f(0.0f), vec3f(0.0f), vec2f(0.f)},
  chart_id{-1} {

}

triangle_t::~triangle_t() {


}

const vec3f triangle_t::get_centroid() const {
    return (v0_.pos_ + v1_.pos_ + v2_.pos_) / 3.f;
}

const float triangle_t::get_area() const {
    return 0.5f * scm::math::length(scm::math::cross(v1_.pos_-v0_.pos_, v2_.pos_-v0_.pos_));
}

const vec3f triangle_t::get_normal() const {
    return scm::math::normalize(scm::math::cross(scm::math::normalize(v1_.pos_-v0_.pos_), scm::math::normalize(v2_.pos_-v0_.pos_)));
}

const bool triangle_t::is_adjacent_vertex(const vec3f& v0) const {
    bool b0 = v0 == v0_.pos_;
    bool b1 = v0 == v1_.pos_;
    bool b2 = v0 == v2_.pos_;

    return b0 || b1 || b2;

}

const bool triangle_t::is_adjacent_edge(const vec3f& v0, const vec3f& v1) const {
    bool b0 = v0 == v0_.pos_;
    bool b1 = v0 == v1_.pos_;
    bool b2 = v0 == v2_.pos_;
    bool b3 = v1 == v0_.pos_;
    bool b4 = v1 == v1_.pos_;
    bool b5 = v1 == v2_.pos_;

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
    b[0] = v0_.pos_ == t.v0_.pos_;
    b[1] = v0_.pos_ == t.v1_.pos_;
    b[2] = v0_.pos_ == t.v2_.pos_;

    b[3] = v1_.pos_ == t.v0_.pos_;
    b[4] = v1_.pos_ == t.v1_.pos_;
    b[5] = v1_.pos_ == t.v2_.pos_;

    b[6] = v2_.pos_ == t.v0_.pos_;
    b[7] = v2_.pos_ == t.v1_.pos_;
    b[8] = v2_.pos_ == t.v2_.pos_;

    int num_true = 0;
    for (int i = 0; i < 9; ++i) {
        num_true += b[i];
    }

    return num_true == 2;
}

const bool triangle_t::is_connected_triangle(const triangle_t& t) const {
    if (v0_.pos_ == t.v0_.pos_) return true;
    if (v0_.pos_ == t.v1_.pos_) return true;
    if (v0_.pos_ == t.v2_.pos_) return true;
    if (v1_.pos_ == t.v0_.pos_) return true;
    if (v1_.pos_ == t.v1_.pos_) return true;
    if (v1_.pos_ == t.v2_.pos_) return true;
    if (v2_.pos_ == t.v0_.pos_) return true;
    if (v2_.pos_ == t.v1_.pos_) return true;
    if (v2_.pos_ == t.v2_.pos_) return true;

    return false;
    
}

}
}
