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
 
std::vector<reduction_entropy::entropy_surfel*> const reduction_entropy::
get_locally_overlapping_neighbours(entropy_surfel const& target_entropy_surfel,
                                   std::vector<entropy_surfel>& entropy_surfel_array) const {
    surfel* target_surfel = target_entropy_surfel.current_surfel; 
    real target_surfel_radius = target_surfel->radius();

    std::vector<entropy_surfel*> overlapping_neighbour_ptrs;

    for ( auto& array_entr_surfel : entropy_surfel_array ) {
        
	// avoid overlaps with the surfel itself
        if (target_entropy_surfel.surfel_id != array_entr_surfel.surfel_id || 
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
compute_center_of_mass(surfel* current_surfel, std::vector<entropy_surfel*>& neighbour_ptrs) const {

    //volume of a sphere (4/3) * pi * r^3
    real current_surfel_radius = current_surfel->radius();
    real current_surfel_mass = (4.0/3.0) * M_PI * current_surfel_radius * current_surfel_radius * current_surfel_radius;

    vec3r center_of_mass_enumerator = current_surfel_mass * current_surfel->pos();
    real center_of_mass_denominator = current_surfel_mass;

    //center of mass equation: c_o_m = ( sum_of( m_i*x_i) ) / ( sum_of(m_i) )
    for (auto curr_neighbour_ptr : neighbour_ptrs) {

	surfel* current_neighbour_surfel = curr_neighbour_ptr->current_surfel;

        real neighbour_radius = current_neighbour_surfel->radius();

        real neighbour_mass = (4.0/3.0) * M_PI * 
                                neighbour_radius * neighbour_radius * neighbour_radius;

        center_of_mass_enumerator += neighbour_mass * current_surfel->pos();

        center_of_mass_denominator += neighbour_mass;
    }

    return center_of_mass_enumerator / center_of_mass_denominator;
}

real reduction_entropy::
compute_enclosing_sphere_radius(vec3r const& center_of_mass, surfel* current_surfel, std::vector<entropy_surfel*> const& neighbour_ptrs) const {

    real enclosing_radius = 0.0;

    enclosing_radius = scm::math::length(center_of_mass - current_surfel->pos()) + current_surfel->radius();

    for (auto const curr_neighbour_ptr : neighbour_ptrs) {

	surfel* current_neighbour_surfel = curr_neighbour_ptr->current_surfel;
        real neighbour_enclosing_radius = scm::math::length(center_of_mass - current_neighbour_surfel->pos()) + current_neighbour_surfel->radius();
        
        if(neighbour_enclosing_radius > enclosing_radius) {
            enclosing_radius = neighbour_enclosing_radius;
        }
    }

    return enclosing_radius;
}

float reduction_entropy::
compute_entropy(entropy_surfel* current_en_surfel, std::vector<entropy_surfel*> const& neighbour_ptrs) const
{   
    float entropy = 0.0;
    
    vec3b const& current_color = current_en_surfel->current_surfel->color();

    for(auto const curr_neighbour_ptr : neighbour_ptrs){
	   surfel* current_neighbour_surfel = curr_neighbour_ptr->current_surfel;

        vec3f neighbour_normal = current_neighbour_surfel->normal();  

        vec3b const& neighbour_color = current_neighbour_surfel->color();

        float normal_angle = scm::math::dot(current_en_surfel->current_surfel->normal(), neighbour_normal);
        entropy += (1 + current_en_surfel->level)/(1.0 + normal_angle);

        real color_diff = scm::math::sqrt( pow((current_color[0] - neighbour_color[0]),2) +
                                            pow((current_color[1] - neighbour_color[1]),2) +
                                            pow((current_color[2] - neighbour_color[2]),2));

        entropy += (1 + current_en_surfel->level) / (-color_diff);
    };    
    
    return entropy;
}

vec3b reduction_entropy::
average_color(surfel* current_surfel, std::vector<entropy_surfel*> const& neighbour_ptrs) const{

    vec3r accumulated_color(0.0, 0.0, 0.0);
    double accumulated_weight = 0.0;

    accumulated_color = current_surfel->color();
    accumulated_weight = 1.0;

    for(auto const curr_neighbour_ptr : neighbour_ptrs){
        accumulated_weight += 1.0;
        accumulated_color += curr_neighbour_ptr->current_surfel->color();
    }

    vec3b normalized_color = vec3b(accumulated_color[0] / accumulated_weight,
                                   accumulated_color[1] / accumulated_weight,
                                   accumulated_color[2] / accumulated_weight );
    return normalized_color;
}

vec3f reduction_entropy::
average_normal(surfel* current_surfel, std::vector<entropy_surfel*> const& neighbour_ptrs) const{
    vec3f new_normal(0.0, 0.0, 0.0);

    real weight_sum = 0.f;

    new_normal = current_surfel->normal();
    weight_sum = 1.0;   

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

bool reduction_entropy::
merge(entropy_surfel* current_entropy_surfel,
      size_t& num_remaining_valid_surfel, size_t num_desired_surfel) const{

    size_t num_invalidated_surfels = 0;

    bool reached_simplification_limit = false;


    std::vector<entropy_surfel*> neighbours_to_merge;// = current_entropy_surfel->neighbours;

    //**replace own invalid neighbours by valid neighbours of invalid neighbours**
    std::map<size_t, std::set<size_t> > added_neighbours_during_merge;

    for (auto const actual_neighbour_ptr : current_entropy_surfel->neighbours) {
        // collect the invalid neighbours of our current node (invalidated in the last iteration
        if (!actual_neighbour_ptr->validity) {
            //iterate the neighbours of the invalid neighbour
            for(auto const second_neighbour_ptr : actual_neighbour_ptr->neighbours){

                // we only have to consider valid neighbours, all the others are also our own neighbours and already invalid
                if(second_neighbour_ptr->validity) {
                    size_t n_id = second_neighbour_ptr->node_id;
                    size_t s_id = second_neighbour_ptr->surfel_id;

                    //ignore 2nd neighbours which we found already at another neighbour
                    if( (added_neighbours_during_merge[n_id]).find(s_id) == added_neighbours_during_merge[n_id].end() ) {
                        (added_neighbours_during_merge[n_id]).insert(s_id);
                        neighbours_to_merge.push_back(second_neighbour_ptr);
                    }
                }
            }

        }
    }

    /*
    neighbours_to_merge.erase(std::remove_if(neighbours_to_merge.begin(),
                                             neighbours_to_merge.end(),
                                             [](entropy_surfel* const& es_ptr) {return es_ptr->validity == false;} ) , neighbours_to_merge.end()  );
    */


    //delete own invalid neighbours & merge neighbours to merge into new neighbours

    std::vector<entropy_surfel*> current_neighbours = current_entropy_surfel->neighbours ;

/*
    current_neighbours.erase(std::remove_if(current_neighbours.begin(),
                                            current_neighbours.end(),
                                            [](entropy_surfel* const& es_ptr) {return es_ptr->validity == false;} ) , current_neighbours.end()  );
*/
    current_neighbours.insert(std::end(current_neighbours), std::begin(neighbours_to_merge), std::end(neighbours_to_merge) );

    current_entropy_surfel->neighbours = current_neighbours;

    std::cout << "Neighbour size: " << current_entropy_surfel->neighbours.size() << "\n";


   std::vector<entropy_surfel*> invalidated_neighbours;

   for (auto const actual_neighbour_ptr : current_entropy_surfel->neighbours){
        
        if (actual_neighbour_ptr->validity) {
            
            std::cout << "Setting neighbour: " << actual_neighbour_ptr->node_id << " --- " << actual_neighbour_ptr->surfel_id << "to false\n";
            actual_neighbour_ptr->validity = false;

            invalidated_neighbours.push_back(actual_neighbour_ptr);

            if( --num_remaining_valid_surfel == num_desired_surfel ) {

                reached_simplification_limit = true;
                break;
            }
        }
    }
    //recompute values for merged surfel
    current_entropy_surfel->level += 1;

    surfel* current_surfel = current_entropy_surfel->current_surfel;
    //current_entropy_surfel.current_surfel->pos() = average_position(current_entropy_surfel.neighbours);

    vec3r center_of_neighbour_masses = compute_center_of_mass(current_surfel, invalidated_neighbours);
    current_surfel->pos() = center_of_neighbour_masses;
    current_surfel->normal() = average_normal(current_surfel, invalidated_neighbours);
    current_surfel->color()  = average_color(current_surfel, invalidated_neighbours);
    //current_entropy_surfel.current_surfel->radius() = average_radius(current_entropy_surfel.neighbours);
    current_surfel->radius() = compute_enclosing_sphere_radius(center_of_neighbour_masses, current_surfel, invalidated_neighbours); 

    //std::vector<reduction_entropy::entropy_surfel*>& pq_entr_surfel_ptrs = Container(pq);

    //mark neighbours as invalid
    //reduction_entropy::entropy_surfel potential_neighbour;


    //std::vector<
    //for(auto& potential_neighbour_ptr : entropy_surfel_array){


        //if(reached_simplification_limit)
          //  break;
    //}



    current_entropy_surfel->entropy = compute_entropy(current_entropy_surfel, invalidated_neighbours);
    //update entropy
    //pq.push(current_entropy_surfel);

    std::cout << "Amount invalidated surfels: " << num_invalidated_surfels << "\n";

    if(!current_entropy_surfel->neighbours.empty()) {
        return true;
        //entropy_surfel_array.push_back(current_entropy_surfel);
    } else {
        std::cout << "READY\n";
        return false;
    }


}

surfel_mem_array reduction_entropy::
create_lod(real& reduction_error,
          const std::vector<surfel_mem_array*>& input,
          const uint32_t surfels_per_node) const
{
    surfel_mem_array mem_array(std::make_shared<surfel_vector>(surfel_vector()), 0, 0);

    std::vector<entropy_surfel> entropy_surfel_array;

    std::vector<entropy_surfel*> min_entropy_surfel_ptr_queue;

    std::vector<entropy_surfel*> finalized_surfels;

    for (size_t node_id = 0; node_id < input.size(); ++node_id){
        for (size_t surfel_id = input[node_id]->offset();
                    surfel_id < input[node_id]->offset() + input[node_id]->length();
                    ++surfel_id){
            
	        auto& current_surfel = input[node_id]->mem_data()->at(input[node_id]->offset() + surfel_id);
            
            //assign default attributes
            entropy_surfel current_entropy_surfel;
            current_entropy_surfel.current_surfel = &current_surfel;
            current_entropy_surfel.surfel_id = surfel_id;
            current_entropy_surfel.node_id   = node_id;
            current_entropy_surfel.validity = true;
            current_entropy_surfel.level = 0;
            
	    entropy_surfel_array.push_back(current_entropy_surfel);
	   }
    }	

    if(entropy_surfel_array.empty())
        std::cout << "ESA initially Empty\n";

    for ( auto& current_entropy_surfel : entropy_surfel_array ){
            
            std::vector<reduction_entropy::entropy_surfel*> overlapping_neighbour_ptrs = get_locally_overlapping_neighbours(current_entropy_surfel, entropy_surfel_array);
            std::cout << "Locally overlapping neighbours: " << overlapping_neighbour_ptrs.size() <<  "\n\n";

            //assign/compute missing attributes
            current_entropy_surfel.neighbours = overlapping_neighbour_ptrs;
            current_entropy_surfel.entropy = compute_entropy(&current_entropy_surfel, overlapping_neighbour_ptrs);

            if( !overlapping_neighbour_ptrs.empty() ) {
                min_entropy_surfel_ptr_queue.push_back(&current_entropy_surfel);
            } else {
                //finalized_surfels.push_back(&current_entropy_surfel);
                std::cout << "Skipping...\n";
            }
    }

    std::sort(min_entropy_surfel_ptr_queue.begin(), min_entropy_surfel_ptr_queue.end(), min_entropy_order());

    size_t num_valid_surfels = min_entropy_surfel_ptr_queue.size() + finalized_surfels.size();

    size_t counter = 0;



    if(num_valid_surfels == 0)
        std::cout << "Empty from the begin\n";
    else
        std::cout << "Not Empty\n";

    while(true) {
        //the back element is still valid, so we still got something to do
        if( !min_entropy_surfel_ptr_queue.empty() ){
            entropy_surfel* current_entropy_surfel = min_entropy_surfel_ptr_queue.back();
            
            min_entropy_surfel_ptr_queue.pop_back();


            if(current_entropy_surfel->validity){

                if( merge(current_entropy_surfel, num_valid_surfels, surfels_per_node) ) {
                    std::cout << "repushing\n";
                    //min_entropy_surfel_ptr_queue.push_back(current_entropy_surfel);
                } else {
                    std::cout << "no neighbours left\n";
                    //finalized_surfels.push_back(current_entropy_surfel);
                }
                //min_pq.push(current_entropy_surfel);
                std::sort(min_entropy_surfel_ptr_queue.begin(), min_entropy_surfel_ptr_queue.end(), min_entropy_order());

                if(num_valid_surfels <= surfels_per_node) {
                    std::cout << "REACHED SURFEL COUNT\n";
                    break;
                }
            }    
            
           // std::cout << "Begun sorting\n";

           // std::cout << "Finished sorting\n";
        } else {

            break;
        }
        
        //std::cout << "Looping\n";
    }

    //end of entropy simplification
    while( (!min_entropy_surfel_ptr_queue.empty()) /*&& min_entropy_surfel_ptr_queue.back()->validity*/ ) {
        entropy_surfel* en_surfel_to_push = min_entropy_surfel_ptr_queue.back();

        std::cout << "Second\n";
        if(en_surfel_to_push->validity == true)
            finalized_surfels.push_back(en_surfel_to_push);
        
        min_entropy_surfel_ptr_queue.pop_back();
        //std::sort(min_entropy_surfel_ptr_queue.begin(), min_entropy_surfel_ptr_queue.end(), min_entropy_order());
    }

    for(auto const& en_surf : finalized_surfels) {
        std::cout << "Pushing Surfel\n";
        mem_array.mem_data()->push_back(*(en_surf->current_surfel));      
    }


    mem_array.set_length(mem_array.mem_data()->size());

    reduction_error = 0.0;

    std::cout << "Done Simplifying\n";
    return mem_array;
};



} // namespace pre
} // namespace lamure
