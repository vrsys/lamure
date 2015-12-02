// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <iostream>

#include <lamure/pre/normal_radii_plane_fitting.h>
#include <lamure/pre/surfel.h>

namespace lamure {
namespace pre {
	
vec3f normal_radii_plane_fitting::
compute_normal(size_t node, size_t surfel_of_interest, std::vector<std::pair<surfel, real>> const& neighbours) const {
		
		size_t node_id = node;
    size_t surfel_id = surfel_of_interest;
		//std::cout << "Compute Normal for selected surfel " << surfel_id << "in node " << node_id << "\n";

		return  vec3f(0.f);
	};

real normal_radii_plane_fitting::
compute_radius(size_t node, size_t surfel_of_interest, std::vector<std::pair<surfel, real>> const&  neighbours) const {

		size_t node_id = node;
    size_t surfel_id = surfel_of_interest;
		//std::cout << "Compute Radius for selected surfel " << surfel_id << "in node " << node_id << "\n";

		return 0.0;
	};

}// namespace pre
}// namespace lamure



