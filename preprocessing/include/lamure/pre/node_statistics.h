// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_NODE_STATISTICS_H_
#define PRE_NODE_STATISTICS_H_

#include <array>

#include <lamure/pre/surfel_mem_array.h>
#include <lamure/assert.h>

namespace lamure {
namespace pre {


using histogram_t = 
    std::array<std::vector<uint32_t>,3>;

class node_statistics
{
public:
	node_statistics() : is_dirty_(true) {}

	void calculate_statistics(surfel_mem_array const& mem_array);

	vec3r_t const mean_pos() const {ASSERT(is_dirty_ == false); 
                                      return mean_pos_;}
	vec3r_t const mean_color() const {ASSERT(is_dirty_ == false);
					return mean_color_;}
	vec3r_t const mean_normal() const {ASSERT(is_dirty_ == false);
					 return mean_normal_;}
	real_t  const mean_radius() const {ASSERT(is_dirty_ == false);
	                                 return mean_radius_;}

	real_t const pos_sd() const {ASSERT(is_dirty_ == false);
			           return pos_sd_;}
	real_t const color_sd() const {ASSERT(is_dirty_ == false);
		                     return color_sd_;}
	real_t const normal_sd() const {ASSERT(is_dirty_ == false);
	                              return normal_sd_;}
	real_t const radius_sd() const {ASSERT(is_dirty_ == false);
	                              return radius_sd_;}
	                              
	real_t const min_radius() const {ASSERT(is_dirty_ == false); return min_radius_;}
	real_t const max_radius() const {ASSERT(is_dirty_ == false); return max_radius_;}


	histogram_t const color_histogram() const {ASSERT(is_dirty_ == false); return color_histogram_;}

	void set_dirty(bool dirty) {is_dirty_ = dirty;}
	bool is_dirty() const {return is_dirty_;}

private:

	bool is_dirty_;
	vec3r_t mean_pos_;
	real_t  pos_sd_;

	vec3r_t mean_color_;
	real_t  color_sd_;


	vec3r_t mean_normal_;
	real_t  normal_sd_;

	real_t  mean_radius_;
	real_t  radius_sd_;
	real_t  max_radius_;
	real_t  min_radius_;

	histogram_t color_histogram_;

};

}
}
#endif
