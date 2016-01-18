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


//^naming inconstency - neighbor
//^needs better name
struct cluster_surfel_with_neighbours{
    uint32_t surfel_id;
    uint32_t node_id;
    bool cluster_seed; //^ name choice
    real overlap;
    real deviation;
    std::vector<std::shared_ptr<cluster_surfel_with_neighbours> > neighbours;
    std::shared_ptr<surfel> contained_surfel;
    cluster_surfel_with_neighbours(surfel const&  in_surfel, uint32_t const in_surfel_id, uint32_t const in_node_id) :
                                                surfel_id(in_surfel_id),
                                                node_id(in_node_id),
                                                cluster_seed(false),
                                                overlap(0),
                                                deviation(0)
                                                 {
        contained_surfel = std::make_shared<surfel>(in_surfel);
    }

};


using shared_cluster_surfel = std::shared_ptr<cluster_surfel_with_neighbours>;
using shared_cluster_surfel_vector = std::vector<shared_cluster_surfel>;

struct max_overlap_order {
    bool operator() (shared_cluster_surfel const first_surfel, shared_cluster_surfel const second_surfel){

        if(first_surfel->overlap >= second_surfel->overlap){
           return false;
        }
        else
           return true;
    }
};


struct min_overlap_order {
    bool operator() (shared_cluster_surfel const first_surfel, shared_cluster_surfel const second_surfel){

        if(first_surfel->overlap <= second_surfel->overlap){
           return false;
        }
        else
           return true;
    }
};

struct min_deviation_order {
    bool operator() (shared_cluster_surfel const first_surfel, shared_cluster_surfel const second_surfel){

        if(first_surfel->deviation <= second_surfel->deviation){
           return false;
        }
        else
           return true;
    }
};


class reduction_k_clustering: public reduction_strategy
{
public:


explicit reduction_k_clustering(size_t num_neighbors): //^change also in builder.cpp
                                    number_of_neighbors_(num_neighbors)
                                    {}


    surfel_mem_array  create_lod(real& reduction_error,
                                 const std::vector<surfel_mem_array*>& input,
                                 const uint32_t surfels_per_node) const override;

private:


   size_t number_of_neighbors_;

  //hash_based algorithm to provide set of min-overlap surfels
  //^no currently no collision handling
  shared_cluster_surfel_vector get_inital_cluster_seeds(vec3f const& avg_norlam, shared_cluster_surfel_vector input_cluster_surfels);
  int get_largest_dim(vec3f const& avg_normal);
  vec3f compute_avg_normal(shared_cluster_surfel_vector input_surfels); //^entropy reduction has similar function


  void assign_locally_overlaping_neighbors(shared_cluster_surfel current_surfel_ptr,
                                           shared_cluster_surfel_vector& input_surfel_ptr_array); //functionality taken from entropy reduction strategy

  void compute_overlap(shared_cluster_surfel current_surfel_ptr); //use neighbors to compute overlap; [call only on set M ]

  void compute_deviation(shared_cluster_surfel current_surfel_ptr); //use neighbors to compute deviation

  uint32_t compute_distance(shared_cluster_surfel first_surfel_ptr,
                            shared_cluster_surfel second_surfel_ptr);

  void resolve_oversampling(shared_cluster_surfel_vector& surfel_ptr_set_M);

  shared_cluster_surfel_vector resolve_undersampling(shared_cluster_surfel_vector& input_surfel_ptr_array);

  void update_cluster_membership(shared_cluster_surfel current_surfel_ptr, bool is_member);

  void assign_locally_nearest_neighbors(shared_cluster_surfel current_surfel_ptr,
                                        const uint32_t number_of_neighbors,
                                        shared_cluster_surfel_vector& input_surfel_ptr_array); //partially taken from get_nearest_neighbours();

  void remove_surfel(shared_cluster_surfel_vector& surfel_ptr_set_M);

  void add_surfel(shared_cluster_surfel_vector& surfel_ptr_set_M,
                  shared_cluster_surfel_vector total_surfel_set);

  //void merge(shared_cluster_surfel current_surfel_ptr){}


};

} // namespace pre
} // namespace lamure

#endif // PRE_REDUCTION_K_CLUSTERING_
