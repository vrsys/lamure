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

void expand_local_bb(vec3r& bb_min, vec3r& bb_max, vec3r const& surfel_pos) {

    for( int axis_idx = 0; axis_idx < 3; ++axis_idx ) {
        if(surfel_pos[axis_idx] < bb_min[axis_idx]) {
            bb_min[axis_idx] = surfel_pos[axis_idx];
        }

        if(surfel_pos[axis_idx] > bb_max[axis_idx]) {
            bb_max[axis_idx] = surfel_pos[axis_idx];
        }

    }
}

void clamp_surfel_to_bb(vec3r const& bb_min, vec3r const& bb_max, vec3r& surfel_pos){
    for( int axis_idx = 0; axis_idx < 3; ++axis_idx ) {
        if(surfel_pos[axis_idx] < bb_min[axis_idx]) {
            surfel_pos[axis_idx] = bb_min[axis_idx];
        }

        if(surfel_pos[axis_idx] > bb_max[axis_idx]) {
            surfel_pos[axis_idx] = bb_max[axis_idx];
        }

    }
}

surfel_mem_array reduction_particle_simulation::
create_lod(real& reduction_error,
          const std::vector<surfel_mem_array*>& input,
          const real avg_radius_all_nodes,
          const uint32_t surfels_per_node,
          const bvh& tree,
          const size_t start_node_id) const {

    std::vector<surfel> original_surfels;

    //create output array
    surfel_mem_array mem_array(std::make_shared<surfel_vector>(surfel_vector()), 0, 0);

    std::vector<std::pair<real, surfel_id_t> > surfel_lookup_vector;

    std::map<surfel_id_t, neighbours_t> surfels_k_nearest_neighbours;

    real accumulated_weights = 0.0;

    vec3r bb_min = vec3r(std::numeric_limits<real>::max(),
                         std::numeric_limits<real>::max(),
                         std::numeric_limits<real>::max());

    vec3r bb_max = vec3r(std::numeric_limits<real>::lowest(),
                         std::numeric_limits<real>::lowest(),
                         std::numeric_limits<real>::lowest());

    // wrap all surfels of the input array to entropy_surfels and push them in the ESA
    for (size_t node_id = 0; node_id < input.size(); ++node_id) {
        for (size_t surfel_id = input[node_id]->offset();
                    surfel_id < input[node_id]->offset() + input[node_id]->length();
                    ++surfel_id){
            
            //this surfel will be referenced in the entropy surfel
            auto current_surfel = input[node_id]->mem_data()->at(input[node_id]->offset() + surfel_id);
            
            vec3r const& curr_surf_pos = current_surfel.pos();
            expand_local_bb(bb_min, bb_max, curr_surf_pos);


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
            original_surfels.push_back(current_surfel);
        // only place where shared pointers should be created
        //entropy_surfel_array.push_back( std::make_shared<entropy_surfel>(current_entropy_surfel) );
       }
    }

    // normalize weights, such that we can draw a random number between 0 and 1 to choose a surfel
    for( auto& weight_entry : surfel_lookup_vector ) {
        weight_entry.first /= accumulated_weights;
    }

    std::vector<std::pair<surfel, real> > particle_repulsion_rad_pairs;

    auto const& global_nodes = tree.nodes();

    //draw our surfels for the next level:
    for( uint32_t final_surfel_idx = 0; final_surfel_idx < surfels_per_node; ++final_surfel_idx ) {

        real rand_weighted_surfel_index = (double)std::rand()/ RAND_MAX;
        size_t surfel_vector_idx = surfel_lookup_vector.size() - 1;

        for( size_t vector_idx = 0; vector_idx < surfel_lookup_vector.size(); ++vector_idx) {
            if ( surfel_lookup_vector[vector_idx].first > rand_weighted_surfel_index ) {
                surfel_vector_idx = std::max(0, int(vector_idx-1) ) ;
                break; //stop, we found the index for our surfel
            }
        }

        surfel_id_t rand_drawn_indices = surfel_lookup_vector[surfel_vector_idx].second;

        auto surfel_to_sample = global_nodes[rand_drawn_indices.node_idx].mem_array().read_surfel(rand_drawn_indices.surfel_idx);

        surfel surfel_to_push(surfel_to_sample);

        surfel_to_push.pos() = surfel_to_push.random_point_on_surfel();

        auto neighbours_of_rand_surfel = tree.get_nearest_neighbours( rand_drawn_indices, 24 );

        std::vector<surfel> neighbour_surfs_of_rand_surfel;
        std::vector<vec3r> neighbour_positions;

        real min_neighbour_distance = std::numeric_limits<real>::max();


        for( auto const& neigh_of_rand : neighbours_of_rand_surfel ) {

            surfel neighbour_surf = 
                global_nodes[neigh_of_rand.first.node_idx].mem_array().read_surfel( neigh_of_rand.first.surfel_idx );
            neighbour_surfs_of_rand_surfel.push_back(neighbour_surf);

            neighbour_positions.push_back(neighbour_surf.pos());

            real neighbour_distance = scm::math::length(neighbour_surf.pos() - surfel_to_push.pos());

            min_neighbour_distance = std::min(min_neighbour_distance, neighbour_distance );
        }


        plane_t plane_to_project_onto;
        plane_t::fit_plane(neighbour_positions, plane_to_project_onto);

        vec3r projected_surfel_pos = surfel_to_push.pos();

        vec2r plane_coords = plane_to_project_onto.project(plane_to_project_onto, plane_to_project_onto.get_right(), projected_surfel_pos);

        surfel_to_push.pos() =  plane_to_project_onto.get_point_on_plane(plane_coords);


        //determine color by natural neighbour interpolation (right now from the point of view of the current surfel)

        vec3b old_color = surfel_to_push.color();

        auto const nn_indices = tree.get_natural_neighbours(rand_drawn_indices, 24);

        // interpolate colors of natural neighbours
        if( nn_indices.size() > 2) {
            vec3r new_temp_color = vec3r(0.0, 0.0, 0.0);
            double accumulated_weights = 0.0;

            for( auto const& nn_index : nn_indices ) {
                surfel const natural_neighbour = 
                    global_nodes[nn_index.first.node_idx].mem_array().read_surfel( nn_index.first.surfel_idx );
                new_temp_color += natural_neighbour.color() * 1.0;// nn_index.second;

                accumulated_weights += 1.0;//nn_index.second;
            }

            //add a little contribution of the surfel itself as well
            double own_surfel_weight = accumulated_weights / nn_indices.size();

            new_temp_color += old_color * own_surfel_weight;

            accumulated_weights += own_surfel_weight;

            surfel_to_push.color() = new_temp_color/accumulated_weights;
        }


        particle_repulsion_rad_pairs.push_back( std::pair<surfel, real>(surfel_to_push,  5 * min_neighbour_distance ) );
    }

    // perform particle repulsion

    //repulsion constant k
    double k = 0.001;
    {
        std::vector<vec3r> particle_displacements;


        particle_displacements.reserve( particle_repulsion_rad_pairs.size() );
        for(auto const& part_rep_pair : particle_repulsion_rad_pairs) {
            vec3r particle_displacement = vec3r(0.0, 0.0, 0.0);

            double r = part_rep_pair.second;

            for(auto const& potential_influence_pair : particle_repulsion_rad_pairs) {

                if(potential_influence_pair.first == part_rep_pair.first)
                    continue;

                if(potential_influence_pair.first.radius() == 0.0)
                    continue;

                vec3r particle_neighbour_vec = part_rep_pair.first.pos() - potential_influence_pair.first.pos();
                real neighbour_dist = scm::math::length(particle_neighbour_vec);

                if (neighbour_dist < part_rep_pair.second) {
                    particle_displacement += k * (r - neighbour_dist) * particle_neighbour_vec;
                }
            }

            particle_displacements.push_back(particle_displacement);
        }


        size_t displacement_idx = 0;

        for(auto& part_rep_pair : particle_repulsion_rad_pairs) {
            part_rep_pair.first.pos() += particle_displacements[displacement_idx++];

            clamp_surfel_to_bb(bb_min, bb_max, part_rep_pair.first.pos());

            interpolate_approx_natural_neighbours(part_rep_pair.first, original_surfels, tree);

            mem_array.mem_data()->push_back(part_rep_pair.first);
        }

    }

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
