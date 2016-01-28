// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/reduction_avg_nearest_neighbour_distance.h>

namespace lamure {
namespace pre {

surfel_mem_array reduction_avg_nearest_neighbour_distance::
create_lod(real& reduction_error,
          const std::vector<surfel_mem_array*>& input,
          const uint32_t surfels_per_node,
          const bvh& tree,
          const size_t start_node_id) const
{
    surfel_mem_array mem_array(std::make_shared<surfel_vector>(surfel_vector()), 0, 0);

    std::vector<std::pair<surfel,real> > candidate_surfels;
    std::vector<surfel> original_surfels;

    // wrap all surfels of the input array to entropy_surfels and push them in the ESA
    for (size_t node_id = 0; node_id < input.size(); ++node_id) {
        for (size_t surfel_id = input[node_id]->offset();
                    surfel_id < input[node_id]->offset() + input[node_id]->length();
                    ++surfel_id){
            
            //this surfel will be referenced in the entropy surfel
            auto current_surfel = input[node_id]->mem_data()->at(input[node_id]->offset() + surfel_id);
            
            // ignore outlier radii of any kind
            if (current_surfel.radius() == 0.0) {
                continue;
            } 

            // only place where shared pointers should be created
            candidate_surfels.push_back( std::make_pair( (current_surfel), 0.0) );
            original_surfels.push_back(current_surfel);
       }
    }

    
    while( candidate_surfels.size() > surfels_per_node ) {

        unsigned first_surfel_id = 0;

        uint16_t num_neighbours = candidate_surfels.size() - 1;

        for (auto& surf_dist_pair : candidate_surfels ) {

            std::vector<real> total_neighbour_distances;

            unsigned second_surfel_id = 0;
            for(auto const& comparison_pair : candidate_surfels) {

                if( first_surfel_id != second_surfel_id ) {

                    real length_squared = scm::math::length_sqr(surf_dist_pair.first.pos() - comparison_pair.first.pos());
                    total_neighbour_distances.push_back(length_squared);

                }
                ++second_surfel_id;
            }

            real avg_dist = 0.0;

            if( !total_neighbour_distances.empty() ) {
                for ( auto const& dist : total_neighbour_distances ) {
                    avg_dist += dist;
                }
                avg_dist /= total_neighbour_distances.size();
            }

            surf_dist_pair.second = avg_dist;
            ++first_surfel_id;
        }

        std::sort( std::begin(candidate_surfels), std::end(candidate_surfels), [](std::pair<surfel, real>const& lhs, std::pair<surfel, real> const& rhs){ return lhs.second > rhs.second; } );
        candidate_surfels.pop_back();
    }

    // retrieve natural neighbour ids for the final surfels


    for (auto const& final_candidate : candidate_surfels) {
        mem_array.mem_data()->push_back(final_candidate.first);
    }

    mem_array.set_length(mem_array.mem_data()->size());

    reduction_error = 0.0;

    return mem_array;
}

} // namespace pre
} // namespace lamure