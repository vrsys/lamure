// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <limits>

#include <lamure/xyz/node_statistics.h>

namespace lamure {
namespace xyz {

void node_statistics::
calculate_statistics(surfel_mem_array const& mem_array) {

    vec3r_t temp_mean        = vec3r_t(0.0, 0.0, 0.0);
    vec3r_t temp_mean_color  = vec3r_t(0.0, 0.0, 0.0);
    vec3r_t temp_mean_normal = vec3r_t(0.0, 0.0, 0.0);
    real_t  temp_mean_radius = 0.0;
    real_t  temp_max_radius  = 0.0;
    real_t  temp_min_radius  = std::numeric_limits<real_t>::max();

    for(int color_comp_idx = 0;
            color_comp_idx < 3;
	    ++color_comp_idx) {
        color_histogram_[color_comp_idx].resize(256, 0);
    }

    size_t num_contributed_surfels(0);

    for (size_t j = mem_array.offset();
                j < mem_array.offset() + mem_array.length();
                ++j) {
        surfel const& current_surfel = mem_array.mem_data()->at(j);
        if (current_surfel.radius() != 0.0) {

	    vec3b_t const& surfel_color = current_surfel.color();
            temp_mean        += current_surfel.pos();
            temp_mean_color  += surfel_color;
            temp_mean_normal += current_surfel.normal();

            real_t current_radius = current_surfel.radius();
            temp_mean_radius += current_surfel.radius();

            temp_max_radius = std::max(temp_max_radius, current_radius);
            temp_min_radius = std:: min(temp_min_radius, current_radius);

            for (int color_comp_idx = 0; color_comp_idx < 3; ++color_comp_idx) {
	        ++(color_histogram_[color_comp_idx][surfel_color[color_comp_idx]]);
            }
	    ++num_contributed_surfels;
	   }
    }

    if (num_contributed_surfels) {
        temp_mean        /= num_contributed_surfels;
        temp_mean_color  /= num_contributed_surfels;
	temp_mean_normal /= num_contributed_surfels;
        temp_mean_normal 
		= lamure::math::normalize(temp_mean_normal);
	temp_mean_radius /= num_contributed_surfels;
    } else {
	real_t lowest_real_t = std::numeric_limits<real_t>::lowest();
        temp_mean        = vec3r_t(lowest_real_t,
			         lowest_real_t,
			         lowest_real_t);
	
        temp_mean_color  = vec3r_t(lowest_real_t,
			         lowest_real_t,
			         lowest_real_t);

        temp_mean_normal = vec3r_t(lowest_real_t,
                                 lowest_real_t,
				                 lowest_real_t);

	temp_mean_radius = lowest_real_t;

    }

    real_t temp_sd = 0.0;
    real_t temp_color_sd = 0.0; 
    real_t temp_normal_sd = 0.0;
    real_t temp_radius_sd = 0.0;
    if(num_contributed_surfels) {
        for (size_t j = mem_array.offset();
                    j < mem_array.offset() + mem_array.length();
                  ++j) {
	    surfel const& current_surfel = mem_array.mem_data()->at(j);

    	    temp_sd += std::pow(lamure::math::length(temp_mean - current_surfel.pos() ), 2);
            temp_color_sd += lamure::math::pow(lamure::math::length(temp_mean_color - current_surfel.color()), 2);
            temp_normal_sd += lamure::math::pow(lamure::math::length(temp_mean_normal - current_surfel.normal()), 2);
	    temp_radius_sd += lamure::math::pow((temp_mean_radius - current_surfel.radius()),2);

	}

        temp_sd = std::sqrt(temp_sd / num_contributed_surfels);
	temp_color_sd = std::sqrt( temp_color_sd / num_contributed_surfels);
	temp_normal_sd = std::sqrt( temp_normal_sd / num_contributed_surfels);
	temp_radius_sd = std::sqrt( temp_radius_sd / num_contributed_surfels);
    } else {
	real_t lowest_real_t = std::numeric_limits<real_t>::lowest();
        
	temp_sd        = lowest_real_t;
	temp_color_sd  = lowest_real_t;
	temp_normal_sd = lowest_real_t;
	temp_radius_sd = lowest_real_t;
    }


    mean_pos_     = temp_mean;
    pos_sd_       = temp_sd;

    mean_color_   = temp_mean_color;
    color_sd_     = temp_color_sd;

    mean_normal_  = temp_mean_normal;
    normal_sd_    = temp_normal_sd;

    mean_radius_  = temp_mean_radius;
    radius_sd_    = temp_radius_sd; 
}

}
}
