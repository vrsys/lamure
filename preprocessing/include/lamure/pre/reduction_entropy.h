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

 struct entropy_surfel{
	uint32_t surfel_id;
	uint32_t node_id;
	bool validity;
	float entropy;
	uint16_t level;
	std::vector<entropy_surfel*> neighbours;
	std::shared_ptr<surfel> contained_surfel;

	entropy_surfel(surfel const&  in_surfel, 
				   uint32_t const in_surfel_id, 
				   uint32_t const in_node_id,
				   bool in_validity = true,
				   float in_entropy = 0.0) : 
												surfel_id(in_surfel_id),
												node_id(in_node_id),
												validity(in_validity),
												entropy(in_entropy),
												level(0),
												neighbours(std::vector<entropy_surfel*>())
												 {
		contained_surfel = std::make_shared<surfel>(in_surfel);
	}
};

struct min_entropy_order{
	bool operator ()(entropy_surfel* const entropy_first, entropy_surfel* const entropy_second){

		// true  : first goes to the front, second to the back
		// false : first goes to the back, first to the front 

		// if first is not valid, sort it to the front

		bool is_rightmost = false;
		if ( entropy_first->validity == false && entropy_second->validity == true) {

			is_rightmost = true;
		} else if (entropy_first->validity == true && entropy_second->validity == true){
			if(entropy_first->entropy > entropy_second->entropy) {
				is_rightmost = true;
			}
		}

		return is_rightmost;
	}
};

struct max_entropy_order{
	bool operator ()(entropy_surfel* const entropy_first, entropy_surfel* const entropy_second){

		// true  : first goes to the front, second to the back
		// false : first goes to the back, first to the front 

		// if first is not valid, sort it to the front

		bool is_rightmost = false;
		if ( entropy_first->validity == false && entropy_second->validity == true) {

			is_rightmost = true;
		} else if (entropy_first->validity == true && entropy_second->validity == true){
			if(entropy_first->entropy < entropy_second->entropy) {
				is_rightmost = true;
			}
		}

		return is_rightmost;
	}
};

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

	std::vector<entropy_surfel*> const
	get_locally_overlapping_neighbours(entropy_surfel const& target_entropy_surfel,
                                   std::vector<entropy_surfel>& entropy_surfel_array) const;

	vec3r compute_center_of_mass(std::shared_ptr<surfel> current_surfel, std::vector<entropy_surfel*>& neighbour_ptrs) const;
	real compute_enclosing_sphere_radius(vec3r const& center_of_mass, std::shared_ptr<surfel> current_surfel, std::vector<entropy_surfel*> const& neighbour_ptrs) const;

	uint16_t update_level(uint16_t level) const {return level+1;}
	float compute_entropy(entropy_surfel* current_en_surfel, std::vector<entropy_surfel*> const& neighbour_ptrs) const;

	vec3f average_normal(std::shared_ptr<surfel> current_surfel, std::vector<entropy_surfel*> const& neighbour_ptrs) const;
	vec3b average_color(std::shared_ptr<surfel> current_surfel, std::vector<entropy_surfel*> const& neighbour_ptrs) const;

    bool
	merge(entropy_surfel* current_entropy_surfel,
      size_t& num_remaining_valid_surfel, size_t num_desired_surfel) const;
	

};

} // namespace pre
} // namespace lamure

#endif // PRE_REDUCTION_ENTROPY_H_
