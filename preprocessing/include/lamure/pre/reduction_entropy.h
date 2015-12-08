// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_REDUCTION_ENTROPY_H_
#define PRE_REDUCTION_ENTROPY_H_

#include <lamure/pre/reduction_strategy.h>
#include <lamure/pre/bvh.h>
#include <lamure/pre/surfel.h>

#include <vector>
#include <queue>


namespace lamure {
namespace pre {

class bvh;

class reduction_entropy: public reduction_strategy
{
public:

    explicit  reduction_entropy(const uint16_t number_of_neighbours)
    	: number_of_neighbours_(number_of_neighbours){}

    surfel_mem_array      create_lod(real& reduction_error,
                                  const std::vector<surfel_mem_array*>& input,
                                  const uint32_t surfels_per_node) const override;

  

private:

	uint16_t  number_of_neighbours_;


	uint16_t update_level(uint16_t level) {return level+1;}
	float compute_entropy(uint16_t level, vec3f own_normal, std::vector<std::pair<surfel, real>> neighbours);
	void initialize_queue(const std::vector<surfel_mem_array*>& input, const bvh& tree);
	vec3r average_position(std::vector<std::pair<surfel, real>> neighbours);
	vec3f average_normal(std::vector<std::pair<surfel, real>> neighbours);    
	real average_radius(std::vector<std::pair<surfel, real>> neighbours);

	  struct entropy_surfel{
		surfel current_surfel;
		int id;
		std::vector<std::pair<surfel, real>> neighbours;
		bool validity;
		uint16_t level;
		float entropy;
	};

	struct min_entropy_order{
		bool operator ()(const entropy_surfel& entropy_struct_l, entropy_surfel& entropy_struct_r){
			return entropy_struct_l.entropy > entropy_struct_r.entropy;
		}
	};

	void merge(entropy_surfel& current_entropy_struct, 
      		   std:: priority_queue <reduction_entropy::entropy_surfel, std::vector<reduction_entropy::entropy_surfel>, min_entropy_order>& pq);
	

};

} // namespace pre
} // namespace lamure

#endif // PRE_REDUCTION_ENTROPY_H_
