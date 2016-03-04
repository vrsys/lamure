// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr


#ifndef  RADIUS_COMPUTATION_AVERAGE_DISTANCE_H_
#define  RADIUS_COMPUTATION_AVERAGE_DISTANCE_H_

#include <lamure/pre/radius_computation_strategy.h>

#include <vector>

namespace lamure {
namespace pre{

class bvh;

class radius_computation_average_distance: public radius_computation_strategy
{
public:
	explicit radius_computation_average_distance(const uint16_t number_of_neighbours) {
			number_of_neighbours_ = number_of_neighbours;
		}


	real  compute_radius(const bvh& tree, 
	 					 const surfel_id_t surfel,
                       	 std::vector<std::pair<surfel_id_t, real>> const& nearest_neighbours) const override;



};

}// namespace pre
}// namespace lamure

#endif // RADIUS_COMPUTATION_AVERAGE_DISTANCE_H_