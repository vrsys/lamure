// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_REDUCTION_SORTED_EVERY_SECOND_H_
#define PRE_REDUCTION_SORTED_EVERY_SECOND_H_

#include <lamure/pre/reduction_strategy.h>
#include <lamure/pre/bvh.h>
#include <lamure/pre/surfel.h>

#include <vector>
#include <queue>


namespace lamure {
namespace pre {

class bvh;


class reduction_sorted_every_second : public reduction_strategy
{
public:

    explicit  reduction_sorted_every_second() {}

    surfel_mem_array      create_lod(real& reduction_error,
                                  const std::vector<surfel_mem_array*>& input,
                                  const uint32_t surfels_per_node,
          						  const bvh& tree,
          						  const size_t start_node_id) const override;

};

} // namespace pre
} // namespace lamure

#endif // PRE_REDUCTION_SORTED_EVERY_SECOND_H_