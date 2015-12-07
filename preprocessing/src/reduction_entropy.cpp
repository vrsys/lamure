// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/reduction_entropy.h>

//#include <math.h>
#include <functional>
#include <numeric>
#include <vector>
#include <queue>

namespace lamure {
namespace pre {

float reduction_entropy::
compute_entropy(uint16_t level, vec3f& first_normal, vec3f& second_normal)
{   

    //float nad = std::inner_produc(first_normal.begin(), first_normal.end(), second_normal.begin(), 0);
    float nad = scm::math::dot(first_normal, second_normal)
    float entropy = (1 + level)/(1 + nad);

    return entropy;
}


void reduction_entropy::
initialize_surfel_struct(const std::vector<surfel_mem_array*>& input){

}


surfel_mem_array reduction_entropy::
create_lod(real& reduction_error,
          const std::vector<surfel_mem_array*>& input,
          const uint32_t surfels_per_node) const
{
    surfel_mem_array mem_array(std::make_shared<surfel_vector>(surfel_vector()), 0, 0);

    const uint32_t fan_factor =  input.size();

    //initialize minimal priority queue
    std:: priority_queue <float, std::vector<float>, min_order> min_pq;

   
    //create lod from input
    for (size_t i = 0; i < input.size(); ++i) {
        for (size_t j = input[i]->offset() + i;
                    j < input[i]->offset() + input[i]->length();
                    j += input.size()) {

            auto surfel = input[i]->mem_data()->at(j);

           
            mem_array.mem_data()->push_back(surfel);
        }
    }

    mem_array.set_length(mem_array.mem_data()->size());

    reduction_error = 0.0;

    return mem_array;
}



} // namespace pre
} // namespace lamure
