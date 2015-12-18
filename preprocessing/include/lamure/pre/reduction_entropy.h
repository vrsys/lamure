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

struct neighbour_distance_t{
	surfel const* surfel_ptr;
	real neighbour_distance;
	surfel_id_t surfel_node_id;

	neighbour_distance_t(surfel const* surf_ptr, real const distance, surfel_id_t const& s_n_id)
	  :surfel_ptr(surf_ptr)
	  ,neighbour_distance(distance)
	  ,surfel_node_id(s_n_id)
	  {}
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

	  struct entropy_surfel{
		surfel* current_surfel;
		uint32_t surfel_id;
		uint32_t node_id;
		std::vector<entropy_surfel*> neighbours;
		bool validity;
		uint16_t level;
		float entropy;
	};

	uint16_t  number_of_neighbours_;

	std::vector<entropy_surfel*> const
	get_locally_overlapping_neighbours(entropy_surfel const& target_entropy_surfel,
                                   std::vector<entropy_surfel>& entropy_surfel_array) const;

	vec3r compute_center_of_mass(std::vector<entropy_surfel*>& neighbour_ptrs) const;
	real compute_enclosing_sphere_radius(vec3r const& surfel_pos, std::vector<entropy_surfel*> const& neighbour_ptrs) const;

	uint16_t update_level(uint16_t level) const {return level+1;}
	float compute_entropy(uint16_t level,  vec3f const& own_normal, std::vector<entropy_surfel*> const& neighbour_ptrs) const;
	//vec3r average_position(std::vector<neighbour_distance_t> const& neighbours) const;
	vec3f average_normal(std::vector<entropy_surfel*> const& neighbour_ptrs) const;
	//real average_radius(std::vector<neighbour_distance_t> const& neighbours) const;

	/*std::vector<std::pair<surfel_id_t, real>>
	get_local_nearest_neighbours (const std::vector<surfel_mem_array*>& input,
                             	  size_t num_local_neighbours,
                             	  surfel_id_t const& target_surfel) const;
	*/
	



	struct min_entropy_order{
		bool operator ()(entropy_surfel* const entropy_first, entropy_surfel* const entropy_second){

			// true  : first goes to the front, second to the back
			// false : first goes to the back, first to the front 

			// if first is not valid, sort it to the front
			if ( entropy_first->validity == false && entropy_second->validity == true) {
				return true;
			} else if (entropy_first->validity == true && entropy_second->validity == true){
				if(entropy_first->entropy > entropy_second->entropy) {
					return true;
				}
			}

			return false;
			/*else {
				//if first is valid, but second not, sort second to the front
				if( entropy_second->validity == false) {
					return false;
				} else {

					//if both are valid, sort the one with the lowest entropy to the back (such that we can easily pop it)
					if(entropy_first->entropy > entropy_second->entropy) {
						return true;
					} else {
						return false;
					}
				}
			}

			std::cout << "Waiting...\n";
			*/
		}
	};

    void
	merge(entropy_surfel* current_entropy_surfel,
      std::vector<entropy_surfel*>& entropy_surfel_array,
      size_t num_remaining_valid_surfel, size_t num_desired_surfel) const;
	

};

// solution for accessing underlying priority-queue container taken from:
// http://stackoverflow.com/questions/1185252/
//        is-there-a-way-to-access-the-underlying-container-of-stl-container-adaptors
template <class T, class S, class C>
    S& Container(std::priority_queue<T, S, C>& q) {
        struct HackedQueue : private std::priority_queue<T, S, C> {
            static S& Container(std::priority_queue<T, S, C>& q) {
                return q.*&HackedQueue::c;
            }
        };
    return HackedQueue::Container(q);
}

} // namespace pre
} // namespace lamure

#endif // PRE_REDUCTION_ENTROPY_H_
