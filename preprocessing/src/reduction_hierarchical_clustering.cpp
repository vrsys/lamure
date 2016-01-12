// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/reduction_hierarchical_clustering.h>
//#include <CGAL/Eigen_vcm_traits.h>


namespace lamure {
namespace pre {

reduction_hierarchical_clustering::
reduction_hierarchical_clustering()
{
	maximum_variation_ = 1.0 / 3.0;
}



surfel_mem_array reduction_hierarchical_clustering::
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

    // Set global parameters depending on input parameters.
    uint32_t maximum_cluster_size = (surfels_to_sample.size() / surfels_per_node) * 2;

	std::vector<std::vector<surfel*>> clusters = split_point_cloud(surfels_to_sample, maximum_cluster_size);
	std::cout << "cluster count: " << clusters.size() << std::endl;



	// TODO: what to do if #clusters < surfels_per_node or #clusters > surfels_per_node



	// Generate positions from clusters.
	surfel_mem_array surfels(std::make_shared<surfel_vector>(surfel_vector()), 0, 0);

	for(uint32_t cluster_index = 0; cluster_index < clusters.size(); ++cluster_index)
	{
		// TODO: This should never happen, just test code.
		if(surfels.mem_data()->size() == surfels_per_node)
		{
			break;
		}

		Vector_3 cluster_centroid = calculate_centroid(clusters.at(cluster_index));
		
		surfel new_surfel;
		
		vec3r new_pos;
		new_pos.x = cluster_centroid.x();
		new_pos.y = cluster_centroid.y();
		new_pos.z = cluster_centroid.z();

		new_surfel.pos() = new_pos;

		surfels.mem_data()->push_back(new_surfel);
	}
	surfels.set_length(surfels.mem_data()->size());

	reduction_error = 0;

	return surfels;
}



std::vector<std::vector<surfel*>> reduction_hierarchical_clustering::
split_point_cloud(const std::vector<surfel*>& input_surfels, const uint32_t& max_cluster_size) const
{
	Matrix covariance_matrix = calculate_covariance_matrix(input_surfels);
	real variation = calculate_variation(covariance_matrix);

	std::vector<std::vector<surfel*>> output_clusters;

	if(input_surfels.size() > max_cluster_size || variation > maximum_variation_)
	{
		std::vector<surfel*> new_surfels_one;
		std::vector<surfel*> new_surfels_two;

		// TODO: split current surfels into two subgroups, split again with two new subgroups
		for(uint32_t surfel_index = 0; surfel_index < input_surfels.size(); ++surfel_index)
		{
			// Test code
			if(surfel_index % 2 == 0)
			{
				new_surfels_one.push_back(input_surfels.at(surfel_index));
			}
			else
			{
				new_surfels_two.push_back(input_surfels.at(surfel_index));
			}
		}

		std::vector<std::vector<surfel*>> result_one = split_point_cloud(new_surfels_one, max_cluster_size);
		std::vector<std::vector<surfel*>> result_two = split_point_cloud(new_surfels_two, max_cluster_size);

		output_clusters.insert(output_clusters.end(), result_one.begin(), result_one.end());
		output_clusters.insert(output_clusters.end(), result_two.begin(), result_two.end());
	}
	else
	{
		// If no split is necessary, the input points create a new cluster.
		output_clusters.push_back(input_surfels);
	}

	return output_clusters;
}



Matrix reduction_hierarchical_clustering::
calculate_covariance_matrix(const std::vector<surfel*>& surfels_to_sample) const
{
	Vector_3 centroid = calculate_centroid(surfels_to_sample);

	Matrix covariance_matrix = Matrix((int)surfels_to_sample.size(), 3);
	for(uint32_t surfel_index = 0; surfel_index < surfels_to_sample.size(); ++surfel_index)
	{
		surfel* current_surfel = surfels_to_sample.at(surfel_index);
		Vector_3 current_point = Vector_3(current_surfel->pos().x, current_surfel->pos().y, current_surfel->pos().z);
		Vector_3 covariance_input = current_point - centroid;

		covariance_matrix(surfel_index, 0) = (real)covariance_input[0];
		covariance_matrix(surfel_index, 1) = (real)covariance_input[1];
		covariance_matrix(surfel_index, 2) = (real)covariance_input[2];
	}

	covariance_matrix = Linear_Algebra::transpose(covariance_matrix) * covariance_matrix;
	return covariance_matrix;
}



real reduction_hierarchical_clustering::
calculate_variation(const Matrix& covariance_matrix) const
{
	// For details, see:
	// http://doc.cgal.org/latest/Point_set_processing_3/classVCMTraits.html
	real cov[] = {
		covariance_matrix(0, 0), 
		covariance_matrix(0, 1), 
		covariance_matrix(0, 2), 
		covariance_matrix(1, 1), 
		covariance_matrix(1, 2), 
		covariance_matrix(2, 2)};

	real eigenvalues[3];
	//diagonalize_selfadjoint_covariance_matrix(cov, eigenvalues);
	return 0;
}



Vector_3 reduction_hierarchical_clustering::
calculate_centroid(const std::vector<surfel*>& surfels_to_sample) const
{
	Vector_3 centroid = Vector_3(0, 0, 0);

	for(uint32_t surfel_index = 0; surfel_index < surfels_to_sample.size(); ++surfel_index)
	{
		surfel* current_surfel = surfels_to_sample.at(surfel_index);
		Vector_3 pos = Vector_3(current_surfel->pos().x, current_surfel->pos().y, current_surfel->pos().z);

		centroid = centroid + pos;
	}

	return centroid / surfels_to_sample.size();
}

} // namespace pre
} // namespace lamure