// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr


#ifndef  NORMAL_COMPUTATION_PLANE_FITTINGH_H_
#define  NORMAL_COMPUTATION_PLANE_FITTINGH_H_

#include <lamure/pre/normal_computation_strategy.h>

#include <vector>

namespace lamure {
namespace pre {

class bvh;

class normal_computation_plane_fitting: public normal_computation_strategy
{
public:
	vec3f  compute_normal(bvh& tree,
						  const size_t node_id,
						  const size_t surfel_id,
						  const uint16_t number_of_neighbours) const override;
	
};

}// namespace pre
}// namespace lamure

#endif // NORMAL_COMPUTATION_PLANE_FITTINGH_H_