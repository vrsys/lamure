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
compute_entropy(uint16_t level,  vec3f own_normal, std::vector<neighbour_distance_t> neighbours) const
{   
    float entropy = 0.0;
    
    //float nad = std::inner_produc(first_normal.begin(), first_normal.end(), second_normal.begin(), 0);
    for(auto const& neighbour : neighbours){
    vec3f neighbour_normal =   (*neighbour.surfel_ptr).normal();  
    float nad = scm::math::dot(own_normal, neighbour_normal);
    entropy += (1 + level)/(1 + nad);
    };    
    

    return entropy;
}

vec3r reduction_entropy::
average_position(std::vector<neighbour_distance_t> neighbours) const{
    
    vec3r new_position;
    real weight_sum = 0.f;
    //partally taken from reduction_ndc
    for(auto const& neighbour : neighbours){
        real weight = 1.0; //surfel.radius();
        weight_sum += weight;

        new_position += weight * (*neighbour.surfel_ptr).pos();
    }

    if( weight_sum != 0.0 ) {
        new_position /= weight_sum;
    } else {
        new_position = vec3r(0.0,0.0,0.0);
    }
    return new_position;
} 

vec3f reduction_entropy::
average_normal(std::vector<neighbour_distance_t> neighbours) const{
    vec3f new_normal(0.0, 0.0, 0.0);

    real weight_sum = 0.f;

    for(auto const& neighbour : neighbours){
        real weight = 1.0; //surfel.radius();
        weight_sum += weight;

        new_normal += weight * (*neighbour.surfel_ptr).normal();
    }

    if( weight_sum != 0.0 ) {
        new_normal /= neighbours.size();
    } else {
        new_normal = vec3r(0.0, 0.0, 0.0);
    }

    return new_normal;
} 


real reduction_entropy::
average_radius(std::vector<neighbour_distance_t> neighbours) const{

    real new_radius = 0.0;
    for(auto const& neighbour : neighbours){
        new_radius += sqrt(neighbour.neighbour_distance);
    }
    if(new_radius != 0.0) {
        new_radius /= neighbours.size(); //times 0.8??
    }

    return new_radius;
} 

void reduction_entropy::
merge(entropy_surfel& current_entropy_struct, 
      std:: priority_queue <reduction_entropy::entropy_surfel, std::vector<reduction_entropy::entropy_surfel>, min_entropy_order>& pq) const{

    //recompute values for merged suefel
    current_entropy_struct.level = update_level(current_entropy_struct.level);
    current_entropy_struct.current_surfel.pos() = average_position(current_entropy_struct.neighbours);
    current_entropy_struct.current_surfel.normal() = average_normal(current_entropy_struct.neighbours);
    current_entropy_struct.current_surfel.radius() = average_radius(current_entropy_struct.neighbours);
    
    
    std::vector<reduction_entropy::entropy_surfel>& underlying_vector = Container(pq);

    //mark neighbours as invalid
    //reduction_entropy::entropy_surfel potential_neighbour;
    for(auto& potential_neighbour : underlying_vector){
        //potential_neighbour = pq.top();
        
        //current_entropy_struct.current_surfel = neighbour.first;
       for (auto const& neighbour : current_entropy_struct.neighbours){
            
            if ((potential_neighbour.id == neighbour.surfel_node_id.surfel_idx) && potential_neighbour.validity){
                potential_neighbour.validity = false;
            }

        }
    }

    //update entropy
    pq.push(current_entropy_struct);
}


