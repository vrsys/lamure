// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_NODE_STATISTICS_H_
#define PRE_NODE_STATISTICS_H_

#include <lamure/pre/surfel_mem_array.h>

namespace lamure {
namespace pre {


using histogram_t = 
    std::array<std::vector<uint32_t>,3>;

class node_statistics
{
public:
	node_statistics() : is_dirty_(true) {}

	void calculate_statistics(surfel_mem_array const& mem_array);

	vec3r const mean_pos() const {assert(is_dirty_ == false); 
                                      return mean_pos_;}
	vec3r const mean_color() const {assert(is_dirty_ == false);
					return mean_color_;}
	vec3r const mean_normal() const {assert(is_dirty == false);
					 return mean_normal_;}
	real  const mean_radius() const {assert(is_dirty_ == false);
	                                 return mean_radius_;}

	real const pos_sd() const {assert(is_dirty_ == false);
			           return pos_sd_;}
	real const color_sd() const {assert(is_dirty_ == false);
		                     return color_sd_;}
	real const normal_sd() const {assert(is_dirty_ == false);
	                              return normal_sd_;}
	real const radius_sd() const {assert(is_dirty_ == false);
	                              return radius_sd_;}

	histogram_t const color_histogram() const {assert(is_dirty_ == false);						       return color_histogram_;}

	void set_dirty(bool dirty) {is_dirty_ = dirty;}
	bool is_dirty() const {return is_dirty_;}

private:

	bool is_dirty_;
	vec3r mean_pos_;
	real  pos_sd_;

	vec3r mean_color_;
	real color_sd_;

	vec3r mean_normal_;
	real  normal_sd_;

	real  mean_radius_;
	real  radius_sd_;

	histogram_t color_histogram_;

};

}
}
#endif
