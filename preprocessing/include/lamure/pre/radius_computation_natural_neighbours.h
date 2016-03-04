// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr


#ifndef  RADIUS_COMPUTATION_NATURAL_NEIGHBOURS_H_
#define  RADIUS_COMPUTATION_NATURAL_NEIGHBOURS_H_

#include <lamure/pre/radius_computation_strategy.h>

#include <vector>

namespace lamure {
namespace pre{

class bvh;

class radius_computation_natural_neighbours: public radius_computation_strategy
{
public:
	explicit radius_computation_natural_neighbours(uint16_t desired_num_nearest_neighbours = 20,
		                                           uint16_t min_num_nearest_neighbours = 10,
												   uint16_t min_num_natural_neighbours = 3) 
		:  min_num_nearest_neighbours_(min_num_nearest_neighbours),
		   min_num_natural_neighbours_(min_num_natural_neighbours) {
		   		number_of_neighbours_ = desired_num_nearest_neighbours;
		   }


	real  compute_radius(const bvh& tree, 
	 					 const surfel_id_t surfel,
                       	  std::vector<std::pair<surfel_id_t, real>> const& nearest_neighbours) const override;


private:
	const uint16_t min_num_nearest_neighbours_;
	const uint16_t min_num_natural_neighbours_;
};

}// namespace pre
}// namespace lamure

#endif // RADIUS_COMPUTATION_NATURAL_NEIGHBOURS_H_