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
 
std::vector<reduction_entropy::entropy_surfel*> const reduction_entropy::
get_locally_overlapping_neighbours(entropy_surfel const& target_entropy_surfel,
                                   std::vector<entropy_surfel>& entropy_surfel_array) const {
    surfel* target_surfel = target_entropy_surfel.current_surfel; 
    real target_surfel_radius = target_surfel->radius();

    std::vector<entropy_surfel*> overlapping_neighbour_ptrs;

    for ( auto& array_entr_surfel : entropy_surfel_array ) {
        
	// avoid overlaps with the surfel itself
        if (target_entropy_surfel.surfel_id != array_entr_surfel.surfel_id && 
	    target_entropy_surfel.node_id   != array_entr_surfel.node_id) {

	    surfel* current_surfel = array_entr_surfel.current_surfel;
            real distance_to_target_surfel = scm::math::length(target_surfel->pos() - current_surfel->pos());
                
            real neighbour_radius = current_surfel->radius();

            if( distance_to_target_surfel <= target_surfel_radius + neighbour_radius ) {
                // keep address of entropy surfel in vector
                overlapping_neighbour_ptrs.push_back(&array_entr_surfel);                    
            }
        }
        
    }

    return overlapping_neighbour_ptrs;
}

// to verify: the center of mass is the point that allows for the minimal enclosing sphere
vec3r reduction_entropy::
compute_center_of_mass(std::vector<entropy_surfel*>& neighbour_ptrs) const {

    //volume of a sphere (4/3) * pi * r^3

    vec3r center_of_mass_enumerator = vec3r(0.0, 0.0, 0.0);
    real center_of_mass_denominator = 0.0;

    //center of mass equation: c_o_m = ( sum_of( m_i*x_i) ) / ( sum_of(m_i) )
    for (auto curr_neighbour_ptr : neighbour_ptrs) {

	surfel* current_surfel = curr_neighbour_ptr->current_surfel;

        real neighbour_radius = current_surfel->radius();

        real neighbour_mass = (4.0/3.0) * M_PI * 
                                neighbour_radius * neighbour_radius * neighbour_radius;

        center_of_mass_enumerator += neighbour_mass * current_surfel->pos();

        center_of_mass_denominator += neighbour_mass;
    }

    return center_of_mass_enumerator / center_of_mass_denominator;
}

real reduction_entropy::
compute_enclosing_sphere_radius(vec3r const& surfel_pos, std::vector<entropy_surfel*> const& neighbour_ptrs) const {

    real enclosing_radius = 0.0;

    for (auto const curr_neighbour_ptr : neighbour_ptrs) {

	surfel* current_surfel = curr_neighbour_ptr->current_surfel;
        real neighbour_enclosing_radius = scm::math::length(surfel_pos - current_surfel->pos()) + current_surfel->radius();
        
        if(neighbour_enclosing_radius > enclosing_radius) {
            enclosing_radius = neighbour_enclosing_radius;
        }
    }

    return enclosing_radius;
}

float reduction_entropy::
compute_entropy(uint16_t level,  vec3f const& own_normal, std::vector<entropy_surfel*> const& neighbour_ptrs) const
{   
    float entropy = 0.0;
    
    for(auto const curr_neighbour_ptr : neighbour_ptrs){
	surfel* current_surfel = curr_neighbour_ptr->current_surfel;

        vec3f neighbour_normal = current_surfel->normal();  
        float normal_angle = scm::math::dot(own_normal, neighbour_normal);
        entropy += (1 + level)/(1.0 + normal_angle);
    };    
    
    return entropy;
}

/*
vec3r reduction_entropy::
average_position(std::vector<entropy_surfel*> const& neighbours) const{
    
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
*/

vec3f reduction_entropy::
average_normal(std::vector<entropy_surfel*> const& neighbour_ptrs) const{
    vec3f new_normal(0.0, 0.0, 0.0);

    real weight_sum = 0.f;

    for(auto const neighbour_ptr : neighbour_ptrs){
	surfel* current_surfel = neighbour_ptr->current_surfel;

        real weight = 1.0; //surfel.radius();
        weight_sum += weight;

        new_normal += weight * current_surfel->normal();
    }

    if( weight_sum != 0.0 ) {
        new_normal /= neighbour_ptrs.size();
    } else {
        new_normal = vec3r(0.0, 0.0, 0.0);
    }

    return new_normal;
} 

/*
real reduction_entropy::
average_radius(std::vector<entropy_surfel*> const& neighbours) const{

    real new_radius = 0.0;
    for(auto const& neighbour : neighbours){
        new_radius += sqrt(neighbour.neighbour_distance);
    }
    if(new_radius != 0.0) {
        new_radius /= neighbours.size(); //times 0.8??
    }

    return new_radius;
} 
*/

