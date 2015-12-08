// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/reduction_entropy.h>

//#include <math.h>
#include <functional>
#include <numeric>
#include <vector>
#include <queue>

namespace lamure {
namespace pre {


//std::vector<reduction_entropy::entropy_surfel> entropy_struct_vec;     

float reduction_entropy::
compute_entropy(uint16_t level,  vec3f own_normal, std::vector<std::pair<surfel, real>> neighbours)
{   
    float entropy = 0.0;
    //float nad = std::inner_produc(first_normal.begin(), first_normal.end(), second_normal.begin(), 0);
    for(auto const& neighbour : neighbours){
    vec3f neighbour_normal =   (neighbour.first).normal();  
    float nad = scm::math::dot(own_normal, neighbour_normal);
    entropy += (1 + level)/(1 + nad);
    };    

    return entropy;
}

vec3r reduction_entropy::
average_position(std::vector<std::pair<surfel, real>> neighbours){
    vec3r cat;
    for(auto const& neighbour : neighbours){
    cat += (neighbour.first).pos();   
    }

    cat = cat/neighbours.size();
    return cat;
} 

vec3f reduction_entropy::
average_normal(std::vector<std::pair<surfel, real>> neighbours){
    vec3f cat;
    for(auto const& neighbour : neighbours){
    cat += sqrt(neighbour.second);
    }

    cat /= neighbours.size();
    return cat;
} 


real reduction_entropy::
average_radius(std::vector<std::pair<surfel, real>> neighbours){

    real cat;
    for(auto const& neighbour : neighbours){
    cat += sqrt(neighbour.second);
    }

    cat /= neighbours.size();
    return cat;
} 

void reduction_entropy::
merge(entropy_surfel& current_entropy_struct, 
      std:: priority_queue <reduction_entropy::entropy_surfel, std::vector<reduction_entropy::entropy_surfel>, min_entropy_order>& pq){

    current_entropy_struct.level = update_level(current_entropy_struct.level);
    current_entropy_struct.current_surfel.pos() = average_position(current_entropy_struct.neighbours);
    current_entropy_struct.current_surfel.normal() = average_normal(current_entropy_struct.neighbours);
    current_entropy_struct.current_surfel.radius() = average_radius(current_entropy_struct.neighbours);

    //for(auto const& neighbour : current_entropy_struct.neighbours){
    //current_entropy_struct =    
    //}
}

void reduction_entropy::
initialize_queue(const std::vector<surfel_mem_array*>& input, const bvh& tree){
    //initialize minimal priority queue
    std:: priority_queue <reduction_entropy::entropy_surfel, std::vector<reduction_entropy::entropy_surfel>, min_entropy_order> min_pq;
    entropy_surfel current_entropy_struct {};
    surfel current_surfel; //
    std::vector<std::pair<surfel, real>> neighbours; //

    for (size_t i = 0; i < input.size(); ++i){
        for (size_t j = 0; j < input[0]->length(); ++j){
            int counter = 0;
            neighbours = tree.get_nearest_neighbours(i, j, number_of_neighbours_);
            current_surfel = input[i]->mem_data()->at(j);
            current_entropy_struct.current_surfel = current_surfel;
            current_entropy_struct.id = counter;
            current_entropy_struct.neighbours = neighbours;
            current_entropy_struct.validity = true;
            current_entropy_struct.level = 0;
            current_entropy_struct.entropy = compute_entropy(0, current_surfel.normal(), neighbours);
            min_pq.push(current_entropy_struct);
            counter++;
        }
    };

    for (size_t j = 0; j < input[0]->length(); ++j){
        if(!min_pq.empty()){
        current_entropy_struct =  min_pq.top();
        min_pq.pop();
            if(current_entropy_struct.validity){
                merge(current_entropy_struct, min_pq);
                min_pq.push(current_entropy_struct);
            }    
        }
    }

};

/////////////////////not used; still old code //////////////////////////// 
surfel_mem_array reduction_entropy::
create_lod(real& reduction_error,
          const std::vector<surfel_mem_array*>& input,
          const uint32_t surfels_per_node) const
{
    surfel_mem_array mem_array(std::make_shared<surfel_vector>(surfel_vector()), 0, 0);



    const uint32_t fan_factor =  input.size();

    

   
    //create lod from input
    for (size_t i = 0; i < input.size(); ++i) {
        for (size_t j = 0; j < input[0]->length(); ++j) {

            auto surfel = input[i]->mem_data()->at(j);

           
            mem_array.mem_data()->push_back(surfel);
        }
    }

    mem_array.set_length(mem_array.mem_data()->size());

    reduction_error = 0.0;

    return mem_array;
};



} // namespace pre
} // namespace lamure
