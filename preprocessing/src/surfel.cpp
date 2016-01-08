// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/surfel.h>

#include <CGAL/Simple_cartesian.h>
#include <CGAL/Plane_3.h>
#include <CGAL/Point_3.h>

#include <CGAL/intersections.h>
#include <CGAL/squared_distance_2.h>

#include <limits.h>

namespace lamure {
namespace pre
{


bool surfel::
intersect(const surfel &left_surfel, const surfel &right_surfel) {

    // retrieve radius and center of both surfels in order to do
    // an early intersection test with the splats imagined as spheres

    auto const left_radius = left_surfel.radius();
    auto const& left_a = left_surfel.pos();

    auto const right_radius = right_surfel.radius();
    auto const& right_a = right_surfel.pos();

    auto const center_distance = scm::math::length(left_a - right_a);

    if( !(center_distance <= left_radius + right_radius ) ) {
        // not even the spheres did intersect - stop here
        return false;
    }

    //**compute tangent vector to normal**//
    auto compute_orthogonal_vector = [] (scm::math::vec3f const& n){

        scm::math::vec3f  u(std::numeric_limits<float>::lowest(),
                            std::numeric_limits<float>::lowest(),
                            std::numeric_limits<float>::lowest());
        if(n.z != 0.0) {
            u = scm::math::vec3f( 1, 1, (-n.x - n.y) / n.z);
        } else if (n.y != 0.0) {
            u = scm::math::vec3f( 1, (-n.x - n.z) / n.y, 1);
        } else {
            u = scm::math::vec3f( (-n.y - n.z)/n.x, 1, 1);
        }

        return scm::math::normalize(u);
    };

    // jitter first normal slightly to avoid numerical problems for splats on skewed a plane
    auto const& left_normal = scm::math::normalize(left_surfel.normal() + scm::math::vec3f(0.001, 0.0, 0.0));
    auto const& left_tangent = compute_orthogonal_vector(left_normal);
    auto const& left_bitangent = scm::math::normalize(scm::math::cross(left_normal, left_tangent));

    auto const& left_b = left_a + left_tangent;
    auto const& left_c = left_a + left_bitangent;

    CGAL::Simple_cartesian<double>::Point_3 l_a(left_a[0], left_a[1], left_a[2]);
    CGAL::Simple_cartesian<double>::Point_3 l_b(left_b[0], left_b[1], left_b[2]);
    CGAL::Simple_cartesian<double>::Point_3 l_c(left_c[0], left_c[1], left_c[2]);
    
    CGAL::Simple_cartesian<double>::Plane_3 left_surfel_plane(l_a, l_b, l_c);

    // jitter second normal slightly to avoid numerical problems for splats on skewed a plane
    auto const& right_normal = scm::math::normalize(right_surfel.normal() + scm::math::vec3f(0.0, 0.0, 0.001));
    auto const& right_tangent = compute_orthogonal_vector(right_normal);
    auto const& right_bitangent = scm::math::normalize(scm::math::cross(right_normal, right_tangent));

    auto const& right_b = right_a + right_tangent;
    auto const& right_c = right_a + right_bitangent;

    CGAL::Simple_cartesian<double>::Point_3 r_a(right_a[0], right_a[1], right_a[2]);
    CGAL::Simple_cartesian<double>::Point_3 r_b(right_b[0], right_b[1], right_b[2]);
    CGAL::Simple_cartesian<double>::Point_3 r_c(right_c[0], right_c[1], right_c[2]);
    
    CGAL::Simple_cartesian<double>::Plane_3 right_surfel_plane(r_a, r_b, r_c);

    CGAL::Object intersection_result = CGAL::intersection(left_surfel_plane, right_surfel_plane);

    CGAL::Simple_cartesian<double>::Line_3 intersection_line;
    CGAL::Simple_cartesian<double>::Plane_3 intersection_plane;

    if (assign(intersection_line, intersection_result)) {
      // check if the distance between the intersection line and the center of both surfels
      // is larger than the respective radii

      // use the center of the left surfel and the intersection line
      auto const& squared_distance_left_to_intersection = CGAL::squared_distance(l_a, intersection_line);
      auto const& squared_distance_right_to_intersection = CGAL::squared_distance(r_a, intersection_line);

      if( squared_distance_left_to_intersection  <= left_radius * left_radius && 
          squared_distance_right_to_intersection <= right_radius * right_radius  ) {
        return true;
      } else {
        return false;
      }

    } else if (assign(intersection_plane, intersection_result)) { 
        // we already checked if the spheres intersect, so their cross-sections
        // that lie in the same plane intersect as well 
        return true;
    }  else {
        std::cout << left_a - right_a << "\n";
        return false;
    }

    return true;
}

bool surfel::
compare_x(const surfel &left_surfel, const surfel &right_surfel) {
    return left_surfel.pos().x < right_surfel.pos().x;
}

bool surfel::
compare_y(const surfel &left_surfel, const surfel &right_surfel) {
    return left_surfel.pos().y < right_surfel.pos().y;
}

bool surfel::
compare_z(const surfel &left_surfel, const surfel &right_surfel) {
    return left_surfel.pos().z < right_surfel.pos().z;
}

surfel::compare_function
surfel::compare(const uint8_t axis) {
    assert(axis <= 2);
    compare_function comp;

    switch (axis) {
        case 0:
            comp = &surfel::compare_x; break;
        case 1:
            comp = &surfel::compare_y; break;
        case 2:
            comp = &surfel::compare_z; break;
    }
    return comp;
}

}} // namespace lamure

