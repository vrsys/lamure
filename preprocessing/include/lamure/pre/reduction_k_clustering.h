// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_REDUCTION_K_CLUSTERING_
#define PRE_REDUCTION_K_CLUSTERING_

#include <lamure/pre/reduction_strategy.h>

#include <lamure/bounding_box.h>
#include <vector>
#include <list>

namespace lamure {
namespace pre {

struct surfel_with_neighbours{
    uint32_t surfel_id;
    uint32_t node_id;
    bool cluster_seed;
    real overlap;
    std::vector<std::shared_ptr<surfel_with_neighbours> > neighbours;
    std::shared_ptr<surfel> contained_surfel;
    surfel_with_neighbours(surfel const&  in_surfel, uint32_t const in_surfel_id, uint32_t const in_node_id) :
                                                surfel_id(in_surfel_id),
                                                node_id(in_node_id),
                                                cluster_seed(false),
                                                overlap(0)
                                                 {
        contained_surfel = std::make_shared<surfel>(in_surfel);
    }

};

class reduction_k_clustering: public reduction_strategy
{
public:

    explicit reduction_k_clustering(){}

    surfel_mem_array  create_lod(real& reduction_error,
                                 const std::vector<surfel_mem_array*>& input,
                                 const uint32_t surfels_per_node) const override;

private:



  //hash_based algorithm to provide set of min-overlap surfels
  //^no currently no collision handling
  std::vector<surfel_with_neighbours> get_inital_cluster_seeds(vec3f avg_norlam, std::vector<surfel_with_neighbours*>& input_surfels){}
  int get_largest_dim(vec3f const& avg_norlam){}
  vec3f compute_avg_normal(std::vector<surfel_with_neighbours*>& input_surfels){} //^entropy reduction has similar function


  void assign_neighbor(){}

  void compute_overlap(){} //fist check for intersection than estimate apr. neighbor overlap value; call only on set M
  void resolve_over_sampling(){}
  void resolve_under_sampling(){}
  void merge(){}



};

} // namespace pre
} // namespace lamure

#endif // PRE_REDUCTION_K_CLUSTERING_
