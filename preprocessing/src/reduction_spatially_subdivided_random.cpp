// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/reduction_spatially_subdivided_random.h>

#include <set>
#include <exception>
namespace lamure {
namespace pre {

surfel_mem_array reduction_spatially_subdivided_random::
create_lod(real_t& reduction_error,
          const std::vector<surfel_mem_array*>& input,
          const uint32_t surfels_per_node,
          const bvh& tree,
          const size_t start_node_id) const
{
    surfel_mem_array mem_array(std::make_shared<surfel_vector>(surfel_vector()), 0, 0);

    std::vector<surfel> already_picked_surfel;
    std::vector<surfel> original_surfels;


    auto const& tree_nodes = tree.nodes();
    auto combined_bounding_box = tree_nodes[start_node_id].get_bounding_box();

    for( uint8_t non_first_child_ids = 1; 
         non_first_child_ids < tree.fan_factor(); 
         ++non_first_child_ids) {

        auto const& sibling_bb = tree_nodes[start_node_id + non_first_child_ids].get_bounding_box();
        combined_bounding_box.expand( sibling_bb.max() );
        combined_bounding_box.expand( sibling_bb.min() );
    }


    uint32_t const num_divisions_for_shortest_axis = 8;

    vec3r_t step_length(-1.0, -1.0, -1.0);

    uint8_t shortest_axis = -1;
    real_t min_length = std::numeric_limits<real_t>::max();

    vec3r_t axis_lengths = combined_bounding_box.max() - combined_bounding_box.min();

    for( uint8_t axis_idx = 0; axis_idx < 3; ++axis_idx ) {
        if( axis_lengths[axis_idx] < min_length ) {
            min_length = axis_lengths[axis_idx];
            shortest_axis = axis_idx;
        }
    }

    step_length[shortest_axis] = axis_lengths[shortest_axis] / num_divisions_for_shortest_axis;

    vec3b_t steps_per_axis(-1,-1,-1);

    steps_per_axis[shortest_axis] = num_divisions_for_shortest_axis;

    for( uint8_t axis_idx = 0; axis_idx < 3; ++axis_idx ) {
        if (axis_idx != shortest_axis) {
            uint32_t num_axis_steps = std::max(1, int(std::ceil(  axis_lengths[axis_idx] / step_length[shortest_axis] ) ) );
            step_length[axis_idx] = axis_lengths[axis_idx] / num_axis_steps;

            steps_per_axis[axis_idx] = num_axis_steps;
        }
    }


    std::vector< std::vector<surfel> > spatially_divided_surfels;
    
    spatially_divided_surfels.resize( steps_per_axis[0]*steps_per_axis[1]*steps_per_axis[2] );


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


            uint32_t x_summand = std::max(uint8_t(0), std::min( uint8_t( steps_per_axis[0] - 1), uint8_t( ( current_surfel.pos()[0] - combined_bounding_box.min()[0] ) /
                                 ( combined_bounding_box.max()[0] - combined_bounding_box.min()[0] ) *
                                 steps_per_axis[0]) ) ) * steps_per_axis[1] * steps_per_axis[2];

            uint32_t y_summand = std::max(uint8_t(0), std::min( uint8_t(steps_per_axis[1] - 1), uint8_t( ( current_surfel.pos()[1] - combined_bounding_box.min()[1] ) /
                                 ( combined_bounding_box.max()[1] - combined_bounding_box.min()[1] ) *
                                 steps_per_axis[1]) ) ) * steps_per_axis[2];

            uint32_t z_summand = std::max(uint8_t(0), std::min( uint8_t(steps_per_axis[2] - 1), uint8_t( ( current_surfel.pos()[2] - combined_bounding_box.min()[2] ) /
                                 ( combined_bounding_box.max()[2] - combined_bounding_box.min()[2] ) *
                                 steps_per_axis[2]) ) );

            uint32_t box_id = x_summand + y_summand + z_summand;


            spatially_divided_surfels[box_id].push_back(current_surfel);
            original_surfels.push_back(current_surfel);
       }
    }

    std::sort(std::begin(spatially_divided_surfels), std::end(spatially_divided_surfels), [](std::vector<surfel> const& lhs, std::vector<surfel> const& rhs) { return lhs.size() > rhs.size();});    

    while( spatially_divided_surfels.back().empty() ){
        spatially_divided_surfels.pop_back();
    } 

    std::set<size_t> picked_box_ids;

    while( already_picked_surfel.size () < surfels_per_node ) {

        size_t box_id;

        do {
            box_id = rand() % spatially_divided_surfels.size();
        } while(picked_box_ids.find(box_id) != picked_box_ids.end());

        picked_box_ids.insert(box_id);
 
        if( picked_box_ids.size() == spatially_divided_surfels.size() ) {
            picked_box_ids.clear();
        }


        auto& current_surfel_box = spatially_divided_surfels[box_id];

        int32_t surf_id;

        surf_id = rand() % current_surfel_box.size();

        surfel picked_surfel = current_surfel_box[surf_id];

        already_picked_surfel.push_back(picked_surfel);

        current_surfel_box.erase(current_surfel_box.begin() + surf_id);
        
        if(current_surfel_box.empty()) {
            spatially_divided_surfels.erase( spatially_divided_surfels.begin() + box_id);

            if(picked_box_ids.find(box_id) != picked_box_ids.end())
                picked_box_ids.erase(box_id);
        }

    }

    for (auto& final_candidate : already_picked_surfel) {

        interpolate_approx_natural_neighbours(final_candidate, original_surfels, tree);

        mem_array.mem_data()->push_back(final_candidate);
    }

    mem_array.set_length(mem_array.mem_data()->size());

    reduction_error = 0.0;

    return mem_array;
}

} // namespace pre
} // namespace lamure
