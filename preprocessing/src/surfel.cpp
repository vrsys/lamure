// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/surfel.h>

#include <lamure/config.h>
#ifdef LAMURE_USE_CGAL_FOR_NNI
#include <CGAL/Simple_cartesian.h>
#include <CGAL/Plane_3.h>

#include <CGAL/Point_3.h>

#include <CGAL/intersections.h>
#include <CGAL/squared_distance_2.h>
#endif

#include <iostream>
#include <limits.h>

namespace lamure
{
namespace pre
{

const vec3r surfel::
random_point_on_surfel() const
{

    auto compute_orthogonal_vector = [](scm::math::vec3f const &n)
    {

        scm::math::vec3f u(std::numeric_limits<float>::lowest(),
                           std::numeric_limits<float>::lowest(),
                           std::numeric_limits<float>::lowest());
        if (n.z != 0.0) {
            u = scm::math::vec3f(1, 1, (-n.x - n.y) / n.z);
        }
        else if (n.y != 0.0) {
            u = scm::math::vec3f(1, (-n.x - n.z) / n.y, 1);
        }
        else {
            u = scm::math::vec3f((-n.y - n.z) / n.x, 1, 1);
        }

        return scm::math::normalize(u);
    };

    // implements the orthogonal method discussed here:
    // http://math.stackexchange.com/questions/511370/how-to-rotate-one-vector-about-another
    auto rotate_a_around_b_by_rad = [](vec3f const &a, vec3f const &b, double rad)
    {
        vec3f a_comp_parallel_b = (scm::math::dot(a, b) / scm::math::dot(b, b)) * b;
        vec3f a_comp_orthogonal_b = a - a_comp_parallel_b;

        vec3f w = scm::math::cross(b, a_comp_orthogonal_b);

        double x_1 = std::cos(rad) / scm::math::length(a_comp_orthogonal_b);
        double x_2 = std::sin(rad) / scm::math::length(w);

        vec3f a_rot_by_rad_orthogonal_b = scm::math::length(a_comp_orthogonal_b) * (x_1 * a_comp_orthogonal_b + x_2 * w);

        return a_rot_by_rad_orthogonal_b + a_comp_parallel_b;
    };

    real random_normalized_radius_extent = ((double) std::rand() / RAND_MAX);
    real random_angle_in_radians = 2.0 * M_PI * ((double) std::rand() / RAND_MAX);

    vec3f random_direction_along_surfel = scm::math::normalize(rotate_a_around_b_by_rad(compute_orthogonal_vector(normal_),
                                                                                        normal_, random_angle_in_radians));

    // see http://mathworld.wolfram.com/DiskPointPicking.html for a short discussion about
    // random sampling of disks
    return (std::sqrt(random_normalized_radius_extent) * radius_) * vec3r(random_direction_along_surfel) + pos_;
}

#ifdef LAMURE_USE_CGAL_FOR_NNI
bool surfel::
intersect(const surfel &left_surfel, const surfel &right_surfel)
{

    auto create_surfel_plane = [](const surfel &target_surfel, bool is_left)
    {

        auto compute_orthogonal_vector = [](scm::math::vec3f const &n)
        {

            scm::math::vec3f u(std::numeric_limits<float>::lowest(),
                               std::numeric_limits<float>::lowest(),
                               std::numeric_limits<float>::lowest());
            if (n.z != 0.0) {
                u = scm::math::vec3f(1, 1, (-n.x - n.y) / n.z);
            }
            else if (n.y != 0.0) {
                u = scm::math::vec3f(1, (-n.x - n.z) / n.y, 1);
            }
            else {
                u = scm::math::vec3f((-n.y - n.z) / n.x, 1, 1);
            }

            return scm::math::normalize(u);
        };

        // jitter first normal slightly to avoid numerical problems for splats on skewed a plane
        auto const &jittered_normal = scm::math::normalize(target_surfel.normal() + scm::math::vec3f(0.001 * is_left, 0.0, 0.001 * (!is_left)));
        auto const &tangent = compute_orthogonal_vector(jittered_normal);
        auto const &bitangent = scm::math::normalize(scm::math::cross(jittered_normal, tangent));

        auto const &a = target_surfel.pos();
        auto const &b = a + tangent;
        auto const &c = a + bitangent;

        CGAL::Simple_cartesian<double>::Point_3 cgal_a(a[0], a[1], a[2]);
        CGAL::Simple_cartesian<double>::Point_3 cgal_b(b[0], b[1], b[2]);
        CGAL::Simple_cartesian<double>::Point_3 cgal_c(c[0], c[1], c[2]);

        return CGAL::Simple_cartesian<double>::Plane_3(cgal_a, cgal_b, cgal_c);
    };

    // retrieve radius and center of both surfels in order to do
    // an early intersection test with the splats considered to be spheres

    auto const left_radius = left_surfel.radius();
    auto const &left_a = left_surfel.pos();

    auto const right_radius = right_surfel.radius();
    auto const &right_a = right_surfel.pos();

    auto const center_distance = scm::math::length(left_a - right_a);

    if (!(center_distance <= left_radius + right_radius)) {
        // not even the spheres did intersect - stop here
        return false;
    }

    auto const left_surfel_plane = create_surfel_plane(left_surfel, true);
    auto const right_surfel_plane = create_surfel_plane(right_surfel, false);

    CGAL::Object intersection_result = CGAL::intersection(left_surfel_plane, right_surfel_plane);

    CGAL::Simple_cartesian<double>::Line_3 intersection_line;
    CGAL::Simple_cartesian<double>::Plane_3 intersection_plane;

    if (assign(intersection_line, intersection_result)) {
        // check if the distance between the intersection line and the center of both surfels
        // is larger than the respective radii

        // use the center of the left surfel and the intersection line
        auto const &squared_distance_left_to_intersection
            = CGAL::squared_distance(CGAL::Simple_cartesian<double>::Point_3(left_a[0], left_a[1], left_a[2]), intersection_line);
        auto const &squared_distance_right_to_intersection
            = CGAL::squared_distance(CGAL::Simple_cartesian<double>::Point_3(right_a[0], right_a[1], right_a[2]), intersection_line);

        if (squared_distance_left_to_intersection <= left_radius * left_radius &&
            squared_distance_right_to_intersection <= right_radius * right_radius) {
            return true;
        }
        else {
            return false;
        }

    }
    else if (assign(intersection_plane, intersection_result)) {
        // we already checked if the spheres intersect, so their cross-sections
        // that lie in the same plane intersect as well 
        return true;
    }
    else {
        std::cout << left_a - right_a << "\n";
        return false;
    }

    return true;
}
#else
bool surfel::
intersect(const surfel &left_surfel, const surfel &right_surfel) {
#if 1
  throw std::runtime_error("to implement without CGAL");
#else
  // if surfel planes are parallel, check distance between centers
  if (scm::math::length(left_surfel.normal() - right_surfel.normal()) < std::numeric_limits<float>::epsilon() ||
      scm::math::length(left_surfel.normal() + right_surfel.normal()) < std::numeric_limits<float>::epsilon()) {
    return scm::math::length(left_surfel.pos() - right_surfel.pos()) < left_surfel.radius() + right_surfel.radius();
  }
  else { // else compute plane-plane intersection and compute closest point on line

    auto n0 = scm::math::vec3d(left_surfel.normal());
    auto n1 = scm::math::vec3d(right_surfel.normal());

    // 1. compute hessian normal form of splat planes
    auto d0 = -scm::math::dot(left_surfel.pos(), n0);
    auto d1 = -scm::math::dot(right_surfel.pos(), n1);

    // 2. compute two arbitrary points on plane-plane intersection
    auto denom_y = n0[1] * n1[0] - n0[0] * n1[1];
    auto denom_z = n0[2] * n1[0] - n0[0] * n1[2];

    scm::math::vec3d p0;
    scm::math::vec3d p1;

    if (std::fabs(denom_y) > std::numeric_limits<float>::epsilon()) {
      // choose z freely
      p0[2] = 1.0;
      p1[2] = 2.0;
      // compute corresponding y
      p0[1] = (n0[0] * d1 - n1[0] * d0) / denom_y - (p0[2] * (n0[2] * n1[0] - n0[0] * n1[2])) / denom_y;
      p1[1] = (n0[0] * d1 - n1[0] * d0) / denom_y - (p1[2] * (n0[2] * n1[0] - n0[0] * n1[2])) / denom_y;
    }
    else {
      // choose y freely
      p0[1] = 1.0;
      p1[1] = 2.0;
      // compute corresponding z
      p0[2] = (n0[0] * d1 - n1[0] * d0) / denom_z - (p0[1] * (n0[1] * n1[0] - n0[0] * n1[1])) / denom_z;
      p1[2] = (n0[0] * d1 - n1[0] * d0) / denom_z - (p1[1] * (n0[1] * n1[0] - n0[0] * n1[1])) / denom_z;
    }
    // compute x coordinate
    p0[0] = -d0 - n0[2] * p0[2] - n0[1] * p0[1];
    p1[0] = -d0 - n0[2] * p1[2] - n0[1] * p1[1];

    // r(t) := p0 + t (p1 - p0)  represents plane intersection 
    auto r = scm::math::normalize(p1 - p0);
  }


  
  



  struct ray {
    scm::math::vec3d origin;
    scm::math::vec3d direction;
  };

  // create rays that connect the surfel center 
  ray ray_right_to_left = { right_surfel.pos(), left_surfel.pos() - right_surfel.pos() };
  ray ray_left_to_right = { left_surfel.pos(), right_surfel.pos() - left_surfel.pos() };

  // projects ray into surfel planes
  auto distance_r = scm::math::dot(scm::math::vec3d(right_surfel.pos()) * left_surfel.normal());
  auto distance_l = scm::math::dot(left_surfel.pos() * right_surfel.normal());

  auto r_projected_on_l = right_surfel.pos() + distance_r * left_surfel.normal();
  auto l_projected_on_r = left_surfel.pos() + distance_l * right_surfel.normal();

  auto distance_rl = scm::math::length(r_projected_on_l - left_surfel.pos());
  auto distance_lr = scm::math::length(l_projected_on_r - right_surfel.pos());

  // to be continued
#endif

}

#endif

bool surfel::
compare_x(const surfel &left_surfel, const surfel &right_surfel)
{
    return left_surfel.pos().x < right_surfel.pos().x;
}

bool surfel::
compare_y(const surfel &left_surfel, const surfel &right_surfel)
{
    return left_surfel.pos().y < right_surfel.pos().y;
}

bool surfel::
compare_z(const surfel &left_surfel, const surfel &right_surfel)
{
    return left_surfel.pos().z < right_surfel.pos().z;
}

surfel::compare_function
surfel::compare(const uint8_t axis)
{
    assert(axis <= 2);
    compare_function comp;

    switch (axis) {
        case 0:comp = &surfel::compare_x;
            break;
        case 1:comp = &surfel::compare_y;
            break;
        case 2:comp = &surfel::compare_z;
            break;
    }
    return comp;
}

}
} // namespace lamure

