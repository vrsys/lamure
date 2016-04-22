// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_REDUCTION_STRATEGY_H_
#define PRE_REDUCTION_STRATEGY_H_


#include <lamure/pre/bvh_node.h>
#include <lamure/pre/surfel_mem_array.h>

namespace lamure {
namespace pre {

class bvh;

class reduction_strategy
{
public:

    virtual ~reduction_strategy() {}

    virtual surfel_mem_array create_lod(
      real_t& reduction_error, 
      const std::vector<surfel_mem_array*>& input,
      const uint32_t surfels_per_node,
      const bvh& tree,
      const size_t start_node_id) const = 0;

    void interpolate_approx_natural_neighbours(
      surfel& surfel_to_update,
      std::vector<surfel> const& input_surfels,
      const bvh& tree,
      size_t const num_nearest_neighbours = 24) const;
};

} // namespace pre
} // namespace lamure

#endif // PRE_REDUCTION_STRATEGY_H_