// get k nearest neighbours in simplification nodes
std::vector<std::pair<surfel_id_t, real>> reduction_entropy::
get_local_nearest_neighbours(const std::vector<surfel_mem_array*>& input,
                             size_t num_local_neighbours,
                             surfel_id_t const& target_surfel) const {

    //size_t current_node = target_surfel.node_idx;
    vec3r center = input[target_surfel.node_idx]->read_surfel(target_surfel.surfel_idx).pos();

    std::vector<std::pair<surfel_id_t, real>> candidates;
    real max_candidate_distance = std::numeric_limits<real>::infinity();

    for (size_t node_id = 0; node_id < input.size(); ++node_id) {
        for (size_t i = 0; i < input[node_id]->length(); ++i) {
            if (i != target_surfel.surfel_idx) {
                const surfel& current_surfel = input[node_id]->read_surfel(i);
                real distance_to_center = scm::math::length_sqr(center - current_surfel.pos());

                if (candidates.size() < num_local_neighbours || (distance_to_center < max_candidate_distance)) {
                    if (candidates.size() == num_local_neighbours)
                        candidates.pop_back();

                    candidates.push_back(std::make_pair(surfel_id_t(node_id, i), distance_to_center));

                    for (uint16_t k = candidates.size() - 1; k > 0; --k) {
                        if (candidates[k].second < candidates[k - 1].second) {
                            std::swap(candidates[k], candidates[k - 1]);
                        }
                        else
                            break;
                    }

                    max_candidate_distance = candidates.back().second;
                }
            }
        }
    }

    return candidates;
}

surfel_mem_array reduction_entropy::
create_lod(real& reduction_error,
          const std::vector<surfel_mem_array*>& input,
          const uint32_t surfels_per_node) const
{
    surfel_mem_array mem_array(std::make_shared<surfel_vector>(surfel_vector()), 0, 0);

    //initialize minimal priority queue
    std:: priority_queue <reduction_entropy::entropy_surfel, std::vector<reduction_entropy::entropy_surfel>, min_entropy_order> min_pq;
    entropy_surfel current_entropy_surfel {};
    surfel current_surfel; //
    std::vector<neighbour_distance_t> neighbours_vec; 

    for (size_t node_id = 0; node_id < input.size(); ++node_id){
        for (size_t surfel_id = 0; surfel_id < input[0]->length()-1; ++surfel_id){
            uint32_t counter = 0;
            
            std::vector<std::pair<surfel_id_t, real>> nearest_neighbours_ids = get_local_nearest_neighbours(input, input.size()-1 ,surfel_id_t(node_id, surfel_id) );



            for (auto const& neighbour_id_pair : nearest_neighbours_ids){
                auto const neighbor_node_id = neighbour_id_pair.first.node_idx;
                auto const neighbor_surfel_id = neighbour_id_pair.first.surfel_idx;

                auto const& current_surfel_array = input[neighbor_node_id];
                size_t surfel_array_offset = current_surfel_array->offset();
                auto const& current_surfel = current_surfel_array->mem_data()->at(surfel_array_offset + neighbor_surfel_id);
                

                auto const& neighbour_distance = neighbour_id_pair.second;
                neighbour_distance_t current_neighbour_with_id(&current_surfel, neighbour_distance, neighbour_id_pair.first);//

                neighbours_vec.push_back(current_neighbour_with_id);
            };
         


            current_surfel = input[node_id]->mem_data()->at(input[node_id]->offset() + surfel_id);
            current_entropy_surfel.current_surfel = current_surfel;
            current_entropy_surfel.id = counter;
            current_entropy_surfel.neighbours = neighbours_vec;
            current_entropy_surfel.validity = true;
            current_entropy_surfel.level = 0;
            current_entropy_surfel.entropy = compute_entropy(0, current_surfel.normal(), neighbours_vec);
            //min_pq.push(current_entropy_surfel);
            ++counter;
        }
    };

    for (size_t j = 0; j < input[input.size()]->length(); ++j){
        if(!min_pq.empty()){
        current_entropy_surfel =  min_pq.top();
        min_pq.pop();
            if(current_entropy_surfel.validity){
                merge(current_entropy_surfel, min_pq);
                min_pq.push(current_entropy_surfel);
            }    
        }
    }


    //end of entropy simplification

    while(! min_pq.empty()) {

        entropy_surfel const& en_surfel_to_push = min_pq.top();
        mem_array.mem_data()->push_back(en_surfel_to_push.current_surfel);
        min_pq.pop();
    }



    mem_array.set_length(mem_array.mem_data()->size());

    reduction_error = 0.0;

    return mem_array;
};



} // namespace pre
} // namespace lamure
