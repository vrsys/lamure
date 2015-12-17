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
    std::vector<std::vector<surfel*>> clusters;

    // Calculate a dynamic maximum bound depending on the provided surfel data.
    real maximum_bound = find_maximum_bound(surfels_to_sample) / surfels_to_sample.size();

	// Choose a random seed point p0.
	uint32_t random_index = (uint32_t)(std::rand() % surfels_to_sample.size());
	surfel* p_i = surfels_to_sample.at(random_index);
	surfels_to_sample.erase(std::find(surfels_to_sample.begin(), surfels_to_sample.end(), p_i));

	// Continue until every surfel has been taken into account.
	while (surfels_to_sample.size() > 0)
	{
		std::vector<surfel*> current_cluster;
		current_cluster.push_back(p_i);
		std::vector<std::pair<surfel*, real>> current_neighbours;

		// Grow cluster until stopping condition reached.
		while (!reached_maximum_bound(current_cluster, maximum_bound) && surfels_to_sample.size() > 0)
		{
			// Get new neighbours if running out of neighbours.
			if (current_neighbours.size() == 0)
			{
				current_neighbours = find_neighbours(surfels_to_sample, p_i, neighbour_growth_rate_);
			}

			// Add surfel to cluster.
			surfel* surfel_to_add = current_neighbours.front().first;
			current_cluster.push_back(surfel_to_add);

			// Remove surfel from list of still available surfels and neighbours.
			current_neighbours.erase(current_neighbours.begin());
			surfels_to_sample.erase(std::find(surfels_to_sample.begin(), surfels_to_sample.end(), surfel_to_add));
		}

		if (surfels_to_sample.size() > 0)
		{
			// Last element is outside of maximum bound, use it as new start point to sample.
			p_i = current_cluster.back();
			current_cluster.pop_back();

			std::vector<surfel*>::iterator remove_iter = std::find(surfels_to_sample.begin(), surfels_to_sample.end(), p_i);
			if (remove_iter != surfels_to_sample.end())
			{
				surfels_to_sample.erase(remove_iter);
			}
		}

		// Save created cluster for further investigation.
		clusters.push_back(current_cluster);
	}

	for (uint32_t cluster_index = 0; cluster_index < clusters.size(); ++cluster_index)
	{
		// Create surfel resulting from cluster and save it.
		surfel* sampled_surfel = sample_point_from_cluster(clusters.at(cluster_index));
		resulting_mem_array.mem_data()->push_back(*sampled_surfel);
	}

	std::cout << "sampled clusters: " << clusters.size() << std::endl;

	// Cutter for now: when too long, simply cut surfels.
	while (resulting_mem_array.length() > surfels_per_node)
	{
		resulting_mem_array.mem_data()->pop_back();
	}

	// Filler for now: pick random surfels.
	for (uint32_t index = resulting_mem_array.length(); index < surfels_per_node; ++index)
	{
		int random_index_memarray = rand() % input.size();
		int random_index_surfel = rand() % input.at(random_index_memarray)->length();

		surfel* random_surfel = &input.at(random_index_memarray)->mem_data()->at(random_index_surfel);
		resulting_mem_array.mem_data()->push_back(*random_surfel);
	}

	resulting_mem_array.set_length(resulting_mem_array.mem_data()->size());
	reduction_error = 0; // TODO
	return resulting_mem_array;
}



real reduction_region_growing::
find_maximum_bound(const std::vector<surfel*>& input_cluster) const
{
	if (input_cluster.size() < 2)
	{
		return 0;
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

	return max_distance_to_centroid;	
}



bool reduction_region_growing::
reached_maximum_bound(const std::vector<surfel*>& input_cluster, const real& maximum_bound) const
{
	return find_maximum_bound(input_cluster) > maximum_bound;
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



std::vector<std::pair<surfel*, real>> reduction_region_growing::
find_neighbours(const std::vector<surfel*>& cluster, const surfel* core_surfel, const uint32_t& number_neighbours) const
{
    vec3r center = core_surfel->pos();

    std::vector<std::pair<surfel*, real>> candidates;
    real max_candidate_distance = std::numeric_limits<real>::infinity();

    // Iterate over given surfel space.
    for (uint32_t surfel_index = 0; surfel_index < cluster.size(); ++surfel_index)
    {
    	surfel* current_surfel = cluster.at(surfel_index);
        
        if (current_surfel != core_surfel)
        {
            real distance_to_center = scm::math::length_sqr(center - current_surfel->pos());

            if (candidates.size() < number_neighbours || (distance_to_center < max_candidate_distance))
            {
            	// Remove candidate if maximum number found already.
                if (candidates.size() == number_neighbours)
                    candidates.pop_back();

                candidates.push_back(std::make_pair(current_surfel, distance_to_center));

                // Sort candidates (closest neighbour is first).
                for (uint16_t k = candidates.size() - 1; k > 0; --k)
                {
                    if (candidates[k].second < candidates[k - 1].second)
                    {
                        std::swap(candidates[k], candidates[k - 1]);
                    }
                    else
                        break;
                }

                max_candidate_distance = candidates.back().second;
            }
        }
    }

    return candidates;
}

} // namespace pre
} // namespace lamure