// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/reduction_hierarchical_clustering_mk2.h>
#include <queue>


namespace lamure {
namespace pre {

reduction_hierarchical_clustering_mk2::
reduction_hierarchical_clustering_mk2()
{
}



surfel_mem_array reduction_hierarchical_clustering_mk2::
create_lod(real& reduction_error,
			const std::vector<surfel_mem_array*>& input,
			const real avg_radius_all_nodes,
            const uint32_t surfels_per_node,
          	const bvh& tree,
          	const size_t start_node_id) const
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

    // Set initial parameters depending on input parameters.
    // These splitting thresholds adapt during the execution of the algorithm.
    // Maximum possible variation is 1/3.
    // TODO: optimize chosen parameters
    uint32_t maximum_cluster_size = (surfels_to_sample.size() / surfels_per_node) * 2;
    real maximum_variation = 0.1;
	
	std::vector<std::vector<surfel*>> clusters;
	clusters = split_point_cloud(surfels_to_sample, maximum_cluster_size, maximum_variation, surfels_per_node);

	// Generate surfels from clusters.
	surfel_mem_array surfels(std::make_shared<surfel_vector>(surfel_vector()), 0, 0);

	for(uint32_t cluster_index = 0; cluster_index < clusters.size(); ++cluster_index)
	{
		surfel new_surfel = create_surfel_from_cluster(clusters.at(cluster_index));
		surfels.mem_data()->push_back(new_surfel);
	}

	surfels.set_length(surfels.mem_data()->size());

	reduction_error = 0;