void reduction_entropy::
merge(entropy_surfel* current_entropy_surfel,
      std::vector<entropy_surfel*>& entropy_surfel_array,
      //std::priority_queue<reduction_entropy::entropy_surfel*, std::vector<reduction_entropy::entropy_surfel*>, min_entropy_surfel_ptr_order>& pq,
      size_t num_remaining_valid_surfel, size_t num_desired_surfel) const{

    //recompute values for merged surfel
    current_entropy_surfel->level += 1;

    surfel* current_surfel = current_entropy_surfel->current_surfel;
    //current_entropy_surfel.current_surfel->pos() = average_position(current_entropy_surfel.neighbours);

    vec3r center_of_neighbour_masses = compute_center_of_mass(current_entropy_surfel->neighbours);
    current_surfel->pos() = center_of_neighbour_masses;
    current_surfel->normal() = average_normal(current_entropy_surfel->neighbours);
    //current_entropy_surfel.current_surfel->radius() = average_radius(current_entropy_surfel.neighbours);
    current_surfel->radius() = compute_enclosing_sphere_radius(center_of_neighbour_masses, current_entropy_surfel->neighbours); 
    
    //std::vector<reduction_entropy::entropy_surfel*>& pq_entr_surfel_ptrs = Container(pq);

    //mark neighbours as invalid
    //reduction_entropy::entropy_surfel potential_neighbour;

    size_t num_invalidated_surfels = 0;

    bool reached_simplification_limit = false;

    //std::vector<
    for(auto potential_neighbour_ptr : entropy_surfel_array){
       for (auto const& actual_neighbour_ptr : current_entropy_surfel->neighbours){
            
            if ((potential_neighbour_ptr->surfel_id == actual_neighbour_ptr->surfel_id) &&
                 potential_neighbour_ptr->node_id   == actual_neighbour_ptr->node_id && 
                 potential_neighbour_ptr->validity){
                
                 potential_neighbour_ptr->validity = false;

                 if( num_remaining_valid_surfel - (--num_invalidated_surfels) <= num_desired_surfel ) {

                    reached_simplification_limit = true;
                    break;
                 }
            }
        }

        if(reached_simplification_limit)
            break;
    }

    //current_entropy_surfel.entropy = compute_entropy(current_entropy_surfel.level, current_entropy_surfel.current_surfel->normal(), current_entropy_surfel.neighbours);
    //update entropy
    //pq.push(current_entropy_surfel);
    entropy_surfel_array.push_back(current_entropy_surfel);
}

surfel_mem_array reduction_entropy::
create_lod(real& reduction_error,
          const std::vector<surfel_mem_array*>& input,
          const uint32_t surfels_per_node) const
{
    surfel_mem_array mem_array(std::make_shared<surfel_vector>(surfel_vector()), 0, 0);

    std::vector<entropy_surfel> entropy_surfel_array;

    std::vector<entropy_surfel*> min_entropy_surfel_ptr_queue;

    for (size_t node_id = 0; node_id < input.size(); ++node_id){
        for (size_t surfel_id = 0; surfel_id < input[node_id-1]->length()-1; ++surfel_id){
            
	        auto& current_surfel = input[node_id]->mem_data()->at(input[node_id]->offset() + surfel_id);
            
            entropy_surfel current_entropy_surfel;
            current_entropy_surfel.current_surfel = &current_surfel;
            current_entropy_surfel.surfel_id = surfel_id;
            current_entropy_surfel.node_id   = node_id;
            current_entropy_surfel.validity = true;
            current_entropy_surfel.level = 0;
            
	    entropy_surfel_array.push_back(current_entropy_surfel);
	    //min_pq.push(current_entropy_surfel);
	   }
    }	

    for ( auto& current_entropy_surfel : entropy_surfel_array ){
            
            std::vector<reduction_entropy::entropy_surfel*> overlapping_neighbour_ptrs = get_locally_overlapping_neighbours(current_entropy_surfel, entropy_surfel_array);

            current_entropy_surfel.neighbours = overlapping_neighbour_ptrs;
            current_entropy_surfel.validity = true;
            current_entropy_surfel.level = 0;
            current_entropy_surfel.entropy = compute_entropy(current_entropy_surfel.level, current_entropy_surfel.current_surfel->normal(), overlapping_neighbour_ptrs);
            min_entropy_surfel_ptr_queue.push_back(&current_entropy_surfel);
    }

    size_t num_valid_surfels = min_entropy_surfel_ptr_queue.size();

    while(true) {
        //the back element is still valid, so we still got something to do
        if( min_entropy_surfel_ptr_queue.back()->validity ){
            entropy_surfel* current_entropy_surfel = min_entropy_surfel_ptr_queue.back();
            
            min_entropy_surfel_ptr_queue.pop_back();

            if(current_entropy_surfel->validity){
                /*num_valid_surfels -=*/merge(current_entropy_surfel, min_entropy_surfel_ptr_queue, num_valid_surfels, surfels_per_node);
                //min_pq.push(current_entropy_surfel);
            }    
        } else {
            break;
        }
        
    }

    //end of entropy simplification
    while( (!min_entropy_surfel_ptr_queue.empty()) && min_entropy_surfel_ptr_queue.back()->validity ) {
        entropy_surfel* en_surfel_to_push = min_entropy_surfel_ptr_queue.back();
        mem_array.mem_data()->push_back(*(en_surfel_to_push->current_surfel));
        
        min_entropy_surfel_ptr_queue.pop_back();
        //std::sort(min_entropy_surfel_ptr_queue.begin(), min_entropy_surfel_ptr_queue.end(), min_entropy_order());
    }


    mem_array.set_length(mem_array.mem_data()->size());

    reduction_error = 0.0;

    return mem_array;
};



} // namespace pre
} // namespace lamure
