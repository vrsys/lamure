// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/reduction_sorted_every_second.h>

namespace lamure {
namespace pre {

surfel_mem_array reduction_sorted_every_second::
create_lod(real& reduction_error,
          const std::vector<surfel_mem_array*>& input,
          const uint32_t surfels_per_node,
          const bvh& tree,
          const size_t start_node_id) const
{
    surfel_mem_array mem_array(std::make_shared<surfel_vector>(surfel_vector()), 0, 0);

    std::vector< surfel > candidate_surfels;

    auto const& tree_nodes = tree.nodes();

    auto combined_bounding_box = tree_nodes[start_node_id].get_bounding_box();

    for( uint8_t non_first_child_ids = 1; 
         non_first_child_ids < tree.fan_factor(); 
         ++non_first_child_ids) {

        auto const& sibling_bb = tree_nodes[start_node_id + non_first_child_ids].get_bounding_box();
        combined_bounding_box.expand( sibling_bb.max() );
        combined_bounding_box.expand( sibling_bb.min() );
    }

    //std::vector<std::pair<bounding_box, std::vector<surfel> > > spatially_divided_scene_boxes;

    uint8_t const num_divisions_per_axis = 3;

    std::vector<surfel> original_surfels;

    std::vector<std::vector<surfel> >spatially_divided_surfels;
    spatially_divided_surfels.resize( pow(num_divisions_per_axis, 3) );
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
            //candidate_surfels.push_back( (current_surfel) );

            uint32_t x_summand = std::max(uint8_t(0), std::min( uint8_t( num_divisions_per_axis - 1), uint8_t( ( current_surfel.pos()[0] - combined_bounding_box.min()[0] ) /
                                 ( combined_bounding_box.max()[0] - combined_bounding_box.min()[0] ) *
                                 num_divisions_per_axis) ) ) * num_divisions_per_axis * num_divisions_per_axis;

            uint32_t y_summand = std::max(uint8_t(0), std::min( uint8_t(num_divisions_per_axis - 1), uint8_t( ( current_surfel.pos()[1] - combined_bounding_box.min()[1] ) /
                                 ( combined_bounding_box.max()[1] - combined_bounding_box.min()[1] ) *
                                 num_divisions_per_axis) ) ) * num_divisions_per_axis;

            uint32_t z_summand = std::max(uint8_t(0), std::min( uint8_t(num_divisions_per_axis - 1), uint8_t( ( current_surfel.pos()[2] - combined_bounding_box.min()[2] ) /
                                 ( combined_bounding_box.max()[2] - combined_bounding_box.min()[2] ) *
                                 num_divisions_per_axis) ) );

            uint32_t box_id = x_summand + y_summand + z_summand;

            //std::cout << "box id: " << box_id << "\n";
            spatially_divided_surfels[box_id].push_back(current_surfel);
            original_surfels.push_back(current_surfel);
       }
    }



    for( auto& surfel_box : spatially_divided_surfels) {
        std::sort(std::begin(surfel_box), std::end(surfel_box), [](surfel const& lhs, surfel const& rhs){
            vec3r lhs_pos = lhs.pos();
            vec3r rhs_pos = rhs.pos();

            if (lhs_pos[0] < rhs_pos[0]) {
                return true;
            } else if (lhs_pos[0] == rhs_pos[0]) {
                if (lhs_pos[1] < rhs_pos[1]) {
                    return true;
                } else if (lhs_pos[1] == rhs_pos[1]) {
                    if(lhs_pos[2] < rhs_pos[2]) {
                        return true;
                    } else if (lhs_pos[2] == rhs_pos[2]) {
                        return &lhs < &rhs;
                    }
                }
            }

            return false;
        });
    }

