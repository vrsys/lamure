// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/reduction_every_second.h>

namespace lamure {
namespace pre {

SurfelMemArray ReductionEverySecond::
CreateLod(real& reduction_error,
          const std::vector<SurfelMemArray*>& input,
          const uint32_t surfels_per_node) const
{
    SurfelMemArray mem_array(std::make_shared<SurfelVector>(SurfelVector()), 0, 0);

    const real fan_factor = 2;
    const real mult = sqrt(1.0 + 1.0 / fan_factor)*1.0;

    //create lod from input
    for (size_t i = 0; i < input.size(); ++i) {
        for (size_t j = input[i]->offset() + i;
                    j < input[i]->offset() + input[i]->length();
                    j += input.size()) {

            auto surfel = input[i]->mem_data()->at(j);

            real new_rad = mult * surfel.radius();
            surfel.radius() = new_rad;
            mem_array.mem_data()->push_back(surfel);
        }
    }

    mem_array.set_length(mem_array.mem_data()->size());

    reduction_error = 0.0;

    return mem_array;
}

} // namespace pre
} // namespace lamure
