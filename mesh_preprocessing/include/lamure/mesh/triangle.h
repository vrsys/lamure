
#ifndef LAMURE_MESH_TRIANGLE_H_
#define LAMURE_MESH_TRIANGLE_H_

#include <lamure/mesh/plane.h>
// #include <lamure/mesh/triangle_chartid.h>

namespace lamure {
namespace mesh {

struct vertex {
  vec3f pos_;
  vec3f nml_;
  vec2f tex_;
};

class triangle_t {
public:
    triangle_t();
    ~triangle_t();

    const vec3f get_centroid() const;
    const float get_area() const;
    const vec3f get_normal() const;

    const bool is_adjacent_vertex(const vec3f& v0) const;
    const bool is_adjacent_edge(const vec3f& v0, const vec3f& v1) const;
    const bool is_adjacent_triangle(const triangle_t& t) const;
    const bool is_connected_triangle(const triangle_t& t) const;

    bool operator == (const triangle_t& r) const {
        return (v0_.pos_ == r.v0_.pos_ && v1_.pos_ == r.v1_.pos_ && v2_.pos_ == r.v2_.pos_);
    }

    const bool is_congruent_triangle(
        const vec3f& v0, 
        const vec3f& v1, 
        const vec3f& v2) {
        bool b[9];
        b[0] = v0_.pos_ == v0;
        b[1] = v0_.pos_ == v1;
        b[2] = v0_.pos_ == v2;

        b[3] = v1_.pos_ == v0;
        b[4] = v1_.pos_ == v1;
        b[5] = v1_.pos_ == v2;

        b[6] = v2_.pos_ == v0;
        b[7] = v2_.pos_ == v1;
        b[8] = v2_.pos_ == v2;

        int num_true = 0;
        for (int i = 0; i < 9; ++i) {
            num_true += b[i];
        }

        return num_true > 2;
    }


    const lamure::mesh::plane_t get_plane() const {
        vec3r n = vec3r(v0_.nml_ + v1_.nml_ + v2_.nml_) * (1.0 / 3.0);
        return lamure::mesh::plane_t(n, vec3r(v0_.pos_));
    }

    const vertex getVertex(const int vertex_id) const {
        switch (vertex_id){
            case 0: return v0_;
            break;
            case 1: return v1_;
            break;
            case 2: return v2_;
            break;
            default:
            break;
        }
        vertex v;
        return v;
    }

    vertex v0_;
    vertex v1_;
    vertex v2_;

};


}
}

#endif
