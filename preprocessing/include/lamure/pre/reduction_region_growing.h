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

class reduction_region_growing : public reduction_strategy
{
public:

	explicit reduction_region_growing();

    surfel_mem_array create_lod(real& reduction_error,
                                const std::vector<surfel_mem_array*>& input,
                                const uint32_t surfels_per_node) const override;

private:

	bool reached_maximum_bound(const std::vector<surfel*>& input_cluster) const;

	surfel* sample_point_from_cluster(const std::vector<surfel*>& input_cluster) const;

	real maximum_bound_;

	uint32_t neighbour_growth_rate_;
};

} // namespace pre
} // namespace lamure

#endif // PRE_REDUCTION_REGION_GROWING_H_