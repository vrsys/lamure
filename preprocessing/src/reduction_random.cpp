// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/reduction_random.h>
#include <lamure/pre/surfel.h>
#include <set>

namespace lamure {
namespace pre {

surfel_mem_array reduction_random::
create_lod(real& reduction_error,
          const std::vector<surfel_mem_array*>& input,
          const uint32_t surfels_per_node) const
{
    surfel_mem_array mem_array(std::make_shared<surfel_vector>(surfel_vector()), 0, 0);

    const uint32_t fan_factor = input.size();
    size_t point_id = 0;

    std::set <size_t> random_id; 
    std::set <size_t>::iterator set_it;

    size_t summed_length_of_nodes = 0;

    //compute max total number of surfels from all nodes
    for ( size_t node_id = 0; node_id < fan_factor; ++node_id) {
        summed_length_of_nodes += input[node_id]->length();
    }
    
    //draw random number within interval 0 to total number of surfels
    for (size_t i = 0; i < input[0]->length(); ++i){
        point_id = rand() % summed_length_of_nodes;

        //check, if point_id repeats
        set_it = random_id.find(point_id);

        //if so, draw another random number 
        if (set_it != random_id.end()){
            do {
                point_id = rand() % (input[1]->length()*fan_factor);   
                set_it = random_id.find(point_id);
                random_id.insert (point_id);
            } while(set_it != random_id.end());
        }

        //store random number
        random_id.insert (point_id);                  
                     

        //compute node id depending on randomly generated int value
        size_t node_id = 0;

        for( ; node_id < fan_factor; ++node_id) {
            if ((point_id) < input[node_id]->length()){
                break;
            } else {
                point_id -= input[node_id]->length();
            }
        }

        auto surfel = input[node_id]->mem_data()->at(point_id + input[node_id]->offset());
        mem_array.mem_data()->push_back(surfel);            
    }

    mem_array.set_length(mem_array.mem_data()->size());

    reduction_error = 0.0;

    return mem_array;
}

} // namespace pre
} // namespace lamure
