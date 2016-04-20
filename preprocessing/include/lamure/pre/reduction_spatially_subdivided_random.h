// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_REDUCTION_OUTLIER_H
#define PRE_REDUCTION_OUTLIER_H

#include <lamure/pre/reduction_strategy.h>
#include <lamure/pre/bvh.h>
namespace lamure {
namespace pre {



class PREPROCESSING_DLL reduction_spatially_subdivided_random : public reduction_strategy
{
public:

    explicit            reduction_spatially_subdivided_random() { }

    surfel_mem_array      create_lod(real& reduction_error,
                                  const std::vector<surfel_mem_array*>& input,
                                  const uint32_t surfels_per_node,
                                  const bvh& tree,
          						  const size_t start_node_id) const override;

};

} // namespace pre
} // namespace lamure

#endif //PRE_REDUCTION_OUTLIER_H