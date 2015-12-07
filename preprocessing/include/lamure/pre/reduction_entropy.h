// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_REDUCTION_ENTROPY_H_
#define PRE_REDUCTION_ENTROPY_H_

#include <lamure/pre/reduction_strategy.h>

namespace lamure {
namespace pre {

class reduction_entropy: public ReductionStrategy
{
public:

    explicit            reduction_entropy() { }

    SurfelMemArray      create_lod(real& reduction_error,
                                  const std::vector<SurfelMemArray*>& input,
                                  const uint32_t surfels_per_node) const override;
private:


	void reset_level() {};
	void compute_entropy() {};
	


	uint32_t entropy_;
	uint32_t level_;
	bool valid_;



};

} // namespace pre
} // namespace lamure

#endif // PRE_REDUCTION_ENTROPY_H_
