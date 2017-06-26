// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifdef CMAKE_OPTION_ENABLE_ALTERNATIVE_STRATEGIES


#ifndef PRE_REDUCTION_RANDOM_H_
#define PRE_REDUCTION_RANDOM_H_

#include <lamure/pre/reduction_strategy.h>

namespace lamure {
namespace pre {

class PREPROCESSING_DLL reduction_random : public reduction_strategy
{
public:

    surfel_mem_array      create_lod(real& reduction_error,
                                  const std::vector<surfel_mem_array*>& input,
                                  const uint32_t surfels_per_node,
          						  const bvh& tree,
          						  const size_t start_node_id) const override;
private:

	//void subsample(surfel_mem_array& joined_input, real const avg_radius) const;    
};

} // namespace pre
} // namespace lamure

#endif // PRE_REDUCTION_RANDOM_H_
#endif // CMAKE_OPTION_ENABLE_ALTERNATIVE_STRATEGIES