// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/reduction_particle_simulation.h>

#include <lamure/pre/radius_computation_natural_neighbours.h>
#include <lamure/pre/plane.h>

//#include <math.h>
#include <functional>
#include <numeric>
#include <vector>
#include <queue>
#include <map>
#include <set>

namespace lamure {
namespace pre {

surfel_mem_array reduction_particle_simulation::
create_lod(real& reduction_error,
          const std::vector<surfel_mem_array*>& input,
          const uint32_t surfels_per_node,
          const bvh& tree,
          const size_t start_node_id) const {

    //create output array
    surfel_mem_array mem_array(std::make_shared<surfel_vector>(surfel_vector()), 0, 0);


    std::vector<std::pair<real, surfel_id_t> > surfel_lookup_vector;

    std::map<surfel_id_t, neighbours_t> surfels_k_nearest_neighbours;

    real accumulated_weights = 0.0;

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

            surfel_id_t current_surfel_ids(start_node_id + node_id, surfel_id);

            auto neighbours = tree.get_nearest_neighbours( current_surfel_ids, 24 );
            surfels_k_nearest_neighbours[current_surfel_ids] = neighbours;

            real enclosing_sphere_radius = compute_enclosing_sphere_radius(current_surfel,
                                                                           neighbours,
                                                                           tree);

            real current_weight = 0.0;
            if( enclosing_sphere_radius != 0.0) {
                current_weight = 1.0 / (enclosing_sphere_radius*enclosing_sphere_radius);
            }

            accumulated_weights += current_weight;

            surfel_lookup_vector.push_back(std::make_pair(accumulated_weights, current_surfel_ids) );
        // only place where shared pointers should be created
        //entropy_surfel_array.push_back( std::make_shared<entropy_surfel>(current_entropy_surfel) );
       }
    }

    // normalize weights, such that we can draw a random number between 0 and 1 to choose a surfel
    for( auto& weight_entry : surfel_lookup_vector ) {
        weight_entry.first /= accumulated_weights;
    }


    //draw our surfels for the next level:
    for( uint32_t final_surfel_idx = 0; final_surfel_idx < surfels_per_node; ++final_surfel_idx ) {

        real rand_weighted_surfel_index = (double)std::rand()/ RAND_MAX;
        //std::cout << "Rand num: " << rand_weighted_surfel_index << "\n";
        size_t surfel_vector_idx = surfel_lookup_vector.size() - 1;


        for( size_t vector_idx = 0; vector_idx < surfel_lookup_vector.size(); ++vector_idx) {
            if ( surfel_lookup_vector[vector_idx].first > rand_weighted_surfel_index ) {
                surfel_vector_idx = std::max(0, int(vector_idx-1) ) ;
                break; //stop, we found the index for our surfel
            }
        }

        //std::cout << "Final index: " << surfel_vector_idx << "/" << surfel_lookup_vector.size()-1<<"\n";
        surfel_id_t rand_drawn_indices = surfel_lookup_vector[surfel_vector_idx].second;


        auto surfel_to_sample = input[rand_drawn_indices.node_idx - start_node_id]->mem_data()->at(input[rand_drawn_indices.node_idx - start_node_id]->offset() + rand_drawn_indices.surfel_idx);
    

        surfel surfel_to_push(surfel_to_sample);

        surfel_to_push.pos() = surfel_to_push.random_point_on_surfel();

        auto neighbours_of_rand_surfel = tree.get_nearest_neighbours( rand_drawn_indices, 24 );

        std::vector<surfel> neighbour_surfs_of_rand_surfel;
        std::vector<vec3r> neighbour_positions;
        for( auto const& neigh_of_rand : neighbours_of_rand_surfel ) {

            surfel neighbour_surf = 
                input[neigh_of_rand.first.node_idx]->mem_data()->at(input[neigh_of_rand.first.node_idx]->offset() + neigh_of_rand.first.surfel_idx);

            neighbour_surfs_of_rand_surfel.push_back(neighbour_surf);

            neighbour_positions.push_back(neighbour_surf.pos());
        }


        plane_t plane_to_project_onto;
        plane_t::fit_plane(neighbour_positions, plane_to_project_onto);

        vec3r projected_surfel_pos = surfel_to_push.pos();

        vec2r plane_coords = plane_to_project_onto.project(plane_to_project_onto, plane_to_project_onto.get_right(), projected_surfel_pos);
/*
        surfel surfel_to_push;
        surfel_to_push.pos() = vec3r(0.0, 0.0, 0.0);
        surfel_to_push.color() = vec3b(255,255,255);
        surfel_to_push.normal() = vec3f(0.0, 1.0, 0.0);
        surfel_to_push.radius() = 1.0;
*/
        //std::cout << surfel_to_push.pos();

        mem_array.mem_data()->push_back(surfel_to_push);
        //mem_array.mem_data()->push_back()
        //std::cout << "Executing\n";
    }


    //std::cout << "Done with " << mem_array.mem_data()->size() << " surfels\n";
    mem_array.set_length(mem_array.mem_data()->size());
    reduction_error = 0.0;

    return mem_array;
};


real reduction_particle_simulation::
compute_enclosing_sphere_radius(surfel const& target_surfel,
                                neighbours_t const& neighbour_ids,
                                bvh const& tree) const {
    double enclosing_sphere_radius = 0.0;

    for (auto const& neigh_id_pair : neighbour_ids) {

        auto const& current_node = tree.nodes()[neigh_id_pair.first.node_idx];
        surfel const neighbour_surfel = current_node.mem_array().read_surfel(neigh_id_pair.first.surfel_idx);

        enclosing_sphere_radius = std::max(enclosing_sphere_radius, 
                                      scm::math::length(target_surfel.pos() - neighbour_surfel.pos()) );
    }

    return enclosing_sphere_radius;
}


} // namespace pre
} // namespace lamure
