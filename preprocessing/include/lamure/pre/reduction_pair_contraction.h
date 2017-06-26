// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifdef CMAKE_OPTION_ENABLE_ALTERNATIVE_STRATEGIES

#ifndef PRE_REDUCTION_PAIR_H_
#define PRE_REDUCTION_PAIR_H_

#include <lamure/pre/reduction_strategy.h>
#include <lamure/pre/bvh.h>

namespace lamure {
namespace pre {

class PREPROCESSING_DLL reduction_pair_contraction : public reduction_strategy
{
public:
  explicit reduction_pair_contraction(const uint16_t number_of_neighbours)
      : number_of_neighbours_(number_of_neighbours){}

    surfel_mem_array      create_lod(real& reduction_error,
                                  const std::vector<surfel_mem_array*>& input,
                                  const uint32_t surfels_per_node,
                                  const bvh& tree,
                                  const size_t start_node_id) const override;

private:
  uint16_t  number_of_neighbours_;
};

lamure::real quadric_error(const vec3r& p, const mat4r& quadric);
mat4r edge_quadric(const vec3f& normal_p1, const vec3r& p1, const vec3r& p2);

std::vector<std::pair<surfel_id_t, real>>
get_local_nearest_neighbours(const std::vector<surfel_mem_array*>& input,
                             size_t num_local_neighbours,
                             surfel_id_t const& target_surfel);

} // namespace pre
} // namespace lamure

#endif // PRE_REDUCTION_PAIR_H_
#endif // CMAKE_OPTION_ENABLE_ALTERNATIVE_STRATEGIES