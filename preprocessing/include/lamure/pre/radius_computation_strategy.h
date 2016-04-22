// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr


#ifndef RADIUS_COMPUTATION_STRATEGY_H_
#define RADIUS_COMPUTATION_STRATEGY_H_

#include <lamure/pre/surfel.h>

namespace lamure{
namespace pre {

class bvh;

class radius_computation_strategy {
public:
	virtual ~radius_computation_strategy() {};
	virtual real_t compute_radius(
          const bvh& tree,
          const surfel_id_t surfel,
          std::vector<std::pair<surfel_id_t, real_t>> const& nearest_neighbours) const = 0;

        uint16_t const number_of_neighbours() const {return number_of_neighbours_;}

protected:
	uint16_t number_of_neighbours_;
};

} // namespace pre
} // namespace lamure

#endif //RADIUS_COMPUTATION_STRATEGY_H_
