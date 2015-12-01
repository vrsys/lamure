// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr


#ifndef NORMAL_COMPUTATION_STRATEGY_H_
#define NORMAL_COMPUTATION_STRATEGY_H_

#include <lamure/pre/bvh.h>
#include <lamure/pre/surfel.h>

namespace lamure{
namespace pre {

class Bvh;

class NormalComputationStrategy {
public:
	virtual ~NormalComputationStrategy() {};
	virtual vec3f compute_normal(Bvh& tree,
								 const size_t node_id,
								 const size_t surfel_id,
								 const uint16_t number_of_neighbours) const = 0;	
};

} // namespace pre
} // namespace lamure

#endif //NORMAL_COMPUTATION_STRATEGY_H_