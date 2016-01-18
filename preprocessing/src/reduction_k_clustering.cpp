// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/reduction_k_clustering.h>

#include <lamure/pre/basic_algorithms.h>
#include <lamure/utils.h>

#include <queue>
#include <array>
#include <utility>   // std::pair
#include <algorithm>    // std::max
#include <math.h>  //floor

namespace lamure {
namespace pre {




int reduction_k_clustering::
get_largest_dim(vec3f const& avg_normal){

    // ^add abs

    if (!((avg_normal.x == avg_normal.y)&&(avg_normal.z == avg_normal.y))){

        if (avg_normal.x == std::max(avg_normal.x, avg_normal.y) && avg_normal.y >= avg_normal.z){

            return 0;
        }
        else if (avg_normal.y == std::max(avg_normal.x, avg_normal.y) && avg_normal.x >= avg_normal.z) {

            return 1;

        }

        else if (avg_normal.z == std::max(avg_normal.z, avg_normal.y) && avg_normal.y >= avg_normal.x){

            return 2;
        }

    }

    else return 0; //^set to rand.?


}

shared_cluster_surfel_vector reduction_k_clustering::
get_inital_cluster_seeds(vec3f const& avg_normal, shared_cluster_surfel_vector input_surfels){

    const int group_num = 8; //^ member var. if user-defind value needed  //^consider different index distribution funbction depending on this mun.
    uint16_t x_coord, y_coord ;  // 2D coordinate mapping
    uint16_t group_id = 1;
    //int num_elemets = -1;

    std::array <shared_cluster_surfel_vector, group_num> cluster_array; // container with 8 (in this case) subgroups


    int avg_dim = get_largest_dim(avg_normal);

    for (int i = 0; i < group_num; ++i){

    }

    for (auto current_surfel : input_surfels){
        x_coord = std::floor((current_surfel->contained_surfel->pos()[(avg_dim + 1) % 3] )/(current_surfel->contained_surfel->radius()));
        y_coord = std::floor((current_surfel->contained_surfel->pos()[(avg_dim + 2) % 3] )/(current_surfel->contained_surfel->radius()));
        group_id = (x_coord*3 + y_coord) % group_num; //^formula needs to reconsidered for user-defined group_num
        update_cluster_membership(current_surfel, true);
        cluster_array[group_id].push_back(current_surfel);

    }


    //^! determine which array member hast biggest simber of elememts

    return cluster_array[group_id];; //^should return group which was last added to

}


vec3f reduction_k_clustering::  //^entropy reduction has similar function
compute_avg_normal(shared_cluster_surfel_vector input_surfels){

    vec3f avg_normal (0.0, 0.0, 0.0);

       for (auto current_cluster_surfel_ptr : input_surfels) {

           avg_normal += current_cluster_surfel_ptr->contained_surfel->normal();
           //^  division by weight_term mising
        }

    return avg_normal;
}


void reduction_k_clustering:: //functionality taken from entropy reduction strategy
assign_locally_overlaping_neighbors(shared_cluster_surfel current_surfel_ptr,
                                    shared_cluster_surfel_vector& input_surfel_ptr_array){

    shared_cluster_surfel_vector neighbors_found;
    shared_surfel target_surfel = current_surfel_ptr->contained_surfel;

    for(auto& input_sufrel_ptr : input_surfel_ptr_array){

        // avoid overlaps with the surfel itself
        if (current_surfel_ptr->surfel_id != input_sufrel_ptr->surfel_id ||
            current_surfel_ptr->node_id   != input_sufrel_ptr->node_id) {

            shared_surfel contained_surfel_ptr = input_sufrel_ptr->contained_surfel;

            if( surfel::intersect(*target_surfel, *contained_surfel_ptr) ) {
                neighbors_found.push_back( input_sufrel_ptr );
            }
    }

    current_surfel_ptr->neighbors.insert(current_surfel_ptr->neighbors.begin(),
                                        neighbors_found.begin(), neighbors_found.end());

}

void reduction_k_clustering::  //use neighbors to compute overlap; call only on set M
compute_overlap(shared_cluster_surfel current_surfel_ptr){

    real overlap = 0;
    shared_surfel target_neighbor_surfel;
    uint32_t distance = 0;

    for(auto current_neighbor : current_surfel_ptr->neighbors){
        target_neighbor_surfel = current_neighbor->contained_surfel;
        distance = compute_distance(current_surfel_ptr, current_neighbor);
        overlap += ((target_neighbor_surfel->radius() + current_surfel_ptr->contained_surfel.radius() - distance);
    }

    current_surfel_ptr->overlap = overlap;
}

uint32_t reduction_k_clustering::
compute_distance(shared_cluster_surfel first_surfel_ptr, shared_cluster_surfel second_surfel_ptr){
    return 1; //^ :P
}

void reduction_k_clustering:: //use neighbors to compute deviation
compute_deviation(shared_cluster_surfel current_surfel_ptr){

    real deviation = 0;
    shared_surfel target_neighbor_surfel;


    for(auto current_neighbor : current_surfel_ptr->neighbors){
        target_neighbor_surfel = current_neighbor->contained_surfel
        deviation += std::fabs(scm::math::dot(current_surfel_ptr->contained_surfel->normal(), target_neighbor_surfel.normal()))
    }

    current_surfel_ptr->deviation = deviation;
}

void reduction_k_clustering::
resolve_oversampling(shared_cluster_surfel_vector& surfel_ptr_set_m){
    std::sort(surfel_ptr_set_m.begin(), surfel_ptr_set_m.end(), max_overlap_order());
    real min_overlap = 0;
    share_cluster_surfel_vector temp_neighbor_list;


    while(m_member->overlap > min_overlap && !surfel_ptr_set_m.empty()){
            shared_cluster_surel m_member =  surfel_ptr_set_m.back();
            temp_neighbor_list = surfel_ptr_set_m.back()->neighbors;

            update_cluster_membership(m_member, false);
            surfel_ptr_set_m.pop_back();

            for(auto current_temp_neighbor : temp_neighbor_list){
                compute_overlap(current_temp_neighbor);
            }

            std::sort(surfel_ptr_set_m.begin(), surfel_ptr_set_m.end(), max_overlap_order());

    }


}


shared_cluster_surfel_vector reduction_k_clustering::
resolve_undersampling(shared_cluster_surfel_vector& input_surfel_ptr_array){

    bool is_member = false;
    shared_cluster_surfel_vector new_set_m;

    for(auto current_surfel_ptr : input_surfel_ptr_array){ //^&
        share_cluster_surfel_vector neighbors = current_surfel_ptr->neighbors;
        if(current_surfel_ptr->cluster_seed == false){
            for(auto current_neighbor : neighbors){
                if(current_neighbor->cluster_seed == true)
                    is_member = true;
                }

        }
        else is_member =  false;

        if(!is_member){
            new_set_m.push_back(current_surfel_ptr);
        }
    }

    return new_set_m;
}


void reduction_k_clustering::
update_cluster_membership(shared_cluster_surfel current_surfel_ptr,
                          bool is_member){

    current_surfel_ptr->cluster_seed = is_member;

}


void reduction_k_clustering::
assign_locally_nearest_neighbors(shared_cluster_surfel current_surfel_ptr,
                                 const uint32_t number_of_neighbors,
                                 shared_cluster_surfel_vector& input_surfel_ptr_array){

    std::vector<std::pair<shared_cluster_surfel, real>> candidates;
    real max_candidate_distance = std::numeric_limits<real>::infinity();

}

void reduction_k_clustering::
remove_surfel(shared_cluster_surfel_vector& surfel_ptr_set_M){

    std::sort(surfel_ptr_set_M.begin(), surfel_ptr_set_M.end(),  min_deviation_order());
    surfel_ptr_set_M.pop_back;
}

void reduction_k_clustering:: //^
add_surfel(shared_cluster_surfel_vector& surfel_ptr_set_M,
           shared_cluster_surfel_vector Complement_set_with_neighbors_in_M){

    std::sort(Complement_set_with_neighbors_in_M.begin(), Complement_set_with_neighbors_in_M.end(), min_overlap_order() )

    cluster_surfel_to_add = Complement_set_with_neighbors_in_M.back;

    surfel_ptr_set_M.push_back(cluster_surfel_to_add);
}

/*void reduction_k_clustering::
merge(shared_cluster_surfel current_surfel_ptr){

} */


surfel_mem_array reduction_k_clustering::
create_lod(real& reduction_error,
          const std::vector<surfel_mem_array*>& input,
          const uint32_t surfels_per_node) const

{
    //average nomal
    vec3f avg_normal = compute_avg_normal(input);

    //create output array
    surfel_mem_array mem_array(std::make_shared<surfel_vector>(surfel_vector()), 0, 0);


    //container for all input surfels including [total set S]
    shared_cluster_surfel_vector cluster_surfel_array;


    //cluster set M
    shared_cluster_surfel_vector cluster_surfel_output_array;

    // wrap all surfels of the input array to cluster_surfels
    for (size_t node_id = 0; node_id < input.size(); ++node_id) {
        for (size_t surfel_id = input[node_id]->offset();
                    surfel_id < input[node_id]->offset() + input[node_id]->length();
                    ++surfel_id){

            //this surfel will be referenced in the cluster surfel
            auto current_surfel = input[node_id]->mem_data()->at(input[node_id]->offset() + surfel_id);

            // ignore outlier radii of any kind
            if (current_surfel.radius() == 0.0) {
                continue;
            }
            //create new cluster surfel
            cluster_surfel current_cluster_surfel(current_surfel, surfel_id, node_id);

        //^? only place where shared pointers should be created
        cluster_surfel_array.push_back( std::make_shared<cluster_surfel>(current_cluster_surfel) );
       }


    for (auto target_surfel : cluster_surfel_array ){

        assign_locally_nearest_neighbors(target_surfel, cluster_surfel_array);
        compute_overlap(target_surfel);
        compute_deviation(target_surfel);
    }

    cluster_surfel_output_array = get_inital_cluster_seeds (avg_normal, cluster_surfel_array);

    resolve_oversampling(cluster_surfel_output_array);

    cluster_surfel_output_array = resolve_undersampling(cluster_surfel_output_array);



    //make sure desired number of output surfels is reached
    if (cluster_surfel_output_array.size() > surfels_per_node){

        remove_surfel(cluster_surfel_output_array);

    }

    else if (cluster_surfel_output_array.size() < surfels_per_node)
    {
        add_surfel (cluster_surfel_output_array, cluster_surfel_complement_with_neighbors_in_M);
    }

    //write surfels for output
    for (auto const& final_cluster_surfel : cluster_surfel_output_array){
        mem_array.mem_data()->push_back(*(final_cluster_surfel->contained_surfel);
    }

    return mem_array;
}

} // namespace pre
} // namespace lamure
