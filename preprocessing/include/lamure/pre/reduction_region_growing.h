// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_REDUCTION_REGION_GROWING_H_
#define PRE_REDUCTION_REGION_GROWING_H_

#include <lamure/pre/reduction_strategy.h>
#include <lamure/pre/surfel.h>

namespace lamure {
namespace pre {

class PREPROCESSING_DLL reduction_region_growing : public reduction_strategy
{
public:

	explicit reduction_region_growing();

    surfel_mem_array create_lod(real_t& reduction_error,
    							const std::vector<surfel_mem_array*>& input,
                                const uint32_t surfels_per_node,
          						const bvh& tree,
          						const size_t start_node_id) const override;

private:

	real_t find_maximum_bound(const std::vector<surfel*>& input_cluster) const;

	bool reached_maximum_bound(const std::vector<surfel*>& input_cluster, const real_t& maximum_bound) const;

	surfel* sample_point_from_cluster(const std::vector<surfel*>& input_cluster) const;

	std::vector<std::pair<surfel*, real_t>> find_neighbours(const std::vector<surfel*>& cluster, const surfel* core_surfel, const uint32_t& number_neighbours) const;

	uint32_t neighbour_growth_rate_;
};

} // namespace pre
} // namespace lamure

#endif // PRE_REDUCTION_REGION_GROWING_H_
