// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/node_statistics.h>

namespace lamure {
namespace pre {

void node_statistics::
calculate_statistics(surfel_mem_array const& mem_array) {

	vec3r temp_mean = vec3r(0.0, 0.0, 0.0);

    for (size_t j = mem_array.offset();
                j < mem_array.offset() + mem_array.length();
                ++j) {
    	temp_mean += mem_array.mem_data()->at(j).pos();
    }

    temp_mean /= mem_array.length();

	real  temp_sd = 0.0;
    for (size_t j = mem_array.offset();
                j < mem_array.offset() + mem_array.length();
                ++j) {
    	temp_sd += scm::math::length(temp_mean - mem_array.mem_data()->at(j).pos() );
    }

    temp_sd /= mem_array.length();

    mean_ = temp_mean;
    sd_   = temp_sd;
}

}
}