// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/bvh.h>
#include <lamure/pre/radius_computation_natural_neighbours.h>

#include <iostream>

namespace lamure {
namespace pre{


real radius_computation_natural_neighbours::
compute_radius(const bvh& tree,
			   const surfel_id_t target_surfel,
               std::vector<std::pair<surfel_id_t, real>> const& nearest_neighbours) const {
	
	// find nearest neighbours
	//std::vector<std::pair<surfel_id_t, real>> nearest_neighbours 
    //    = tree.get_nearest_neighbours(target_surfel, number_of_neighbours_);
    
                 
    if (nearest_neighbours.size() < min_num_nearest_neighbours_) {
         return 0.0f;
    }
    
    auto const natural_neighbour_ids = tree.get_natural_neighbours(target_surfel, false, nearest_neighbours );

    if (natural_neighbour_ids.size() < min_num_natural_neighbours_) {
        return 0.0;
    }

    std::vector<surfel> natural_neighbours;

    for (auto const& surf_id_pair : natural_neighbour_ids ) {

        auto const& current_node = tree.nodes()[surf_id_pair.first.node_idx];
        natural_neighbours.emplace_back( current_node.mem_array().read_surfel(surf_id_pair.first.surfel_idx).pos() );
    }

    vec3r point_of_interest = ((tree.nodes()[target_surfel.node_idx]).mem_array().read_surfel(target_surfel.surfel_idx)).pos();

    //determine most distant natural neighbour
    real max_distance = 0.f;
    for (const auto& nn : natural_neighbours) {

        auto const& surfel_pos = nn.pos();
        real new_dist = scm::math::length_sqr( point_of_interest - surfel_pos );
        max_distance = std::max(max_distance, new_dist);
    }

    if (max_distance >= std::numeric_limits<real>::min()) {
        max_distance = scm::math::sqrt(max_distance);
        return 0.5 * max_distance;

    } else { 
        return 0.0;
    }

    natural_neighbours.clear();

	};

}// namespace pre
}// namespace lamure