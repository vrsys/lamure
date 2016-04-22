// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/xyz/reduction_every_second.h>

namespace lamure {
namespace xyz {

surfel_mem_array reduction_every_second::
create_lod(real_t& reduction_error,
          const std::vector<surfel_mem_array*>& input,
          const uint32_t surfels_per_node,
          const bvh& tree,
          const size_t start_node_id) const
{
    surfel_mem_array mem_array(std::make_shared<surfel_vector>(surfel_vector()), 0, 0);

    const real_t fan_factor = 2;
    const real_t mult = sqrt(1.0 + 1.0 / fan_factor)*1.0;

    //create lod from input
    for (size_t i = 0; i < input.size(); ++i) {
        for (size_t j = input[i]->offset() + i;
                    j < input[i]->offset() + input[i]->length();
                    j += input.size()) {

            auto surfel = input[i]->mem_data()->at(j);

            real_t new_rad = mult * surfel.radius();
            surfel.radius() = new_rad;
            mem_array.mem_data()->push_back(surfel);
        }
    }

    mem_array.set_length(mem_array.mem_data()->size());

    reduction_error = 0.0;

    return mem_array;
}

} // namespace xyz
} // namespace lamure
