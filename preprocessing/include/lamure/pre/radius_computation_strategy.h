// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr


#ifndef RADIUS_COMPUTATION_STRATEGY_H_
#define RADIUS_COMPUTATION_STRATEGY_H_

#include <lamure/pre/bvh.h>
#include <lamure/pre/surfel.h>

namespace lamure{
namespace pre {

class bvh;

class radius_computation_strategy {
public:
	virtual ~radius_computation_strategy() {};
	virtual real compute_radius(bvh& tree,
								const size_t node_id,
								const size_t surfel_id, 
								const uint16_t number_of_neighbours) const = 0;
};

} // namespace pre
} // namespace lamure

#endif //RADIUS_COMPUTATION_STRATEGY_H_