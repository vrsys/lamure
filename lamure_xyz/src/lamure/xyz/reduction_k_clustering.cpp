// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/xyz/reduction_k_clustering.h>
#include <lamure/xyz/basic_algorithms.h>

#include <queue>
#include <array>
#include <utility>   // std::pair
#include <algorithm>//  std::max
#include <math.h>  //   floor
#include <cmath>  //    std::fabs

#include <forward_list>
#include <iterator> 

namespace lamure {
namespace xyz {


int reduction_k_clustering::
get_largest_dim(vec3f_t const& avg_normal) const{

    //compare normal absolute values of the 3 normal components 

    if (!((avg_normal.x_ == avg_normal.y_)&&(avg_normal.z_ == avg_normal.y_))) {

        if (avg_normal.x_ == std::max(std::fabs(avg_normal.x_), std::fabs(avg_normal.y_)) && std::fabs(avg_normal.y_) >= std::fabs(avg_normal.z_)) {
            return 0;
        }
        else if (avg_normal.y_ == std::max(std::fabs(avg_normal.x_), std::fabs(avg_normal.y_)) && std::fabs(avg_normal.x_) >= std::fabs(avg_normal.z_)) {
            return 1;
        }

        else if (avg_normal.z_ == std::max(std::fabs(avg_normal.z_), std::fabs(avg_normal.y_)) && std::fabs(avg_normal.y_) >= std::fabs(avg_normal.x_)) {
            return 2;
        } else {
            return 0;
        }

    } else {
        return 0; 
    }


}

shared_cluster_surfel_vector reduction_k_clustering::
get_initial_cluster_seeds(vec3f_t const& avg_normal, shared_cluster_surfel_vector const& input_surfels) const{
    //hash-based grouping
    //reference: http://www.ifi.uzh.ch/vmml/publications/older-puclications/DeferredBlending.pdf 

    const int group_num = 8; //set as member var. if user-defind value needed  but then consider different index distribution function depending on this mun.
    std::array <shared_cluster_surfel_vector, group_num> cluster_array; // container with 8 (in this case) subgroups

    //find largest dimention of the average normal vector
    int avg_dim = get_largest_dim(avg_normal); 

    for (auto const& current_surfel : input_surfels){

        uint16_t x_coord, y_coord ;  //variables to store 2D coordinate mapping

        x_coord = std::floor((current_surfel->contained_surfel->pos()[(avg_dim + 1) % 3] )/(current_surfel->contained_surfel->radius()));
        y_coord = std::floor((current_surfel->contained_surfel->pos()[(avg_dim + 2) % 3] )/(current_surfel->contained_surfel->radius()));
        uint16_t group_id = (x_coord*3 + y_coord) % group_num; // formula might need to be reconsidered for different group_num

        cluster_array[group_id].push_back(current_surfel);
    }

    //determine which array member hast biggest simber of elememts
    int max_size_group_id = -1;
    int32_t max_num_elements = -1;
    for (int i = 0; i < group_num; ++i){

        int32_t temp_num_elements = cluster_array[i].size();

        if (max_num_elements < temp_num_elements){
            max_num_elements = temp_num_elements;
            max_size_group_id = i;
        }

    }

    //surfels hashed to the largest group become the cluster seed set M
    for(auto const& current_surfel : cluster_array[max_size_group_id]){
       update_cluster_membership(current_surfel, true);
    }

    return cluster_array[max_size_group_id];
}


vec3f_t reduction_k_clustering::  
compute_avg_normal(shared_cluster_surfel_vector const& input_surfels) const{

    vec3f_t avg_normal (0.0, 0.0, 0.0);
    if( input_surfels.size() != 0){

        for (auto const& current_cluster_surfel_ptr : input_surfels) {
           avg_normal += current_cluster_surfel_ptr->contained_surfel->normal();
        }

        avg_normal /= input_surfels.size();

        if(lamure::math::length(avg_normal) != 0.0) {
            avg_normal = lamure::math::normalize(avg_normal);
        }
    }
       
    return avg_normal;
}


void reduction_k_clustering:: //functionality taken from entropy reduction strategy
assign_locally_overlapping_neighbours(shared_cluster_surfel current_surfel_ptr,
                                      shared_cluster_surfel_vector& input_surfel_ptr_array) const{


    shared_cluster_surfel_vector neighbours_found;
    shared_surfel target_surfel = current_surfel_ptr->contained_surfel;

    for(auto const& input_sufrel_ptr : input_surfel_ptr_array){

        // avoid overlaps with the surfel itself
        if (current_surfel_ptr->surfel_id != input_sufrel_ptr->surfel_id ||
            current_surfel_ptr->node_id   != input_sufrel_ptr->node_id) {

            shared_surfel contained_surfel_ptr = input_sufrel_ptr->contained_surfel;

            if( surfel::intersect(*target_surfel, *contained_surfel_ptr) ) {
                neighbours_found.push_back( input_sufrel_ptr );
            } 
        }
    }

    current_surfel_ptr->neighbours.insert(current_surfel_ptr->neighbours.end(),
                                        neighbours_found.begin(), neighbours_found.end());
}

void reduction_k_clustering::  
compute_overlap(shared_cluster_surfel current_surfel_ptr, bool look_in_M) const {
    //use only neighbours belonging to either set M or set S-M in order to compute overlap

    real_t overlap = 0;
    shared_surfel target_neighbour_surfel;
    real_t distance = 0.0;

    for(auto const& current_neighbour : current_surfel_ptr->neighbours){
        if (current_neighbour->member_of_M == look_in_M){
        target_neighbour_surfel = current_neighbour->contained_surfel;
        distance = compute_distance(current_surfel_ptr, current_neighbour);
        overlap += ((target_neighbour_surfel->radius() + current_surfel_ptr->contained_surfel->radius()) - distance);
        }
        
    }

    current_surfel_ptr->overlap = overlap;
}

real_t reduction_k_clustering::
compute_distance(shared_cluster_surfel first_surfel_ptr, shared_cluster_surfel second_surfel_ptr) const {
    return lamure::math::length(first_surfel_ptr->contained_surfel->pos() - second_surfel_ptr->contained_surfel->pos()); //
}

// compute deviation to all neighbours, independent on their set membership 
void reduction_k_clustering:: 
compute_deviation(shared_cluster_surfel current_surfel_ptr) const {

    real_t deviation = 0;
    shared_surfel target_neighbour_surfel;


    for(auto const& current_neighbour : current_surfel_ptr->neighbours){
        target_neighbour_surfel = current_neighbour->contained_surfel;
        deviation += 1 - std::fabs(lamure::math::dot(current_surfel_ptr->contained_surfel->normal(), target_neighbour_surfel->normal()));
    }

    current_surfel_ptr->deviation = deviation;
}

void reduction_k_clustering::
resolve_oversampling(shared_cluster_surfel_vector& surfel_ptr_set_m) const {
    std::sort(surfel_ptr_set_m.begin(), surfel_ptr_set_m.end(), max_overlap_order());
    real_t min_overlap = 0;
    shared_cluster_surfel_vector temp_neighbour_list;

    //maximal overlap computed
    shared_cluster_surfel m_member =  surfel_ptr_set_m.back();
    
    while( !surfel_ptr_set_m.empty() ) {

        shared_cluster_surfel m_member =  surfel_ptr_set_m.back();

        if(m_member->overlap > min_overlap){
            temp_neighbour_list = surfel_ptr_set_m.back()->neighbours;

            update_cluster_membership(m_member, false);
            surfel_ptr_set_m.pop_back();
            for(auto const& current_temp_neighbour : temp_neighbour_list){
                compute_overlap(current_temp_neighbour, true);
            }
            std::sort(surfel_ptr_set_m.begin(), surfel_ptr_set_m.end(), max_overlap_order());
        } else {
            break;
        }
    }

}

void reduction_k_clustering::
resolve_undersampling(shared_cluster_surfel_vector& input_surfel_ptr_array) const{


    shared_cluster_surfel_vector new_set_M;

    for(auto const& current_surfel_ptr : input_surfel_ptr_array){
        shared_cluster_surfel_vector neighbours = current_surfel_ptr->neighbours;

        bool member_neighbours = false; 
        if(current_surfel_ptr->member_of_M == false){
            for(auto const& current_neighbour : neighbours){
                if(current_neighbour->member_of_M == true){
                     member_neighbours = true;
                }
                    
            }

        }  

        if(!member_neighbours){
            new_set_M.push_back(current_surfel_ptr);
            update_cluster_membership(current_surfel_ptr, true);
        }
    }

    input_surfel_ptr_array.insert(input_surfel_ptr_array.end(), new_set_M.begin(), new_set_M.end());    
}

void reduction_k_clustering::
update_cluster_membership(shared_cluster_surfel current_surfel_ptr,
                          bool is_member) const {

    current_surfel_ptr->member_of_M = is_member;

}

void reduction_k_clustering::
remove_surfel(shared_cluster_surfel_vector& surfel_ptr_set_M) const {
    std::sort(surfel_ptr_set_M.begin(), surfel_ptr_set_M.end(),  min_deviation_order());
    surfel_ptr_set_M.pop_back();
}

void reduction_k_clustering:: 
add_surfel(shared_cluster_surfel_vector& surfel_ptr_set_M,
           shared_cluster_surfel_vector& total_surfel_set) const {

    
    shared_cluster_surfel_vector complement_set;
                          
    //sort out and store the complement set of M                        
    for (auto const& current_surfel_ptr : total_surfel_set){
         if(current_surfel_ptr->member_of_M == false){
            complement_set.push_back(current_surfel_ptr);
         }
    }

    //sum up total overlap of a compelement-member surfel with set-M-member neigbours
    for (auto const& complement_set_current_cluster_surfel : complement_set) {
         compute_overlap(complement_set_current_cluster_surfel, true);
    }    

    //creat new surfel to add to set M
    shared_cluster_surfel cluster_surfel_to_add; 

    //sort all surfels in the complement set base on overlap 
    std::sort(complement_set.begin(), 
              complement_set.end(), 
              min_overlap_order() );

        
    cluster_surfel_to_add = complement_set.back();
    surfel_ptr_set_M.push_back(complement_set.back());
    update_cluster_membership(cluster_surfel_to_add, true);
    //^complement_set.pop_back();    

}

void reduction_k_clustering::
merge(shared_cluster_surfel_vector& final_cluster_surfel_set) const
{   
    vec3r_t avg_position = vec3r_t(0.0, 0.0, 0.0);    
    vec3r_t avg_color = vec3r_t(0.0, 0.0, 0.0);

    shared_cluster_surfel_vector current_temp_neighbour_vector;
    for(auto const& current_surfel_ptr : final_cluster_surfel_set){
        avg_position = current_surfel_ptr->contained_surfel->pos();
        avg_color = current_surfel_ptr->contained_surfel->color();
        current_temp_neighbour_vector = current_surfel_ptr->neighbours;

        for(auto const& current_neighbour : current_temp_neighbour_vector){
            avg_position += current_neighbour->contained_surfel->pos();
            avg_color += current_neighbour->contained_surfel->color();
          
        }
        avg_position /= (current_temp_neighbour_vector.size() + 1 );
        avg_color /= (current_temp_neighbour_vector.size() + 1 );
        current_surfel_ptr->contained_surfel->pos() = avg_position;
        current_surfel_ptr->contained_surfel->color() = vec3b_t(avg_color[0], avg_color[1], avg_color[2]) ;
    }
} 


void reduction_k_clustering::
subsample(surfel_mem_array& joined_input, real_t const avg_radius) const{
    //std::cout<< "I should work on node with id: " << node.node_id() << "\n";
    const real_t max_radius_difference = 0.0; //
    //real_t avg_radius = node.avg_surfel_radius();

    auto copute_new_postion = [] (surfel const& plane_ref_surfel, real_t radius_offset, real_t rot_angle) {
        vec3r_t new_position (0.0, 0.0, 0.0);

        vec3f_t n = plane_ref_surfel.normal();

        //from random_point_on_surfel() in surfe.cpp
        //find a vector orthogonal to given normal vector
        vec3f_t  u(std::numeric_limits<float>::lowest(),
                   std::numeric_limits<float>::lowest(),
                   std::numeric_limits<float>::lowest());
        if(n.z_ != 0.0) {
            u = vec3f_t( 1, 1, (-n.x_ - n.y_) / n.z_);
        } else if (n.y_ != 0.0) {
            u = vec3f_t( 1, (-n.x_ - n.z_) / n.y_, 1);
        } else {
            u = vec3f_t( (-n.y_ - n.z_)/n.x_, 1, 1);
        }
        lamure::math::normalize(u);

        //vector rotation according to: https://en.wikipedia.org/wiki/Rodrigues'_rotation_formula
        vec3f_t u_rotated = n*cos(rot_angle) + lamure::math::cross(n,u)*sin(rot_angle) + u*lamure::math::dot(n,u)*(1-cos(rot_angle));

        //extend vector  lenght to match desired radius 
        u_rotated = u_rotated*radius_offset;
        
        new_position = u_rotated - plane_ref_surfel.pos();
        return new_position; 
    };

    //create new vector to store node surfels; unmodified + modified ones
    surfel_mem_array modified_mem_array (std::make_shared<surfel_vector>(surfel_vector()), 0, 0);
    for(uint32_t i = 0; i < joined_input.mem_data()->size(); ++i){

        surfel current_surfel = joined_input.read_surfel(i);

        //replace big surfels by collection of average-size surfels
        if (current_surfel.radius() - avg_radius > max_radius_difference){
            
            //how many times does average radius fit into big radius
            int itteration_level = floor(current_surfel.radius()/avg_radius);             

            //keep all surfel properties but shrink its radius to the average radius
            surfel new_surfel = current_surfel;
            joined_input.mem_data()->pop_back(); //remove the big-radius surfel
            new_surfel.radius() = avg_radius;
            joined_input.mem_data()->push_back(new_surfel);            
                    //modified_mem_array.mem_data()->push_back(new_surfel);

            //create new average-size surfels to fill up the area orininally covered by bigger surfel            
            for(int k = 1; k <= itteration_level; ++k){
               uint16_t num_new_surfels = 6*k; //^^ formula basis https://en.wikipedia.org/wiki/Circle_packing_in_a_circle
               for(int j = 0; j < num_new_surfels; ++j){
                    real_t radius_offset = k*2*avg_radius; 
                    real_t angle = 0.0;
                    real_t angle_offset = 2*M_PI / num_new_surfels;
                    new_surfel.color() = vec3b_t(1.0, 1.0, 1.0); 
                    new_surfel.pos() = copute_new_postion(current_surfel, radius_offset, angle);
                    joined_input.mem_data()->push_back(new_surfel);
                            //modified_mem_array.mem_data()->push_back(new_surfel);
                    angle += angle_offset;
               }
            }          
        }
    }

}


surfel_mem_array reduction_k_clustering::
create_lod(real_t& reduction_error,
           const std::vector<surfel_mem_array*>& input,
           const uint32_t surfels_per_node,
           const bvh& tree,
           const size_t start_node_id) const {

    //create output array
    surfel_mem_array mem_array(std::make_shared<surfel_vector>(surfel_vector()), 0, 0);

    //^^create surfel array for subsampling
    surfel_mem_array mem_array_temp(std::make_shared<surfel_vector>(surfel_vector()), 0, 0);

    //container for all input surfels including [total set S]
    shared_cluster_surfel_vector cluster_surfel_array;


    //cluster set M
    shared_cluster_surfel_vector cluster_surfel_output_array;

    //^^ interm. results: surfel + node_id
    std::pair<surfel_mem_array, uint32_t> sufel_node_pair;

    uint32_t radius_discarded_surfels = 0;

    // pack all surfels of the input array in a vector to be subsampled
    /*for (size_t node_id = 0; node_id < input.size(); ++node_id) {
        for (size_t surfel_id = input[node_id]->offset();
                    surfel_id < input[node_id]->offset() + input[node_id]->length();
                    ++surfel_id){

            //this surfel will be referenced in the cluster surfel
            auto current_surfel = input[node_id]->mem_data()->at(input[node_id]->offset() + surfel_id);

            // ignore outlier radii of any kind
            if (current_surfel.radius() == 0.0) {
                ++radius_discarded_surfels;
                continue;
            }

            mem_array_temp.mem_data()->push_back(current_surfel);
       }
    }*/

    //subsample
    //subsample(mem_array_temp,avg_radius_all_nodes);

    // wrap all surfels of the subsampled input array to cluster_surfels
    /*for (uint32_t i = 0; i < mem_array_temp.mem_data()->size(); ++i) {
        

            //this surfel will be referenced in the cluster surfel
            auto current_surfel = mem_array_temp.read_surfel(i);        

            // only place where shared pointers should be created
            cluster_surfel_array.push_back( std::make_shared<cluster_surfel>( cluster_surfel(current_surfel, i, node_id)  ) );
    }*/

       // wrap all surfels of the subsampled input array to cluster_surfels

    for (size_t node_id = 0; node_id < input.size(); ++node_id) {
        for (size_t surfel_id = input[node_id]->offset();
                    surfel_id < input[node_id]->offset() + input[node_id]->length();
                    ++surfel_id){

            //this surfel will be referenced in the cluster surfel
            auto current_surfel = input[node_id]->mem_data()->at(input[node_id]->offset() + surfel_id);

            // ignore outlier radii of any kind
            if (current_surfel.radius() == 0.0) {
                ++radius_discarded_surfels;
                continue;
            }              

            // only place where shared pointers should be created
            cluster_surfel_array.push_back( std::make_shared<cluster_surfel>( cluster_surfel(current_surfel, surfel_id, node_id)  ) );
       }
    }

    //define basic features for every cluster_surfel   
    for (auto const& target_surfel : cluster_surfel_array){
        assign_locally_overlapping_neighbours(target_surfel, cluster_surfel_array);
        compute_overlap(target_surfel, false);
        compute_deviation(target_surfel);
    }

    //average nomal, used in computaions of the hash-based grouping
    vec3f_t avg_normal = compute_avg_normal(cluster_surfel_array);


//sort all surfels into 2 sets
    //- set M - bais for output resul
    //- the complement of set M - all surfel which will not contribute to output
    cluster_surfel_output_array = get_initial_cluster_seeds (avg_normal, cluster_surfel_array);

    for (auto const& target_surfel : cluster_surfel_output_array ){    
        compute_overlap(target_surfel, true);       
    }
    
//make sure surfels selected in M are overlap-free and unifromly distributed
    resolve_oversampling(cluster_surfel_output_array);   
    resolve_undersampling(cluster_surfel_output_array);

    
//make sure desired number of output surfels is reached 

    //remove a surfel, if too many
     while(cluster_surfel_output_array.size() > surfels_per_node) {
        remove_surfel(cluster_surfel_output_array);
    }

    
    //add a surfel, if too few
    while(cluster_surfel_output_array.size() < surfels_per_node) {
        add_surfel (cluster_surfel_output_array, cluster_surfel_array);
    }
    
    //average color and postion of output surfels with their neighbours
    merge(cluster_surfel_output_array);
    

    //write surfels for output
    for (auto const& final_cluster_surfel : cluster_surfel_output_array) {

        mem_array.mem_data()->push_back(*(final_cluster_surfel->contained_surfel) );
    }

    mem_array.set_length(mem_array.mem_data()->size());  


    return mem_array;
}

} // namespace xyz
} // namespace lamure
