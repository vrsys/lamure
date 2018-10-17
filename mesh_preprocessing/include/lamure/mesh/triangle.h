
#ifndef LAMURE_MESH_TRIANGLE_H_
#define LAMURE_MESH_TRIANGLE_H_

#include <lamure/mesh/plane.h>

namespace lamure {
namespace mesh {


class triangle_t {
public:
    triangle_t();
    ~triangle_t();

    triangle_t(const triangle_t& _t)
        : va_(_t.va_), vb_(_t.vb_), vc_(_t.vc_),
          n0_(_t.n0_), n1_(_t.n1_), n2_(_t.n2_),
          c0_(_t.c0_), c1_(_t.c1_), c2_(_t.c2_) {
    }

    triangle_t& operator =(const triangle_t& _t) {
        va_ = _t.va_;
        vb_ = _t.vb_;
        vc_ = _t.vc_;
        n0_ = _t.n0_;
        n1_ = _t.n1_;
        n2_ = _t.n2_;
        c0_ = _t.c0_;
        c1_ = _t.c1_;
        c2_ = _t.c2_;
        return *this;
    }

    const vec3f get_centroid() const;
    const float get_area() const;
    const vec3f get_normal() const;

    const bool is_adjacent_vertex(const vec3f& v0) const;
    const bool is_adjacent_edge(const vec3f& v0, const vec3f& v1) const;
    const bool is_adjacent_triangle(const triangle_t& t) const;
    const bool is_connected_triangle(const triangle_t& t) const;

    bool operator == (const triangle_t& r) const {
        return (va_ == r.va_ && vb_ == r.vb_ && vc_ == r.vc_);
    }

    const bool is_congruent_triangle(
        const vec3f& v0, 
        const vec3f& v1, 
        const vec3f& v2) {
        bool b[9];
        b[0] = va_ == v0;
        b[1] = va_ == v1;
        b[2] = va_ == v2;

        b[3] = vb_ == v0;
        b[4] = vb_ == v1;
        b[5] = vb_ == v2;

        b[6] = vc_ == v0;
        b[7] = vc_ == v1;
        b[8] = vc_ == v2;

        int num_true = 0;
        for (int i = 0; i < 9; ++i) {
            num_true += b[i];
        }

        return num_true > 2;
    }


    const lamure::mesh::plane_t get_plane() const {
        vec3r n = vec3r(n0_ + n1_ + n2_) * (1.0 / 3.0);
        return lamure::mesh::plane_t(n, vec3r(va_));

    }

    vec3f va_;
    vec3f n0_;
    vec2f c0_;
    vec3f vb_;
    vec3f n1_;
    vec2f c1_;
    vec3f vc_;
    vec3f n2_;
    vec2f c2_;
};


}
}

#endif
