
#include <lamure/mesh/plane.h>
#include <iostream>

namespace lamure
{
namespace mesh
{
plane_t::plane_t() : a_(0.0f), b_(0.0f), c_(0.0f), d_(0.0f), origin_(vec3r(0.f, 0.f, 0.f)) {}

plane_t::plane_t(const vec3r& _normal, const vec3r& _origin) : origin_(_origin)
{
    vec3r n = vec3r(scm::math::normalize(_normal));
    a_ = n.x;
    b_ = n.y;
    c_ = n.z;
    d_ = -scm::math::dot(_origin, n);
}

vec3r plane_t::get_normal() const { return vec3r(a_, b_, c_); }

vec3r plane_t::get_origin() const { return origin_; }

real plane_t::signed_distance(const plane_t& _p, const vec3r& _v) { return std::abs(_p.a_ * _v.x + _p.b_ * _v.y + _p.c_ * _v.z + _p.d_) / sqrt(_p.a_ * _p.a_ + _p.b_ * _p.b_ + _p.c_ * _p.c_); }

vec3r plane_t::get_right() const
{
    vec3r normal = get_normal();
    vec3r up = vec3r(0.0f, 1.0f, 0.0f);
    vec3r right = vec3r(1.0f, 0.0f, 0.0f);
    if(normal != up)
    {
        right = scm::math::normalize(scm::math::cross(normal, up));
    }
    return right;
}

vec3r plane_t::get_up() const { return scm::math::cross(get_normal(), get_right()); }

vec3r plane_t::get_point_on_plane(vec2r const& plane_coords) const { return origin_ + vec3r(get_right()) * plane_coords[0] + vec3r(get_up()) * plane_coords[1]; }

void plane_t::transform(const mat4r& _t)
{
    mat4r it = scm::math::transpose(scm::math::inverse(_t));
    vec3r n = scm::math::normalize(it * get_normal());
    a_ = n.x;
    b_ = n.y;
    c_ = n.z;
}

plane_t::classification_result_t plane_t::classify(const vec3r& _p) const
{
    double d = (a_ * _p.x + b_ * _p.y + c_ * _p.z + d_);
    double epsilon = -10.0;
    if(d > epsilon)
    {
        return classification_result_t::FRONT;
    }
    else if(d < epsilon)
    {
        return classification_result_t::BACK;
    }

    return classification_result_t::FRONT;
}

vec2r plane_t::project(const plane_t& _p, const vec3r& right, const vec3r& _v)
{
    real s = scm::math::dot(_v - _p.origin_, right);
    real t = scm::math::dot(_v - _p.origin_, _p.get_up());
    return vec2r(s, t);
}

vec2r plane_t::project(const plane_t& _p, const vec3r& right, const vec3r& up, const vec3r& _v)
{
    real s = scm::math::dot(_v - _p.origin_, right);
    real t = scm::math::dot(_v - _p.origin_, up);
    return vec2r(s, t);
}

} // namespace mesh
} // namespace lamure
