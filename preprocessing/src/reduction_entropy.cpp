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
average_position(std::vector<reduction_entropy::neighbour_with_id> neighbours){
    
    vec3r new_position;
    real weight_sum = 0.f;
    //partally taken from reduction_ndc
    for(auto const& neighbour : neighbours){
    real weight = 1.0; //surfel.radius();
    weight_sum += weight;
    new_position += weight * neighbour.neighbour_surfel.pos();
    }
    new_position /= weight_sum;
    return new_position;
} 

vec3f reduction_entropy::
average_normal(std::vector<reduction_entropy::neighbour_with_id> neighbours){
    vec3f new_normal;
    for(auto const& neighbour : neighbours){
    new_normal += neighbour.neighbour_surfel.normal();
    }

    new_normal /= neighbours.size();
    return new_normal;
} 


real reduction_entropy::
average_radius(std::vector<reduction_entropy::neighbour_with_id> neighbours){

    real new_radius;
    for(auto const& neighbour : neighbours){
    new_radius += sqrt(neighbour.distance_to_neighbour);
    }

    new_radius /= neighbours.size(); //times 0.8??
    return new_radius;
} 

void reduction_entropy::
merge(entropy_surfel& current_entropy_struct, 
      std:: priority_queue <reduction_entropy::entropy_surfel, std::vector<reduction_entropy::entropy_surfel>, min_entropy_order>& pq){

    //recompute values for merged suefel
    current_entropy_struct.level = update_level(current_entropy_struct.level);
    current_entropy_struct.current_surfel.pos() = average_position(current_entropy_struct.neighbours);
    current_entropy_struct.current_surfel.normal() = average_normal(current_entropy_struct.neighbours);
    current_entropy_struct.current_surfel.radius() = average_radius(current_entropy_struct.neighbours);
    
    
    std::vector<reduction_entropy::entropy_surfel>& underlying_vector = Container(pq);

    //mark neighbours as invalid
    //reduction_entropy::entropy_surfel potential_neighbour;
    for(auto const& potential_neighbour : underlying_vector){
        //potential_neighbour = pq.top();
        
        //current_entropy_struct.current_surfel = neighbour.first;
       for (auto const& neighbour : current_entropy_struct.neighbours){
            
            if ((potential_neighbour.id == neighbour.id) && potential_neighbour.validity){
                potential_neighbour.validity = false;
            }

        }
    }

    //update entropy
    pq.push(current_entropy_struct);
}

void reduction_entropy::
initialize_queue(const std::vector<surfel_mem_array*>& input, const bvh& tree){
    //initialize minimal priority queue
    std:: priority_queue <reduction_entropy::entropy_surfel, std::vector<reduction_entropy::entropy_surfel>, min_entropy_order> min_pq;
    entropy_surfel current_entropy_struct {};
    surfel current_surfel; //
    std::vector<neighbour_with_id> neighbours_struct_vec; 
    neighbour_with_id current_neighbour_with_id;//

    for (size_t i = 0; i < input.size(); ++i){
        for (size_t j = 0; j < input[0]->length(); ++j){
            uint32_t counter = 0;
            std::vector<std::pair<surfel, real>> nearest_neighbours = tree.get_nearest_neighbours(i, j, number_of_neighbours_);
            for (auto const& neighbour : nearest_neighbours){
                current_neighbour_with_id.neighbour_surfel = neighbour.first;
                current_neighbour_with_id.distance_to_neighbour = neighbour.second;
                current_neighbour_with_id.id = counter;

                neighbours_struct_vec.push_back(current_neighbour_with_id);
            };
         
            current_surfel = input[i]->mem_data()->at(j);
            current_entropy_struct.current_surfel = current_surfel;
            current_entropy_struct.id = counter;
            current_entropy_struct.neighbours = neighbours_struct_vec;
            current_entropy_struct.validity = true;
            current_entropy_struct.level = 0;
            current_entropy_struct.entropy = compute_entropy(0, current_surfel.normal(), neighbours);
            //min_pq.push(current_entropy_struct);
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
