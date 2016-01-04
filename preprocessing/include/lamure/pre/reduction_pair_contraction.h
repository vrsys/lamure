// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_REDUCTION_RANDOM_H_
#define PRE_REDUCTION_RANDOM_H_

#include <lamure/pre/reduction_strategy.h>
#include <lamure/pre/bvh.h>

namespace lamure {
namespace pre {

class reduction_pair_contraction : public reduction_strategy
{
public:

    surfel_mem_array      create_lod(real& reduction_error,
                                  const std::vector<surfel_mem_array*>& input,
                                  const uint32_t surfels_per_node) const override;


    surfel_mem_array      create_lod(bvh* tree,
                                  real& reduction_error,
                                  const std::vector<surfel_mem_array*>& input,
                                  const uint32_t surfels_per_node) const;


};

lamure::real quadric_error(const vec3r& p, const mat4r& quadric);
mat4r edge_quadric(const vec3f& normal_p1, const vec3r& p1, const vec3r& p2);

std::vector<std::pair<surfel_id_t, real>>
get_local_nearest_neighbours(const std::vector<surfel_mem_array*>& input,
                             size_t num_local_neighbours,
                             surfel_id_t const& target_surfel);

} // namespace pre
} // namespace lamure

#endif // PRE_REDUCTION_RANDOM_H_
