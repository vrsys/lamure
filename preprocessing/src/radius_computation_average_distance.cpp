// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <iostream>

#include <lamure/pre/bvh.h>
#include <lamure/pre/radius_computation_average_distance.h>

namespace lamure {
namespace pre{
	

real radius_computation_average_distance::
compute_radius(const bvh& tree,
			   const surfel_id_t target_surfel,
			   std::vector<std::pair<surfel_id_t, real>> const& nearest_neighbours) const {

    real avg_distance = 0.0;

    unsigned processed_neighbour_counter = 0;
    // compute radius
    for (auto const& neighbour : nearest_neighbours) {
    	if( processed_neighbour_counter++ < number_of_neighbours_) {
    		avg_distance += sqrt(neighbour.second);
    	}
    	else {
    		break;
    	}
    }

    avg_distance /= processed_neighbour_counter;

	return avg_distance * radius_multiplier_;
	};

}// namespace pre
}// namespace lamure