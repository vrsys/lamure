// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_REDUCTION_K_CLUSTERING_
#define PRE_REDUCTION_K_CLUSTERING_

#include <lamure/pre/reduction_strategy.h>

#include <lamure/pre/surfel.h>
#include <memory>
#include <vector>
#include <list>

namespace lamure {
namespace pre {



struct cluster_surfel_with_neighbours{
    uint32_t surfel_id;
    uint32_t node_id;
    bool member_of_M; 
    real overlap;
    real deviation;
    std::vector<std::shared_ptr<cluster_surfel_with_neighbours> > neighbours;
    std::shared_ptr<surfel> contained_surfel;
    cluster_surfel_with_neighbours(surfel const&  in_surfel, uint32_t const in_surfel_id, uint32_t const in_node_id) :
                                                surfel_id(in_surfel_id),
                                                node_id(in_node_id),
                                                member_of_M(false),
                                                overlap(0),
                                                deviation(0)
                                                 {
        contained_surfel = std::make_shared<surfel>(in_surfel);
    }

};

using cluster_surfel        = cluster_surfel_with_neighbours;
using shared_cluster_surfel = std::shared_ptr<cluster_surfel_with_neighbours>;
using shared_cluster_surfel_vector = std::vector<shared_cluster_surfel>;

struct max_overlap_order {
    bool operator() (shared_cluster_surfel const& first_surfel, shared_cluster_surfel const& second_surfel){
        if(!first_surfel || !second_surfel){
            throw std::exception();
        }
        if(first_surfel->overlap == second_surfel->overlap){
            if(first_surfel->node_id < second_surfel->node_id) {
                if(first_surfel->surfel_id < second_surfel->surfel_id) {
                    return true;
                }
                else return false;
            }
            else return false;
        }
        if(first_surfel->overlap > second_surfel->overlap){
           return false;
        }
        else
           return true;
    }
};


struct  min_SN_id_order{
    bool operator() (shared_cluster_surfel const& first_surfel, shared_cluster_surfel const& second_surfel) {
        if(!first_surfel || !second_surfel){
            throw std::exception();
        }

        if(first_surfel->node_id < second_surfel->node_id) {
            return true;
        } else if (first_surfel->node_id > second_surfel->node_id) {
            return false;
        } else if (first_surfel->node_id == second_surfel->node_id) {
            if(first_surfel->surfel_id < second_surfel->surfel_id){
                return true;
            }
            else {
                return false;
            }
        }
        else return false;
    }
    
};

struct min_overlap_order {
    bool operator() (shared_cluster_surfel const& first_surfel, shared_cluster_surfel const& second_surfel){
        if(!first_surfel || !second_surfel){
            throw std::exception();
        }
        if(first_surfel->overlap == second_surfel->overlap){
            if(first_surfel->node_id < second_surfel->node_id) {
                if(first_surfel->surfel_id < second_surfel->surfel_id) {
                    return true;
                }
                else return false;
            }
            else return false;
        }
        if(first_surfel->overlap < second_surfel->overlap){
           return false;
        }
        else
           return true;
    }
};

struct min_deviation_order {
    bool operator() (shared_cluster_surfel const& first_surfel, shared_cluster_surfel const& second_surfel){
        if(!first_surfel || !second_surfel){
            throw std::exception();
        }
        if(first_surfel->overlap == second_surfel->overlap){
            if(first_surfel->node_id < second_surfel->node_id) {
                if(first_surfel->surfel_id < second_surfel->surfel_id) {
                    return true;
                }
                else return false;
            }
            else return false;
        }
        if(first_surfel->deviation < second_surfel->deviation){
           return false;
        }
        else
           return true;
    }
};


class reduction_k_clustering: public reduction_strategy
{
public:


explicit reduction_k_clustering(size_t num_neighbours): 
                                    number_of_neighbours_(num_neighbours)
                                    {}


    surfel_mem_array      create_lod(real& reduction_error,
                                  const std::vector<surfel_mem_array*>& input,
                                  const uint32_t surfels_per_node,
                                  const bvh& tree,
                                  const size_t start_node_id) const override;
private:


   size_t number_of_neighbours_;

  //hash_based algorithm to provide set of min-overlap surfels
  //^no currently no collision handling
  shared_cluster_surfel_vector get_initial_member_of_Ms(vec3f const& avg_normal, shared_cluster_surfel_vector const&  input_cluster_surfels) const;
  int get_largest_dim(vec3f const& avg_normal) const;
  vec3f compute_avg_normal(shared_cluster_surfel_vector const& input_surfels) const; //^entropy reduction has similar function


  void assign_locally_overlapping_neighbours(shared_cluster_surfel current_surfel_ptr,
                                           shared_cluster_surfel_vector& input_surfel_ptr_array) const; //functionality taken from entropy reduction strategy

  void compute_overlap(shared_cluster_surfel current_surfel_ptr, bool look_in_M) const; //use neighbours to compute overlap; [call only on set M ]

  void compute_deviation(shared_cluster_surfel current_surfel_ptr) const; //use neighbours to compute deviation

  real compute_distance(shared_cluster_surfel first_surfel_ptr,
                            shared_cluster_surfel second_surfel_ptr) const;

  void resolve_oversampling(shared_cluster_surfel_vector& surfel_ptr_set_M) const;

  void resolve_undersampling(shared_cluster_surfel_vector& input_surfel_ptr_array) const;

  void update_cluster_membership(shared_cluster_surfel current_surfel_ptr, bool is_member) const;

  /*void assign_locally_nearest_neighbours(shared_cluster_surfel current_surfel_ptr,
                                        const uint32_t number_of_neighbours,
                                        shared_cluster_surfel_vector& input_surfel_ptr_array) const; //partially taken from get_nearest_neighbours();
   */

  void remove_surfel(shared_cluster_surfel_vector& surfel_ptr_set_M) const;

  void add_surfel(shared_cluster_surfel_vector& surfel_ptr_set_M,
                  shared_cluster_surfel_vector& total_surfel_set) const;

  void merge(shared_cluster_surfel_vector& final_cluster_surfel_set) const;


};

} // namespace pre
} // namespace lamure

#endif // PRE_REDUCTION_K_CLUSTERING_
