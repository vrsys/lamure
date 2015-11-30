// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr


#ifndef NORMAL_RADII_PLANE_FITTINGH_H_
#define NORMAL_RADII_PLANE_FITTINGH_H_

#include <lamure/pre/normal_radii_strategy.h>

#include <vector>

namespace lamure {
namespace pre{
	
class NormalRadiiPlaneFitting: public NormalRadiiStrategy{

	virtual vec3f  compute_normal(size_t node, size_t surfel, std::vector<std::pair<Surfel, real>>  neighbours) const override;
	virtual real compute_radius(size_t node, size_t surfel, std::vector<std::pair<Surfel, real>>  neighbours) const override;

};

}// namespace pre
}// namespace lamure

#endif // NORMAL_RADII_PLANE_FITTINGH_H_