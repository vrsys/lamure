// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_REDUCTION_PAIR_H_
#define PRE_REDUCTION_PAIR_H_

#include <lamure/xyz/reduction_strategy.h>
#include <lamure/xyz/bvh.h>

namespace lamure {
namespace xyz {

class XYZ_DLL reduction_pair_contraction : public reduction_strategy
{
public:
  explicit reduction_pair_contraction(const uint16_t number_of_neighbours)
      : number_of_neighbours_(number_of_neighbours){}

    surfel_mem_array      create_lod(real_t& reduction_error,
                                  const std::vector<surfel_mem_array*>& input,
                                  const uint32_t surfels_per_node,
                                  const bvh& tree,
                                  const size_t start_node_id) const override;

private:
  uint16_t  number_of_neighbours_;
};

lamure::real_t quadric_error(const vec3r_t& p, const mat4r_t& quadric);
mat4r_t edge_quadric(const vec3f_t& normal_p1, const vec3r_t& p1, const vec3r_t& p2);

std::vector<std::pair<surfel_id_t, real_t>>
get_local_nearest_neighbours(const std::vector<surfel_mem_array*>& input,
                             size_t num_local_neighbours,
                             surfel_id_t const& target_surfel);

} // namespace xyz
} // namespace lamure

#endif // PRE_REDUCTION_PAIR_H_
