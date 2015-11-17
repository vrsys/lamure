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

class ReductionStrategy
{
public:

    virtual ~ReductionStrategy() {}

    virtual SurfelMemArray CreateLod(real& reduction_error, const std::vector<SurfelMemArray*>& input,
                                     const uint32_t surfels_per_node) const = 0;

};

} // namespace pre
} // namespace lamure

#endif // PRE_REDUCTION_STRATEGY_H_
