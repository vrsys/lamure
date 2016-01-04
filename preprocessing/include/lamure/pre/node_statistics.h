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

class node_statistics
{
public:
	node_statistics() : is_dirty_(true) {}

	void calculate_statistics(surfel_mem_array const& mem_array);

	vec3r const mean() const {assert(is_dirty_ == false); 
							  return mean_;}
	real const sd() const {assert(is_dirty_ == false);
						   return sd_;}

	void set_dirty(bool dirty) {is_dirty_ = dirty;}
	bool is_dirty() const {return is_dirty_;}
private:

	bool is_dirty_;
	vec3r mean_;
	real  sd_;
};

}
}
#endif