    std::vector<surfel> final_surfels;
    std::vector<surfel> not_considered_surfels;

 
   bool gathered_all_surfels = false;
    do{
        for( auto& surfel_box : spatially_divided_surfels) {
            for (uint32_t surf_idx = 0; surf_idx < surfel_box.size(); surf_idx += 2) {

                uint8_t random_surfel_idx = rand() % tree.fan_factor();

                if (surf_idx + random_surfel_idx >= surfel_box.size() ) {
                    random_surfel_idx = 0;
                }

                final_surfels.push_back(surfel_box[surf_idx + random_surfel_idx]);

                if(final_surfels.size() == surfels_per_node) {
                    gathered_all_surfels = true;
                    break;
                }

                for(int i = 0; i < tree.fan_factor(); ++i) {
                    if( i != random_surfel_idx) {

                        if (surf_idx + random_surfel_idx >= surfel_box.size() )
                            continue;

              
                        not_considered_surfels.push_back(surfel_box[surf_idx + random_surfel_idx]);
                    }
                }
            }

            if(gathered_all_surfels) {
                break;
            }

        }


        for(auto& vec : spatially_divided_surfels) {
            vec.clear();
        }


        while( !not_considered_surfels.empty() ) {

            auto& current_surfel = not_considered_surfels.back();
                uint32_t x_summand = std::max(uint8_t(0), std::min( uint8_t( num_divisions_per_axis - 1), uint8_t( ( current_surfel.pos()[0] - combined_bounding_box.min()[0] ) /
                                     ( combined_bounding_box.max()[0] - combined_bounding_box.min()[0] ) *
                                     num_divisions_per_axis) ) ) * num_divisions_per_axis * num_divisions_per_axis;

                uint32_t y_summand = std::max(uint8_t(0), std::min( uint8_t(num_divisions_per_axis - 1), uint8_t( ( current_surfel.pos()[1] - combined_bounding_box.min()[1] ) /
                                     ( combined_bounding_box.max()[1] - combined_bounding_box.min()[1] ) *
                                     num_divisions_per_axis) ) ) * num_divisions_per_axis;

                uint32_t z_summand = std::max(uint8_t(0), std::min( uint8_t(num_divisions_per_axis - 1), uint8_t( ( current_surfel.pos()[2] - combined_bounding_box.min()[2] ) /
                                     ( combined_bounding_box.max()[2] - combined_bounding_box.min()[2] ) *
                                     num_divisions_per_axis) ) );

                uint32_t box_id = x_summand + y_summand + z_summand;

                spatially_divided_surfels[box_id].push_back(current_surfel.pos());

                not_considered_surfels.pop_back();

                for( auto& surfel_box : spatially_divided_surfels) {
                    std::sort(std::begin(surfel_box), std::end(surfel_box), [](surfel const& lhs, surfel const& rhs){
                        vec3r lhs_pos = lhs.pos();
                        vec3r rhs_pos = rhs.pos();

                        if (lhs_pos[0] < rhs_pos[0]) {
                            return true;
                        } else if (lhs_pos[0] == rhs_pos[0]) {
                            if (lhs_pos[1] < rhs_pos[1]) {
                                return true;
                            } else if (lhs_pos[1] == rhs_pos[1]) {
                                if(lhs_pos[2] < rhs_pos[2]) {
                                    return true;
                                } else if (lhs_pos[2] == rhs_pos[2]) {
                                    return &lhs < &rhs;
                                }
                            }
                        }

                        return false;
                    });
                }

        }

    } while(final_surfels.size() < surfels_per_node);


    for (auto& final_candidate : final_surfels) {

        auto nn_pairs = tree.get_locally_natural_neighbours(original_surfels, final_candidate.pos(), 24);

        //std::cout << "NN_pairs size: " << nn_pairs.size() << "\n";
        real accumulated_nn_weights = 0.0;
        vec3r accumulated_color = vec3r(0.0, 0.0, 0.0);
        vec3r accumulated_normal = vec3r(0.0, 0.0, 0.0);

        for (auto const& nn_pair : nn_pairs ) {
            real nn_weight = nn_pair.second;
            accumulated_color  += vec3r(nn_pair.first.color()) * nn_weight;
            accumulated_normal += nn_pair.first.normal() * nn_weight;

            accumulated_nn_weights += nn_weight;
        }

        if (accumulated_nn_weights != 0.0) {
            accumulated_color /= accumulated_nn_weights;
            accumulated_normal /= accumulated_nn_weights;

            final_candidate.color() = accumulated_color;
            final_candidate.normal() = accumulated_normal;
         }

        mem_array.mem_data()->push_back(final_candidate);
    }

    if(mem_array.mem_data()->size() < surfels_per_node) {
        mem_array.mem_data()->push_back(surfel(vec3r(0.0, 0.0, 0.0), vec3b(0,0,0) ));
    }

    mem_array.set_length(mem_array.mem_data()->size());

    reduction_error = 0.0;

    return mem_array;
}

} // namespace pre
} // namespace lamure