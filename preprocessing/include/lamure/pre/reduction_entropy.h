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


	struct neighbour_with_id{
		surfel neighbour_surfel;
		real distance_to_neighbour;
		uint32_t id;
	};

	uint16_t update_level(uint16_t level) {return level+1;}
	float compute_entropy(uint16_t level, vec3f own_normal, std::vector<std::pair<surfel, real>> neighbours);
	void initialize_queue(const std::vector<surfel_mem_array*>& input, const bvh& tree);
	vec3r average_position(std::vector<neighbour_with_id> neighbours);
	vec3f average_normal(std::vector<neighbour_with_id> neighbours);    
	real average_radius(std::vector<neighbour_with_id> neighbours);



	

	  struct entropy_surfel{
		surfel current_surfel;
		uint32_t id;
		std::vector<neighbour_with_id> neighbours;
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
