
#ifndef LAMURE_MESH_PLANE_H_
#define LAMURE_MESH_PLANE_H_

#include <lamure/types.h>
#include <scm/core/math.h>

#include <complex>
#include <vector>

namespace lamure {
namespace mesh {

class plane_t {
public:
    plane_t();
    plane_t(const vec3r& _normal, const vec3r& _origin);

    enum classification_result_t {
      FRONT,
      BACK,
      INSIDE
    };

    vec3r get_normal() const;
    vec3r get_origin() const;
    vec3r get_right() const;
    vec3r get_up() const;

    vec3r get_point_on_plane(vec2r const& plane_coords) const;

    void transform(const mat4r& _t);
    classification_result_t classify(const vec3r& _p) const;

    static real signed_distance(const plane_t& _p, const vec3r& _v);
    static vec2r project(const plane_t& _p, const vec3r& _right, const vec3r& _v);
    static vec2r project(const plane_t& _p, const vec3r& right, const vec3r& up, const vec3r& _v);

    real a_;
    real b_;
    real c_;
    real d_;

    vec3r origin_;
};

} // namespace mesh
} // namespace lamure

#endif
