
#ifndef LMR_MRM_TRIANGLE_H_INCLUDED
#define LMR_MRM_TRIANGLE_H_INCLUDED

#include <lamure/types.h>
#include <lamure/math/plane.h>

#include <cmath>
#include <limits>

namespace lamure {
namespace math {

template<typename vertex_data_t>
class triangle_base_t {
public:
    triangle_base_t();
    triangle_base_t(const vec3r_t& _v0, const vec3r_t& _v1, const vec3r_t& _v2);
    triangle_base_t(const triangle_base_t& _t);
    ~triangle_base_t();

    triangle_base_t& operator =(const triangle_base_t& _t);
    bool operator ==(const triangle_base_t& _t) const;
    bool operator !=(const triangle_base_t& _t) const;

    vec3r_t get_centroid() const;
    float get_area() const;
    vec3r_t get_normal() const;
    plane_t get_plane() const;

    bool is_adjacent_vertex(const vec3r_t& _v) const;
    bool is_adjacent_edge(const vec3r_t& _v0, const vec3r_t& _v1) const;
    bool is_adjacent(const triangle_base_t& _t) const;
    bool is_congruent(const triangle_base_t& _t) const;


protected:
    vertex_data_t data_;


};

}
}

#include <lamure/math/triangle.h>

namespace lamure {
namespace math {

struct vertex_data_t {
  vec3r_t va_;
  vec3r_t vb_;
  vec3r_t vc_;
};
typedef triangle_base_t<vertex_data_t> triangle_t;

}
}

#endif