	return surfels;
}



std::vector<std::vector<surfel*>> reduction_hierarchical_clustering_mk2::
split_point_cloud(const std::vector<surfel*>& input_surfels, uint32_t max_cluster_size, real max_variation, const uint32_t& max_clusters) const
{
	std::priority_queue<hierarchical_cluster_mk2, 
						std::vector<hierarchical_cluster_mk2>, 
						cluster_comparator_mk2> cluster_queue;
	cluster_queue.push(calculate_cluster_data(input_surfels));

	while(cluster_queue.size() < max_clusters)
	{
		hierarchical_cluster_mk2 current_cluster = cluster_queue.top();
		cluster_queue.pop();

		real current_max_variation = std::max(current_cluster.variation_pos, std::max(current_cluster.variation_normal, current_cluster.variation_color));

		if(current_cluster.surfels.size() > max_cluster_size || current_max_variation > max_variation)
		{
			std::vector<surfel*> new_surfels_one;
			std::vector<surfel*> new_surfels_two;

			// Choose splitting mode by picking strongest variation (either position, normal or color variation).
			uint8_t split_mode = 0;
			if(current_cluster.variation_normal > current_cluster.variation_pos)
			{
				split_mode = 1;
			}
			if(current_cluster.variation_color > current_cluster.variation_normal && current_cluster.variation_color > current_cluster.variation_pos)
			{
				split_mode = 2;
			}

			// Split the surfels into two sub-groups along splitting plane defined by eigenvector.
			for(uint32_t surfel_index = 0; surfel_index < current_cluster.surfels.size(); ++surfel_index)
			{
				surfel* current_surfel = current_cluster.surfels.at(surfel_index);
				real surfel_side = 0;

				// Split among axis defined by splitting mode.
				if(split_mode == 0)
				{
					surfel_side = point_plane_distance(current_cluster.centroid_pos, current_cluster.normal_pos, current_surfel->pos());
				}
				else if(split_mode == 1)
				{
					vec3r normal_r(current_surfel->normal().x, current_surfel->normal().y, current_surfel->normal().z);
					surfel_side = point_plane_distance(current_cluster.centroid_normal, current_cluster.normal_normal, normal_r);
				}
				else if(split_mode == 2)
				{
					vec3r color_r(current_surfel->color().x, current_surfel->color().y, current_surfel->color().z);
					surfel_side = point_plane_distance(current_cluster.centroid_color, current_cluster.normal_color, color_r);
				}


				if(surfel_side >= 0)
				{
					new_surfels_one.push_back(current_surfel);
				}
				else
				{
					new_surfels_two.push_back(current_surfel);
				}
			}

			if(new_surfels_one.size() > 0)
			{
				cluster_queue.push(calculate_cluster_data(new_surfels_one));
			}
			if(new_surfels_two.size() > 0)
			{
				cluster_queue.push(calculate_cluster_data(new_surfels_two));
			}
		}
		else
		{
			cluster_queue.push(current_cluster);

			max_cluster_size = max_cluster_size * 3 / 4;
			max_variation = max_variation * 0.75;
		}
	}

	std::vector<std::vector<surfel*>> output_clusters;
	while(cluster_queue.size() > 0)
	{
		output_clusters.push_back(cluster_queue.top().surfels);
		cluster_queue.pop();
	}

	return output_clusters;
}



hierarchical_cluster_mk2 reduction_hierarchical_clustering_mk2::
calculate_cluster_data(const std::vector<surfel*>& input_surfels) const
{
	vec3r centroid_pos;
	vec3r centroid_normal;
	vec3r centroid_color;

	scm::math::mat3d covariance_matrix_pos = calculate_covariance_matrix(input_surfels, centroid_pos);
	scm::math::mat3d covariance_matrix_normal = calculate_covariance_matrix_normal(input_surfels, centroid_normal);
	scm::math::mat3d covariance_matrix_color = calculate_covariance_matrix_color(input_surfels, centroid_color);

	vec3f normal_pos;
	vec3f normal_normal;
	vec3f normal_color;

	real variation_pos = calculate_variation(covariance_matrix_pos, normal_pos);
	real variation_normal = calculate_variation(covariance_matrix_normal, normal_normal);
	real variation_color = calculate_variation(covariance_matrix_color, normal_color);

	hierarchical_cluster_mk2 new_cluster;
	new_cluster.surfels = input_surfels;

	new_cluster.centroid_pos = centroid_pos;
	new_cluster.centroid_normal = centroid_normal;
	new_cluster.centroid_color = centroid_color;

	new_cluster.normal_pos = normal_pos;
	new_cluster.normal_normal = normal_normal;
	new_cluster.normal_color = normal_color;

	new_cluster.variation_pos = variation_pos;
	new_cluster.variation_normal = variation_normal;
	new_cluster.variation_color = variation_color;

	return new_cluster;
}



scm::math::mat3d reduction_hierarchical_clustering_mk2::
calculate_covariance_matrix(const std::vector<surfel*>& surfels_to_sample, vec3r& centroid) const
{
    scm::math::mat3d covariance_mat = scm::math::mat3d::zero();
    centroid = calculate_centroid(surfels_to_sample);

    for (uint32_t surfel_index = 0; surfel_index < surfels_to_sample.size(); ++surfel_index)
    {
		surfel* current_surfel = surfels_to_sample.at(surfel_index);
        
        covariance_mat.m00 += std::pow(current_surfel->pos().x-centroid.x, 2);
        covariance_mat.m01 += (current_surfel->pos().x-centroid.x) * (current_surfel->pos().y - centroid.y);
        covariance_mat.m02 += (current_surfel->pos().x-centroid.x) * (current_surfel->pos().z - centroid.z);

        covariance_mat.m03 += (current_surfel->pos().y-centroid.y) * (current_surfel->pos().x - centroid.x);
        covariance_mat.m04 += std::pow(current_surfel->pos().y-centroid.y, 2);
        covariance_mat.m05 += (current_surfel->pos().y-centroid.y) * (current_surfel->pos().z - centroid.z);

        covariance_mat.m06 += (current_surfel->pos().z-centroid.z) * (current_surfel->pos().x - centroid.x);
        covariance_mat.m07 += (current_surfel->pos().z-centroid.z) * (current_surfel->pos().y - centroid.y);
        covariance_mat.m08 += std::pow(current_surfel->pos().z-centroid.z, 2);
    }

    return covariance_mat;
}



scm::math::mat3d reduction_hierarchical_clustering_mk2::
calculate_covariance_matrix_normal(const std::vector<surfel*>& surfels_to_sample, vec3r& centroid) const
{
    scm::math::mat3d covariance_mat = scm::math::mat3d::zero();
    centroid = calculate_centroid_normal(surfels_to_sample);

    for (uint32_t surfel_index = 0; surfel_index < surfels_to_sample.size(); ++surfel_index)
    {
		surfel* current_surfel = surfels_to_sample.at(surfel_index);
        
        covariance_mat.m00 += std::pow(current_surfel->normal().x-centroid.x, 2);
        covariance_mat.m01 += (current_surfel->normal().x-centroid.x) * (current_surfel->normal().y - centroid.y);
        covariance_mat.m02 += (current_surfel->normal().x-centroid.x) * (current_surfel->normal().z - centroid.z);

        covariance_mat.m03 += (current_surfel->normal().y-centroid.y) * (current_surfel->normal().x - centroid.x);
        covariance_mat.m04 += std::pow(current_surfel->normal().y-centroid.y, 2);
        covariance_mat.m05 += (current_surfel->normal().y-centroid.y) * (current_surfel->normal().z - centroid.z);

        covariance_mat.m06 += (current_surfel->normal().z-centroid.z) * (current_surfel->normal().x - centroid.x);
        covariance_mat.m07 += (current_surfel->normal().z-centroid.z) * (current_surfel->normal().y - centroid.y);
        covariance_mat.m08 += std::pow(current_surfel->normal().z-centroid.z, 2);
    }

    return covariance_mat;
}



scm::math::mat3d reduction_hierarchical_clustering_mk2::
calculate_covariance_matrix_color(const std::vector<surfel*>& surfels_to_sample, vec3r& centroid) const
{
    scm::math::mat3d covariance_mat = scm::math::mat3d::zero();
    centroid = calculate_centroid_color(surfels_to_sample);

    for (uint32_t surfel_index = 0; surfel_index < surfels_to_sample.size(); ++surfel_index)
    {
		surfel* current_surfel = surfels_to_sample.at(surfel_index);
        
        covariance_mat.m00 += std::pow(current_surfel->color().x-centroid.x, 2);
        covariance_mat.m01 += (current_surfel->color().x-centroid.x) * (current_surfel->color().y - centroid.y);
        covariance_mat.m02 += (current_surfel->color().x-centroid.x) * (current_surfel->color().z - centroid.z);

        covariance_mat.m03 += (current_surfel->color().y-centroid.y) * (current_surfel->color().x - centroid.x);
        covariance_mat.m04 += std::pow(current_surfel->color().y-centroid.y, 2);
        covariance_mat.m05 += (current_surfel->color().y-centroid.y) * (current_surfel->color().z - centroid.z);

        covariance_mat.m06 += (current_surfel->color().z-centroid.z) * (current_surfel->color().x - centroid.x);
        covariance_mat.m07 += (current_surfel->color().z-centroid.z) * (current_surfel->color().y - centroid.y);
        covariance_mat.m08 += std::pow(current_surfel->color().z-centroid.z, 2);
    }

    return covariance_mat;
}



real reduction_hierarchical_clustering_mk2::
calculate_variation(const scm::math::mat3d& covariance_matrix, vec3f& normal) const
{
	//solve for eigenvectors
    real* eigenvalues = new real[3];
    real** eigenvectors = new real*[3];
    for (int i = 0; i < 3; ++i) {
       eigenvectors[i] = new real[3];
    }

    jacobi_rotation(covariance_matrix, eigenvalues, eigenvectors);

    real variation = eigenvalues[0] / (eigenvalues[0] + eigenvalues[1] + eigenvalues[2]);

    // Use eigenvector with highest magnitude as splitting plane normal.
    normal = scm::math::vec3f(eigenvectors[0][2], eigenvectors[1][2], eigenvectors[2][2]);

    /*for(uint32_t eigen_index = 1; eigen_index <= 2; ++eigen_index)
    {
    	vec3f other_normal = scm::math::vec3f(eigenvectors[0][eigen_index], eigenvectors[1][eigen_index], eigenvectors[2][eigen_index]);
    	
    	if(scm::math::length(normal) < scm::math::length(other_normal))
    	{
    		normal = other_normal;
    	}
    }*/

    delete[] eigenvalues;
    for (int i = 0; i < 3; ++i) {
       delete[] eigenvectors[i];
    }
    delete[] eigenvectors;

	return variation;
}



vec3r reduction_hierarchical_clustering_mk2::
calculate_centroid(const std::vector<surfel*>& surfels_to_sample) const
{
	vec3r centroid = vec3r(0, 0, 0);

	for(uint32_t surfel_index = 0; surfel_index < surfels_to_sample.size(); ++surfel_index)
	{
		surfel* current_surfel = surfels_to_sample.at(surfel_index);
		centroid = centroid + current_surfel->pos();
	}

	return centroid / surfels_to_sample.size();
}



vec3r reduction_hierarchical_clustering_mk2::
calculate_centroid_normal(const std::vector<surfel*>& surfels_to_sample) const
{
	vec3r centroid = vec3r(0, 0, 0);

	for(uint32_t surfel_index = 0; surfel_index < surfels_to_sample.size(); ++surfel_index)
	{
		surfel* current_surfel = surfels_to_sample.at(surfel_index);
		centroid = centroid + current_surfel->normal();
	}

	return centroid / surfels_to_sample.size();
}



vec3r reduction_hierarchical_clustering_mk2::
calculate_centroid_color(const std::vector<surfel*>& surfels_to_sample) const
{
	vec3r centroid = vec3r(0, 0, 0);

	for(uint32_t surfel_index = 0; surfel_index < surfels_to_sample.size(); ++surfel_index)
	{
		surfel* current_surfel = surfels_to_sample.at(surfel_index);
		centroid = centroid + current_surfel->color();
	}

	return centroid / surfels_to_sample.size();
}



surfel reduction_hierarchical_clustering_mk2::
create_surfel_from_cluster(const std::vector<surfel*>& surfels_to_sample) const
{
	vec3r centroid = vec3r(0, 0, 0);
	vec3f normal = vec3f(0, 0, 0);
	vec3r color_overrun = vec3r(0, 0, 0);
	real radius = 0;

	for(uint32_t surfel_index = 0; surfel_index < surfels_to_sample.size(); ++surfel_index)
	{
		surfel* current_surfel = surfels_to_sample.at(surfel_index);

		centroid = centroid + current_surfel->pos();
		normal = normal + current_surfel->normal();
		color_overrun = color_overrun + current_surfel->color();
	}

	centroid = centroid / surfels_to_sample.size();
	normal = normal / surfels_to_sample.size();
	color_overrun = color_overrun / surfels_to_sample.size();

	// Compute raius by taking max radius of cluster surfels and max distance from centroid.
	real highest_distance = 0;
	for(uint32_t surfel_index = 0; surfel_index < surfels_to_sample.size(); ++surfel_index)
	{
		surfel* current_surfel = surfels_to_sample.at(surfel_index);
		real distance_centroid_surfel = scm::math::length(centroid - current_surfel->pos());

		if(distance_centroid_surfel > highest_distance)
		{
			highest_distance = distance_centroid_surfel;
		}

		if(current_surfel->radius() > radius)
		{
			radius = current_surfel->radius();
		}
	}

	surfel new_surfel;
	new_surfel.pos() = centroid;
	new_surfel.normal() = normal;
	new_surfel.color() = vec3b(color_overrun.x, color_overrun.y, color_overrun.z);
	new_surfel.radius() = (radius + highest_distance);

	return new_surfel;
}



real reduction_hierarchical_clustering_mk2::
point_plane_distance(const vec3r& centroid, const vec3f& normal, const vec3r& point) const
{
	vec3f normalized_normal = scm::math::normalize(normal);
	vec3r w = centroid - point;
	real a = normalized_normal.x;
	real b = normalized_normal.y;
	real c = normalized_normal.z;

	real distance = (a * w.x + b * w.y + c * w.z) / sqrt(pow(a, 2) + pow(b, 2) + pow(c, 2));
	return distance;
}



void reduction_hierarchical_clustering_mk2::
jacobi_rotation(const scm::math::mat3d& _matrix, double* eigenvalues, double** eigenvectors) const
{
    unsigned int max_iterations = 10;
    double max_error = 0.00000001;
    unsigned int dim = 3;

    double iR1[3][3];
    iR1[0][0] = _matrix[0];
    iR1[0][1] = _matrix[1];
    iR1[0][2] = _matrix[2];
    iR1[1][0] = _matrix[3];
    iR1[1][1] = _matrix[4];
    iR1[1][2] = _matrix[5];
    iR1[2][0] = _matrix[6];
    iR1[2][1] = _matrix[7];
    iR1[2][2] = _matrix[8];


    for (uint32_t i = 0; i <= dim-1; ++i){
        eigenvectors[i][i] = 1.0;
        for (uint32_t j = 0; j <= dim-1; ++j){
            if (i != j) {
               eigenvectors[i][j] = 0.0;
            }
        }
    }

    uint32_t iteration = 0;
    uint32_t p, q;

    while (iteration < max_iterations){

        double max_diff = 0.0;

        for (uint32_t i = 0; i < dim-1 ; ++i){
            for (uint32_t j = i+1; j < dim ; ++j){
                long double diff = std::abs(iR1[i][j]);
                if (i != j && diff >= max_diff){
                    max_diff = diff;
                    p = i;
                    q = j;
                }
            }
        }

        if (max_diff < max_error){
            for (uint32_t i = 0; i < dim; ++i){
                eigenvalues[i] = iR1[i][i];
            }
            break;
        }

        long double x = -iR1[p][q];
        long double y = (iR1[q][q] - iR1[p][p]) / 2.0;
        long double omega = x / sqrt(x * x + y * y);

        if (y < 0.0) {
            omega = -omega;
        }

        long double sn = 1.0 + sqrt(1.0 - omega*omega);
        sn = omega / sqrt(2.0 * sn);
        long double cn = sqrt(1.0 - sn*sn);

        max_diff = iR1[p][p];

        iR1[p][p] = max_diff*cn*cn + iR1[q][q]*sn*sn + iR1[p][q]*omega;
        iR1[q][q] = max_diff*sn*sn + iR1[q][q]*cn*cn - iR1[p][q]*omega;
        iR1[p][q] = 0.0;
        iR1[q][p] = 0.0;

        for (uint32_t j = 0; j <= dim-1; ++j){
            if (j != p && j != q){
                max_diff = iR1[p][j];
                iR1[p][j] = max_diff*cn + iR1[q][j]*sn;
                iR1[q][j] = -max_diff*sn + iR1[q][j]*cn;
            }
        }

        for (uint32_t i = 0; i <= dim-1; ++i){
            if (i != p && i != q){
                max_diff = iR1[i][p];
                iR1[i][p] = max_diff*cn + iR1[i][q]*sn;
                iR1[i][q] = -max_diff*sn + iR1[i][q]*cn;
            }
        }

        for (uint32_t i = 0; i <= dim-1; ++i){
            max_diff = eigenvectors[i][p];
            eigenvectors[i][p] = max_diff*cn + eigenvectors[i][q]*sn;
            eigenvectors[i][q] = -max_diff*sn + eigenvectors[i][q]*cn;
        }

        ++iteration;
    }

    
    eigsrt_jacobi(dim, eigenvalues, eigenvectors);
}



void reduction_hierarchical_clustering_mk2::
eigsrt_jacobi(int dim, double* eigenvalues, double** eigenvectors) const
{
    for (int i = 0; i < dim; ++i)
    {
        int k = i;
        double v = eigenvalues[k];

        for (int j = i + 1; j < dim; ++j)
        {
            if (eigenvalues[j] < v) // eigenvalues[j] <= v ?
            { 
                k = j;
                v = eigenvalues[k];
            }
        }

        if (k != i)
        {
            eigenvalues[k] = eigenvalues[i];
            eigenvalues[i] = v;

            for (int j = 0; j < dim; ++j)
            {
                v = eigenvectors[j][i];
                eigenvectors[j][i] = eigenvectors[j][k];
                eigenvectors[j][k] = v;
            }
        }
    }
}

} // namespace pre
} // namespace lamure