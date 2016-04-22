// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/xyz/surfel.h>

#include <CGAL/Simple_cartesian.h>
#include <CGAL/Plane_3.h>

#include <CGAL/Point_3.h>

#include <CGAL/intersections.h>
#include <CGAL/squared_distance_2.h>

#include <limits.h>

namespace lamure {
namespace xyz
{

const vec3r_t surfel::
random_point_on_surfel() const {

    auto compute_orthogonal_vector = [] (vec3f_t const& n) {

        vec3f_t  u(std::numeric_limits<float>::lowest(),
                   std::numeric_limits<float>::lowest(),
                   std::numeric_limits<float>::lowest());
        if(n.z_ != 0.0) {
            u = vec3f_t( 1, 1, (-n.x_ - n.y_) / n.z_);
        } else if (n.y_ != 0.0) {
            u = vec3f_t( 1, (-n.x_ - n.z_) / n.y_, 1);
        } else {
            u = vec3f_t( (-n.y_ - n.z_)/n.x_, 1, 1);
        }

        return lamure::math::normalize(u);
    };

    // implements the orthogonal method discussed here:
    // http://math.stackexchange.com/questions/511370/how-to-rotate-one-vector-about-another
    auto rotate_a_around_b_by_rad = [](vec3f_t const& a, vec3f_t const& b, double rad) {
        vec3f_t a_comp_parallel_b = (lamure::math::dot(a,b) / lamure::math::dot(b,b) ) * b;
        vec3f_t a_comp_orthogonal_b = a - a_comp_parallel_b;

        vec3f_t w = lamure::math::cross(b, a_comp_orthogonal_b);

        double x_1 = std::cos(rad) / lamure::math::length(a_comp_orthogonal_b);
        double x_2 = std::sin(rad) / lamure::math::length(w);

        vec3f_t a_rot_by_rad_orthogonal_b = lamure::math::length(a_comp_orthogonal_b) * (x_1 * a_comp_orthogonal_b + x_2 * w);

        return a_rot_by_rad_orthogonal_b + a_comp_parallel_b;
    };

    real_t random_normalized_radius_extent    = ((double)std::rand()/RAND_MAX);
    real_t random_angle_in_radians = 2.0 * M_PI * ((double)std::rand()/RAND_MAX);

    vec3f_t random_direction_along_surfel        = lamure::math::normalize(rotate_a_around_b_by_rad( compute_orthogonal_vector(normal_), 
                                                                                                normal_, random_angle_in_radians));

    // see http://mathworld.wolfram.com/DiskPointPicking.html for a short discussion about
    // random sampling of disks
    return (std::sqrt(random_normalized_radius_extent) * radius_) * vec3r_t(random_direction_along_surfel) + pos_;
} 

bool surfel::
intersect(const surfel &left_surfel, const surfel &right_surfel) {

    auto create_surfel_plane = [](const surfel& target_surfel, bool is_left) {

        auto compute_orthogonal_vector = [] (vec3f_t const& n) {

            vec3f_t  u(std::numeric_limits<float>::lowest(),
                       std::numeric_limits<float>::lowest(),
                       std::numeric_limits<float>::lowest());
            if(n.z_ != 0.0) {
                u = vec3f_t( 1, 1, (-n.x_ - n.y_) / n.z_);
            } else if (n.y_ != 0.0) {
                u = vec3f_t( 1, (-n.x_ - n.z_) / n.y_, 1);
            } else {
                u = vec3f_t( (-n.y_ - n.z_)/n.x_, 1, 1);
            }

            return lamure::math::normalize(u);
        };

        // jitter first normal slightly to avoid numerical problems for splats on skewed a plane
        auto const& jittered_normal = lamure::math::normalize(target_surfel.normal() + vec3f_t(0.001*is_left, 0.0, 0.001*(!is_left)) );
        auto const& tangent = compute_orthogonal_vector(jittered_normal);
        auto const& bitangent = lamure::math::normalize(lamure::math::cross(jittered_normal, tangent));

        auto const& a = target_surfel.pos();
        auto const& b = a + tangent;
        auto const& c = a + bitangent;

        CGAL::Simple_cartesian<double>::Point_3 cgal_a(a[0], a[1], a[2]);
        CGAL::Simple_cartesian<double>::Point_3 cgal_b(b[0], b[1], b[2]);
        CGAL::Simple_cartesian<double>::Point_3 cgal_c(c[0], c[1], c[2]);
        
        return CGAL::Simple_cartesian<double>::Plane_3(cgal_a, cgal_b, cgal_c);
    };

    // retrieve radius and center of both surfels in order to do
    // an early intersection test with the splats considered to be spheres

    auto const left_radius = left_surfel.radius();
    auto const& left_a = left_surfel.pos();

    auto const right_radius = right_surfel.radius();
    auto const& right_a = right_surfel.pos();

    auto const center_distance = lamure::math::length(left_a - right_a);

    if( !(center_distance <= left_radius + right_radius ) ) {
        // not even the spheres did intersect - stop here
        return false;
    }

    auto const left_surfel_plane  = create_surfel_plane(left_surfel, true);
    auto const right_surfel_plane = create_surfel_plane(right_surfel, false);

    CGAL::Object intersection_result = CGAL::intersection(left_surfel_plane, right_surfel_plane);

    CGAL::Simple_cartesian<double>::Line_3 intersection_line;
    CGAL::Simple_cartesian<double>::Plane_3 intersection_plane;

    if (assign(intersection_line, intersection_result)) {
      // check if the distance between the intersection line and the center of both surfels
      // is larger than the respective radii

      // use the center of the left surfel and the intersection line
      auto const& squared_distance_left_to_intersection 
        = CGAL::squared_distance(CGAL::Simple_cartesian<double>::Point_3(left_a[0], left_a[1], left_a[2]), intersection_line);
      auto const& squared_distance_right_to_intersection 
        = CGAL::squared_distance(CGAL::Simple_cartesian<double>::Point_3(right_a[0], right_a[1], right_a[2]), intersection_line);

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
        //std::cout << left_a - right_a << "\n";
        return false;
    }

    return true;
}

bool surfel::
compare_x(const surfel &left_surfel, const surfel &right_surfel) {
    return left_surfel.pos().x_ < right_surfel.pos().x_;
}

bool surfel::
compare_y(const surfel &left_surfel, const surfel &right_surfel) {
    return left_surfel.pos().y_ < right_surfel.pos().y_;
}

bool surfel::
compare_z(const surfel &left_surfel, const surfel &right_surfel) {
    return left_surfel.pos().z_ < right_surfel.pos().z_;
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

