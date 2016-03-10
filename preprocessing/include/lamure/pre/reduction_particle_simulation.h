// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_REDUCTION_PARTICLE_SIMULATION_H_
#define PRE_REDUCTION_PARTICLE_SIMULATION_H_

#include <lamure/pre/reduction_strategy.h>
#include <lamure/pre/bvh.h>
#include <lamure/pre/surfel.h>

#include <vector>
#include <queue>


namespace lamure {
namespace pre {

using neighbours_t = std::vector<std::pair<surfel_id_t, real> >;

class bvh;

//using shared_entropy_surfel = std::shared_ptr<entropy_surfel>;
//using shared_entropy_surfel_vector = std::vector<shared_entropy_surfel>;

class reduction_particle_simulation: public reduction_strategy
{
public:

    explicit  reduction_particle_simulation() {}

    surfel_mem_array      create_lod(real& reduction_error,
                                  const std::vector<surfel_mem_array*>& input,
                                  const uint32_t surfels_per_node,
          						  const bvh& tree,
          						  const size_t start_node_id) const override;
private:

	real 
	compute_enclosing_sphere_radius(surfel const& target_surfel,
                                	neighbours_t const& neighbour_ids,
                                	bvh const& tree) const;

};

} // namespace pre
} // namespace lamure

#endif // PRE_REDUCTION_PARTICLE_SIMULATION_H_
