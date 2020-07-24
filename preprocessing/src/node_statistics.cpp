// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <limits>

#include <lamure/pre/node_statistics.h>

namespace lamure
{
namespace pre
{

void node_statistics::
calculate_statistics(surfel_mem_array const &mem_array)
{

    vec3r temp_mean = vec3r(0.0, 0.0, 0.0);
    vec3r temp_mean_color = vec3r(0.0, 0.0, 0.0);
    vec3r temp_mean_normal = vec3r(0.0, 0.0, 0.0);
    real temp_mean_radius = 0.0;
    real temp_max_radius = 0.0;
    real temp_min_radius = std::numeric_limits<real>::max();

    for (int color_comp_idx = 0;
         color_comp_idx < 3;
         ++color_comp_idx) {
        color_histogram_[color_comp_idx].resize(256, 0);
    }

    size_t num_contributed_surfels(0);

    for (size_t j = mem_array.offset();
         j < mem_array.offset() + mem_array.length();
         ++j) {
        surfel const &current_surfel = mem_array.surfel_mem_data()->at(j);
        if (current_surfel.radius() != 0.0) {

            vec3b const &surfel_color = current_surfel.color();
            temp_mean += current_surfel.pos();
            temp_mean_color += surfel_color;
            temp_mean_normal += current_surfel.normal();

            real current_radius = current_surfel.radius();
            temp_mean_radius += current_surfel.radius();

            temp_max_radius = std::max(temp_max_radius, current_radius);
            temp_min_radius = std::min(temp_min_radius, current_radius);

            for (int color_comp_idx = 0; color_comp_idx < 3; ++color_comp_idx) {
                ++(color_histogram_[color_comp_idx][surfel_color[color_comp_idx]]);
            }
            ++num_contributed_surfels;
        }
    }

    if (num_contributed_surfels) {
        temp_mean /= num_contributed_surfels;
        temp_mean_color /= num_contributed_surfels;
        temp_mean_normal /= num_contributed_surfels;
        temp_mean_normal
            = scm::math::normalize(temp_mean_normal);
        temp_mean_radius /= num_contributed_surfels;
    }
    else {
        real lowest_real = std::numeric_limits<real>::lowest();
        temp_mean = vec3r(lowest_real,
                          lowest_real,
                          lowest_real);

        temp_mean_color = vec3r(lowest_real,
                                lowest_real,
                                lowest_real);

        temp_mean_normal = vec3r(lowest_real,
                                 lowest_real,
                                 lowest_real);

        temp_mean_radius = lowest_real;

    }

    real temp_sd = 0.0;
    real temp_color_sd = 0.0;
    real temp_normal_sd = 0.0;
    real temp_radius_sd = 0.0;
    if (num_contributed_surfels) {
        for (size_t j = mem_array.offset();
             j < mem_array.offset() + mem_array.length();
             ++j) {
            surfel const &current_surfel = mem_array.surfel_mem_data()->at(j);

            temp_sd += std::pow(scm::math::length(temp_mean - current_surfel.pos()), 2);
            temp_color_sd += std::pow(scm::math::length(temp_mean_color - vec3b(current_surfel.color()) ), 2);
            temp_normal_sd += std::pow(scm::math::length(temp_mean_normal - current_surfel.normal()), 2);
            temp_radius_sd += std::pow((temp_mean_radius - current_surfel.radius()), 2);

        }

        temp_sd = std::sqrt(temp_sd / num_contributed_surfels);
        temp_color_sd = std::sqrt(temp_color_sd / num_contributed_surfels);
        temp_normal_sd = std::sqrt(temp_normal_sd / num_contributed_surfels);
        temp_radius_sd = std::sqrt(temp_radius_sd / num_contributed_surfels);
    }
    else {
        real lowest_real = std::numeric_limits<real>::lowest();

        temp_sd = lowest_real;
        temp_color_sd = lowest_real;
        temp_normal_sd = lowest_real;
        temp_radius_sd = lowest_real;
    }


    mean_pos_ = temp_mean;
    pos_sd_ = temp_sd;

    mean_color_ = temp_mean_color;
    color_sd_ = temp_color_sd;

    mean_normal_ = temp_mean_normal;
    normal_sd_ = temp_normal_sd;

    mean_radius_ = temp_mean_radius;
    radius_sd_ = temp_radius_sd;
}

}
}
