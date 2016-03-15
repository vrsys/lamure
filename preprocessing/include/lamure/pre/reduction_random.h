// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_REDUCTION_RANDOM_H_
#define PRE_REDUCTION_RANDOM_H_

#include <lamure/pre/reduction_strategy.h>

namespace lamure {
namespace pre {

class reduction_random : public reduction_strategy
{
public:

    surfel_mem_array      create_lod(real& reduction_error,
                                  const std::vector<surfel_mem_array*>& input,
                                  const uint32_t surfels_per_node) const override;

};

} // namespace pre
} // namespace lamure

#endif // PRE_REDUCTION_RANDOM_H_
