// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <iostream>

#include <lamure/pre/normal_computation_plane_fitting.h>

namespace lamure {
namespace pre{
	
vec3f NormalComputationPlaneFitting::
compute_normal(const size_t node_id, const size_t surfel_id,  const uint16_t number_of_neighbours) const {
		
		return  vec3f(0.f);
	};

}// namespace pre
}// namespace lamure



