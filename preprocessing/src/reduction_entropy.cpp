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
#include <map>
#include <set>

namespace lamure {
namespace pre {
 
std::vector<std::shared_ptr<entropy_surfel> > const reduction_entropy::
get_locally_overlapping_neighbours(std::shared_ptr<entropy_surfel> target_entropy_surfel_ptr,
                                   std::vector<std::shared_ptr<entropy_surfel> >& entropy_surfel_ptr_array,
                                   float overlap_radius_factor) const {
    std::shared_ptr<surfel> target_surfel = target_entropy_surfel_ptr->contained_surfel; 
    real target_surfel_radius = target_surfel->radius() * overlap_radius_factor;

    //std::cout << "target_surfel_radius: " << target_surfel_radius << "\n";
    std::vector< std::shared_ptr<entropy_surfel> > overlapping_neighbour_ptrs;

    for ( auto& array_entr_surfel_ptr : entropy_surfel_ptr_array ) {
        
	// avoid overlaps with the surfel itself
        if (target_entropy_surfel_ptr->surfel_id != array_entr_surfel_ptr->surfel_id || 
	    target_entropy_surfel_ptr->node_id   != array_entr_surfel_ptr->node_id) {

	    std::shared_ptr<surfel> current_surfel = array_entr_surfel_ptr->contained_surfel;
            real distance_to_target_surfel = scm::math::length(target_surfel->pos() - current_surfel->pos());
                
            real neighbour_radius = current_surfel->radius() * overlap_radius_factor;
            /*std::cout << "neighbour surfel radius: " << neighbour_radius << "\n";
            std::cout << "Distance to target surfel: " << distance_to_target_surfel << "\n";

            std::cout << "Target surfel pos: " << target_surfel->pos() << "\n";
            std::cout << "Neighbour surfel pos: " << current_surfel->pos() << "\n";
            std::cout << "subtraction: " << target_surfel->pos() - current_surfel->pos() << "\n";
            */
            if( distance_to_target_surfel <= target_surfel_radius + neighbour_radius ) {
                // keep address of entropy surfel in vector
                overlapping_neighbour_ptrs.push_back( array_entr_surfel_ptr );                    
            }
        }
        
    }

    return overlapping_neighbour_ptrs;
}

// to verify: the center of mass is the point that allows for the minimal enclosing sphere
vec3r reduction_entropy::
compute_center_of_mass(std::shared_ptr<surfel> current_surfel, std::vector<std::shared_ptr<entropy_surfel> >& neighbour_ptrs) const {

    //volume of a sphere (4/3) * pi * r^3
    real current_surfel_radius = current_surfel->radius();
    real current_surfel_mass = (4.0/3.0) * M_PI * current_surfel_radius * current_surfel_radius * current_surfel_radius;

    vec3r center_of_mass_enumerator = current_surfel_mass * current_surfel->pos();
    real center_of_mass_denominator = current_surfel_mass;

    //center of mass equation: c_o_m = ( sum_of( m_i*x_i) ) / ( sum_of(m_i) )
    for (auto curr_neighbour_ptr : neighbour_ptrs) {

	std::shared_ptr<surfel> current_neighbour_surfel = curr_neighbour_ptr->contained_surfel;

        real neighbour_radius = current_neighbour_surfel->radius();

        real neighbour_mass = (4.0/3.0) * M_PI * 
                                neighbour_radius * neighbour_radius * neighbour_radius;

        center_of_mass_enumerator += neighbour_mass * current_neighbour_surfel->pos();

        center_of_mass_denominator += neighbour_mass;
    }

    return center_of_mass_enumerator / center_of_mass_denominator;
}

real reduction_entropy::
compute_enclosing_sphere_radius(vec3r const& center_of_mass, std::shared_ptr<surfel> current_surfel, std::vector< std::shared_ptr<entropy_surfel> > const& neighbour_ptrs) const {

    real enclosing_radius = 0.0;

    enclosing_radius = scm::math::length(center_of_mass - current_surfel->pos()) + current_surfel->radius();

    for (auto const curr_neighbour_ptr : neighbour_ptrs) {

	std::shared_ptr<surfel> current_neighbour_surfel = curr_neighbour_ptr->contained_surfel;
        real neighbour_enclosing_radius = scm::math::length(center_of_mass - current_neighbour_surfel->pos()) + current_neighbour_surfel->radius();
        
        if(neighbour_enclosing_radius > enclosing_radius) {
            enclosing_radius = neighbour_enclosing_radius;
        }
    }

    return enclosing_radius;
}

float reduction_entropy::
compute_entropy(std::shared_ptr<entropy_surfel> current_en_surfel, std::vector<std::shared_ptr<entropy_surfel> > const& neighbour_ptrs) const
{   
    double entropy = 0.0;
    
    vec3b const& current_color = current_en_surfel->contained_surfel->color();

    for(auto const curr_neighbour_ptr : neighbour_ptrs){
	   std::shared_ptr<surfel> current_neighbour_surfel = curr_neighbour_ptr->contained_surfel;

        vec3f const& neighbour_normal = current_neighbour_surfel->normal();  

        vec3b const& neighbour_color = current_neighbour_surfel->color();

        float normal_angle = scm::math::dot(current_en_surfel->contained_surfel->normal(), neighbour_normal);
        entropy += (1 + current_en_surfel->level)/(1.0 + normal_angle);

        real color_diff = scm::math::length( vec3r((current_color[0] - neighbour_color[0]),
                                                   (current_color[1] - neighbour_color[1]),
                                                   (current_color[2] - neighbour_color[2]) ) ) ;

        entropy += (1 + current_en_surfel->level) * (color_diff) ;
    };    
    
    //std::cout << "Entropy for " << " to entropy surfel (ids: ["<< current_en_surfel->node_id << "]["<<current_en_surfel->surfel_id<<"]: "<< entropy <<"\n";

    return entropy;
}

vec3b reduction_entropy::
average_color(std::shared_ptr<surfel> current_surfel, std::vector<std::shared_ptr<entropy_surfel> > const& neighbour_ptrs) const{

    vec3r accumulated_color(0.0, 0.0, 0.0);
    double accumulated_weight = 0.0;

    accumulated_color = current_surfel->color();
    accumulated_weight = 1.0;

    for(auto const curr_neighbour_ptr : neighbour_ptrs){
        accumulated_weight += 1.0;
        accumulated_color += curr_neighbour_ptr->contained_surfel->color();
    }

    vec3b normalized_color = vec3b(accumulated_color[0] / accumulated_weight,
                                   accumulated_color[1] / accumulated_weight,
                                   accumulated_color[2] / accumulated_weight );
    return normalized_color;
}

vec3f reduction_entropy::
average_normal(std::shared_ptr<surfel> current_surfel, std::vector<std::shared_ptr<entropy_surfel> > const& neighbour_ptrs) const{
    vec3f new_normal(0.0, 0.0, 0.0);

    real weight_sum = 0.f;

    new_normal = current_surfel->normal();
    weight_sum = 1.0;   

    for(auto const neighbour_ptr : neighbour_ptrs){
	std::shared_ptr<surfel> current_surfel = neighbour_ptr->contained_surfel;

        real weight = 1.0; //surfel.radius();
        weight_sum += weight;

        new_normal += weight * current_surfel->normal();
    }

    if( weight_sum != 0.0 ) {
        new_normal /= neighbour_ptrs.size();
    } else {
        new_normal = vec3r(0.0, 0.0, 0.0);
    }


    return scm::math::normalize(new_normal);
} 

bool reduction_entropy::
merge(std::shared_ptr<entropy_surfel> current_entropy_surfel,
      size_t& num_remaining_valid_surfel, size_t num_desired_surfel) const{

    size_t num_invalidated_surfels = 0;

    bool reached_simplification_limit = false;


    std::vector<std::shared_ptr<entropy_surfel> > neighbours_to_merge;// = current_entropy_surfel->neighbours;

    //**replace own invalid neighbours by valid neighbours of invalid neighbours**
    std::map<size_t, std::set<size_t> > added_neighbours_during_merge;


    //sort neighbours by increasing distance to current neighbour
    std::sort(current_entropy_surfel->neighbours.begin(),
              current_entropy_surfel->neighbours.end(),
              [&](std::shared_ptr<entropy_surfel> const& left_entropy_surfel,
                  std::shared_ptr<entropy_surfel> const& right_entropy_surfel){

                    //double distance_to_left_surfel 

                    if(/*scm::math::length(current_entropy_surfel->contained_surfel->pos()-
                       left_entropy_surfel->contained_surfel->pos()) */
                        left_entropy_surfel->entropy
                        < 
                       /*
                       scm::math::length(current_entropy_surfel->contained_surfel->pos()-
                       right_entropy_surfel->contained_surfel->pos())*/
                        right_entropy_surfel->entropy
                    ) {
                        return true;
                    } else {
                        return false;
                    }
                }
              );


   std::vector<std::shared_ptr<entropy_surfel> > invalidated_neighbours;

   for (auto const actual_neighbour_ptr : current_entropy_surfel->neighbours){
        
        if (actual_neighbour_ptr->validity) {
            actual_neighbour_ptr->validity = false;
            /*std::cout << "Invalidated: " << "[" << actual_neighbour_ptr->node_id << "]["
                                                << actual_neighbour_ptr->surfel_id << "]\n";
            */
            invalidated_neighbours.push_back(actual_neighbour_ptr);
            ++num_invalidated_surfels;
            if( --num_remaining_valid_surfel == num_desired_surfel ) {

                reached_simplification_limit = true;
                //std::cout << "reached_simplification_limit\n";
                break;
            }
        }
    }

    for (auto const actual_neighbour_ptr : invalidated_neighbours) {

            //iterate the neighbours of the invalid neighbour
            for(auto const second_neighbour_ptr : actual_neighbour_ptr->neighbours){

                // we only have to consider valid neighbours, all the others are also our own neighbours and already invalid
                if(second_neighbour_ptr->validity) {
                    size_t n_id = second_neighbour_ptr->node_id;
                    size_t s_id = second_neighbour_ptr->surfel_id;

                    //avoid getting the surfel itself as neighbour
                    if( n_id != current_entropy_surfel->node_id || 
                        s_id != current_entropy_surfel->surfel_id) {
                        //ignore 2nd neighbours which we found already at another neighbour
                        if( (added_neighbours_during_merge[n_id]).find(s_id) == added_neighbours_during_merge[n_id].end() ) {
                            (added_neighbours_during_merge[n_id]).insert(s_id);
                            neighbours_to_merge.push_back(second_neighbour_ptr);
                        }
                    }
                }
            }

    }

    std::vector<std::shared_ptr<entropy_surfel> > current_neighbours = current_entropy_surfel->neighbours ;


    current_neighbours.insert(std::end(current_neighbours), std::begin(neighbours_to_merge), std::end(neighbours_to_merge) );

    current_entropy_surfel->neighbours = current_neighbours;


    //recompute values for merged surfel
    current_entropy_surfel->level += 1;

    std::shared_ptr<surfel> current_surfel = current_entropy_surfel->contained_surfel;

    vec3r center_of_neighbour_masses = compute_center_of_mass(current_surfel, invalidated_neighbours);
    current_surfel->pos() = center_of_neighbour_masses;
    current_surfel->normal() = average_normal(current_surfel, invalidated_neighbours);
    current_surfel->color()  = average_color(current_surfel, invalidated_neighbours);
    current_surfel->radius() = compute_enclosing_sphere_radius(center_of_neighbour_masses, current_surfel, invalidated_neighbours); 

    if(current_surfel->radius() == 0) {

    }

    current_entropy_surfel->entropy = compute_entropy(current_entropy_surfel, invalidated_neighbours);

    if(num_invalidated_surfels == 0)
        return false;
    if(!current_entropy_surfel->neighbours.empty()) {
        return true;
    } else {
        return false;
    }


}

surfel_mem_array reduction_entropy::
create_lod(real& reduction_error,
          const std::vector<surfel_mem_array*>& input,
          const uint32_t surfels_per_node) const
{
    std::cout << "Started new LOD creation\n";
    //create output array
    surfel_mem_array mem_array(std::make_shared<surfel_vector>(surfel_vector()), 0, 0);

    //container for all input surfels including entropy (entropy_surfel_array = ESA)
    std::vector<std::shared_ptr<entropy_surfel> > entropy_surfel_array;
    //priority queue as vector with min entropy surfels at the back
    std::vector<std::shared_ptr<entropy_surfel> > min_entropy_surfel_ptr_queue;

    //final surfels
    std::vector< std::shared_ptr<entropy_surfel> > finalized_surfels;

    // wrap all surfels of the input array to entropy_surfels and push them in the ESA
    for (size_t node_id = 0; node_id < input.size(); ++node_id) {
        for (size_t surfel_id = input[node_id]->offset();
                    surfel_id < input[node_id]->offset() + input[node_id]->length();
                    ++surfel_id){
            
            //this surfel will be referenced in the entropy surfel
	        auto& current_surfel = input[node_id]->mem_data()->at(input[node_id]->offset() + surfel_id);
            
            //create new entropy surfel
            entropy_surfel current_entropy_surfel(current_surfel, surfel_id, node_id);
            
            /*std::cout << "made surfel with color: " << (int)current_surfel.color()[0] << ", "
                                                    << (int)current_surfel.color()[1] << ", "
                                                    << (int)current_surfel.color()[2] << " to entropy surfel (ids: ["<< node_id << "]["<<surfel_id<<"]\n";
            */
        // only place where shared pointers should be created
	    entropy_surfel_array.push_back( std::make_shared<entropy_surfel>(current_entropy_surfel) );
	   }
    }	

    double overlap_radius_factor = 1.0;

    //iterate as long as we did not create the right number of surfels
    while(true) {

        // iterate all wrapped surfels 
        for ( auto& current_entropy_surfel_ptr : entropy_surfel_array ){
            //std::cout << "Fetching surfel [" << current_entropy_surfel_ptr->node_id << "][" << current_entropy_surfel_ptr->surfel_id << "]\n";


            std::vector< std::shared_ptr<entropy_surfel> > overlapping_neighbour_ptrs 
                = get_locally_overlapping_neighbours(current_entropy_surfel_ptr, entropy_surfel_array, overlap_radius_factor);

            //std::cout << "with neighbours: \n";
            std::cout << "\n";
            for (auto const& sesp : overlapping_neighbour_ptrs) {
                std::cout << (int)sesp->contained_surfel->color()[0] << ", "
                          << (int)sesp->contained_surfel->color()[1] << ", "
                          << (int)sesp->contained_surfel->color()[2] << " to entropy surfel (ids: ["<< sesp->node_id << "]["<<sesp->surfel_id<<"]\n";
                
            }

            //assign/compute missing attributes
            current_entropy_surfel_ptr->neighbours = overlapping_neighbour_ptrs;
            current_entropy_surfel_ptr->entropy = compute_entropy( current_entropy_surfel_ptr, overlapping_neighbour_ptrs);

            //if overlapping neighbours were found, put the entropy surfel back into the priority_queue
            if( !overlapping_neighbour_ptrs.empty() ) {
                min_entropy_surfel_ptr_queue.push_back( current_entropy_surfel_ptr );
            } else { //otherwise, consider this surfel to be finalized
                finalized_surfels.push_back( current_entropy_surfel_ptr );
            }
        }

        // the queue was modified, therefore it is resorted
        std::sort(min_entropy_surfel_ptr_queue.begin(), min_entropy_surfel_ptr_queue.end(), min_entropy_order());

        size_t num_valid_surfels = min_entropy_surfel_ptr_queue.size() + finalized_surfels.size();

        size_t counter = 0;



        while(true) {

            if( !min_entropy_surfel_ptr_queue.empty() ){
                //std::cout << min_entropy_surfel_ptr_queue.size() << "\n";
                std::shared_ptr<entropy_surfel> current_entropy_surfel = min_entropy_surfel_ptr_queue.back();
                
                min_entropy_surfel_ptr_queue.pop_back();

                //std::cout << "!!!Surfel with lowest_entropy: " << "[" << current_entropy_surfel->node_id << "][" << current_entropy_surfel->surfel_id << "]" << "\n";

                //the back element is still valid, so we still got something to do
                if(current_entropy_surfel->validity){
                    //std::cout << "Working on the surfel\n";
                    // if merge returns true, the surfel still has neighbours

                    
                    if( merge(current_entropy_surfel, num_valid_surfels, surfels_per_node) ) {
                        min_entropy_surfel_ptr_queue.push_back(current_entropy_surfel);

                        std::cout << "Pushing back into min_entropy_surfel_ptr_queue: " << (int)current_entropy_surfel->contained_surfel->color()[0] << "\n";
                    } else { //otherwise we can push it directly into the finalized surfel list
                        finalized_surfels.push_back(current_entropy_surfel);
                    }

                    std::sort(min_entropy_surfel_ptr_queue.begin(), min_entropy_surfel_ptr_queue.end(), min_entropy_order());

                    if(num_valid_surfels <= surfels_per_node) {
                        break;
                    } else {
                        //break
                    }
                }    

            } else {
                break;
            }

            //std::cout << "Loop 1\n";
        }


        if (num_valid_surfels > surfels_per_node) {


            //std::cout << "num valid surfels still bigger (" << num_valid_surfels << ")\n";
            //finalized_surfels.clear();
            //std::cout << "Size: " << finalized_surfels.size() << "\n";


            entropy_surfel_array.clear();

            std::vector< std::shared_ptr<entropy_surfel> >next_iteration_entropy_surfel_array;

            next_iteration_entropy_surfel_array.reserve(min_entropy_surfel_ptr_queue.size() + finalized_surfels.size());

            //entropy_surfel_array.reserve(min_entropy_surfel_ptr_queue.size() + finalized_surfels.size() );

            for (auto const& entr_surfel_ptr : min_entropy_surfel_ptr_queue) {
                next_iteration_entropy_surfel_array.push_back( entr_surfel_ptr );
            }

            for (auto const& entr_surfel_ptr : finalized_surfels) {
                next_iteration_entropy_surfel_array.push_back( entr_surfel_ptr );
            }


            entropy_surfel_array = next_iteration_entropy_surfel_array;

            min_entropy_surfel_ptr_queue.clear();
            finalized_surfels.clear();

            overlap_radius_factor *= 1.2;

        } else { // we are done with the simplification
            break;
        }

    }

    //end of entropy simplification
    while( (!min_entropy_surfel_ptr_queue.empty()) /*&& min_entropy_surfel_ptr_queue.back()->validity*/ ) {
        std::shared_ptr<entropy_surfel> en_surfel_to_push = min_entropy_surfel_ptr_queue.back();

        if(en_surfel_to_push->validity == true) {
            //std::cout << "Pushing surfel to fin queue: " << "[" << en_surfel_to_push->node_id << "][" << en_surfel_to_push->surfel_id << "]\n";
            finalized_surfels.push_back(en_surfel_to_push);
        }
        //else
        //    std::cout << "Discarding surfel\n";
        
        min_entropy_surfel_ptr_queue.pop_back();

        //std::cout << "Loop 2\n";
    }

    std::sort(finalized_surfels.begin(), finalized_surfels.end(), max_entropy_order());

    size_t chosen_surfels = 0;
    for(auto const& en_surf : finalized_surfels) {

        if(chosen_surfels++ < surfels_per_node) {
            mem_array.mem_data()->push_back(*(en_surf->contained_surfel));
            //std::cout << "Pushing surfel: [" << en_surf->node_id << "][" << en_surf->surfel_id << "]\n";
        } else {   
            // discarding a surfel
            break;
        }
    }

    //std::cout << "Mem data size: " << mem_array.mem_data()->size() << "\n";
    //std::cout << "Chosen Surfel: " << chosen_surfels << "\n";
    //std::cout << "Rest length: " << finalized_surfels.size() << "/" <<  surfels_per_node <<  "\n";
    //std::cout << "End with surfel array size " << mem_array.mem_data()->size() << "/" << surfels_per_node << "\n"; 
    mem_array.set_length(mem_array.mem_data()->size());

    reduction_error = 0.0;

    return mem_array;
};

} // namespace pre
} // namespace lamure
