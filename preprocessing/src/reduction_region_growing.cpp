// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/reduction_region_growing.h>

namespace lamure {
namespace pre {

reduction_region_growing::
reduction_region_growing()
{
	maximum_bound_ = 10.0;
	neighbour_growth_rate_ = 50;
}



surfel_mem_array reduction_region_growing::
create_lod(real& reduction_error,
          const std::vector<surfel_mem_array*>& input,
          const uint32_t surfels_per_node) const
{
	// Create a single surfel vector to sample from.
	std::vector<surfel*> surfels_to_sample;
	for (uint32_t child_mem_array_index = 0; child_mem_array_index < input.size(); ++child_mem_array_index)
    {
		surfel_mem_array* child_mem_array = input.at(child_mem_array_index);

    	for (uint32_t surfel_index = 0; surfel_index < child_mem_array->length(); ++surfel_index)
    	{
    		surfels_to_sample.push_back(&child_mem_array->mem_data()->at(surfel_index));
    	}
    }

    surfel_mem_array resulting_mem_array(std::make_shared<surfel_vector>(surfel_vector()), 0, 0);

	// Choose a random seed point p0.
	int random_index = std::rand() % surfels_to_sample.size();
	surfel* p_i = surfels_to_sample.at(random_index);

	std::vector<surfel*> current_cluster;
	current_cluster.push_back(p_i);
	surfels_to_sample.erase(surfels_to_sample.begin() + random_index);

	// get neighbours -> need access to bvh?
	std::vector<surfel*> current_neighbours;
	uint32_t neighbour_index = 0;

	// Grow cluster until stopping condition reached.
	while (!reached_maximum_bound(current_cluster))
	{
		// get new neighbours if running out of neighbours

		surfel* surfel_to_add = current_neighbours.at(neighbour_index);
		current_cluster.push_back(surfel_to_add);
		++neighbour_index;

		surfels_to_sample.erase(std::remove(surfels_to_sample.begin(), surfels_to_sample.end(), surfel_to_add), surfels_to_sample.end());
	}

	surfel* sampled_surfel = sample_point_from_cluster(current_cluster);
	resulting_mem_array.mem_data()->push_back(*sampled_surfel);


	// Filler for now: pick random surfels.
	for (uint32_t index = resulting_mem_array.length(); index < surfels_per_node; ++index)
	{
		random_index = rand() % surfels_to_sample.size();
		surfel* random_surfel = surfels_to_sample.at(random_index);
		resulting_mem_array.mem_data()->push_back(*random_surfel);
	}

	resulting_mem_array.set_length(resulting_mem_array.mem_data()->size());
	reduction_error = 0; // TODO
	return resulting_mem_array;
}



bool reduction_region_growing::
reached_maximum_bound(const std::vector<surfel*>& input_cluster) const
{
	if (input_cluster.size() < 2)
	{
		return false;
	}

	// Calculate current centroid to get a reference point.
	vec3r centroid = vec3r(0);
	for (uint32_t surfel_index = 0; surfel_index < input_cluster.size(); ++surfel_index)
	{
		surfel* current_surfel = input_cluster.at(surfel_index);
		centroid += current_surfel->pos();
	}

	centroid /= input_cluster.size();

	// Get largest distance to centroid in current cluster.
	real max_distance_to_centroid = 0;
	for (uint32_t surfel_index = 0; surfel_index < input_cluster.size(); ++surfel_index)
	{
		surfel* current_surfel = input_cluster.at(surfel_index);
		real current_distance = scm::math::distance(centroid, current_surfel->pos());

		if (current_distance > max_distance_to_centroid)
		{
			max_distance_to_centroid = current_distance;
		}
	}

	return max_distance_to_centroid > maximum_bound_;
}



surfel* reduction_region_growing::
sample_point_from_cluster(const std::vector<surfel*>& input_cluster) const
{
	// Calculate current centroid as new surfel position.
	vec3r new_position = vec3r(0);
	// TODO: also interpolate normal, colour, radius

	for (uint32_t surfel_index = 0; surfel_index < input_cluster.size(); ++surfel_index)
	{
		surfel* current_surfel = input_cluster.at(surfel_index);
		new_position += current_surfel->pos();
	}

	// Create new surfel and set position.
	new_position /= input_cluster.size();
	surfel* new_surfel = new surfel();
	new_surfel->pos() = new_position;

	return new_surfel;
}


} // namespace pre
} // namespace lamure