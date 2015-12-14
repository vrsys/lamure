// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr


#ifndef NORMAL_COMPUTATION_STRATEGY_H_
#define NORMAL_COMPUTATION_STRATEGY_H_

// #include <lamure/pre/bvh.h>
#include <lamure/pre/surfel.h>

namespace lamure{
namespace pre {

class bvh;

class normal_computation_strategy {
public:
	virtual ~normal_computation_strategy() {};
	virtual vec3f compute_normal(const bvh& tree,
								 const surfel_id_t surfel) const = 0;
};

} // namespace pre
} // namespace lamure

#endif //NORMAL_COMPUTATION_STRATEGY_H_