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
}



surfel_mem_array reduction_hierarchical_clustering::
create_lod(real& reduction_error,
			const std::vector<surfel_mem_array*>& input,
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

    // Set global parameters depending on input parameters.
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
	//std::cout << "new node surfel count: " << surfels.length() << " / " << surfels_per_node << std::endl;

	reduction_error = 0;

	return surfels;
}



std::vector<std::vector<surfel*>> reduction_hierarchical_clustering::
split_point_cloud(const std::vector<surfel*>& input_surfels, uint32_t max_cluster_size, real max_variation, const uint32_t& max_clusters) const
{
	std::vector<std::vector<surfel*>> output_clusters;
	output_clusters.push_back(input_surfels);
	bool splitting = true;

	// TODO: use of priority queue.
	int repeat_counter = 0;

	while(output_clusters.size() < max_clusters && splitting)
	{
		std::vector<surfel*> current_surfels = output_clusters.front();
		output_clusters.erase(output_clusters.begin());

		vec3r centroid;
		scm::math::mat3d covariance_matrix = calculate_covariance_matrix(current_surfels, centroid);
		vec3f normal;
		real variation = calculate_variation(covariance_matrix, normal);

		if(current_surfels.size() > max_cluster_size || variation > max_variation)
		{
			std::vector<surfel*> new_surfels_one;
			std::vector<surfel*> new_surfels_two;

			// Split the surfels into two sub-groups along splitting plane defined by eigenvector.
			for(uint32_t surfel_index = 0; surfel_index < current_surfels.size(); ++surfel_index)
			{
				surfel* current_surfel = current_surfels.at(surfel_index);
				real surfel_side = point_plane_distance(centroid, normal, current_surfel->pos());

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
				output_clusters.push_back(new_surfels_one);
			}
			if(new_surfels_two.size() > 0)
			{
				output_clusters.push_back(new_surfels_two);
			}

			//std::cout << new_surfels_one.size() << " / " << new_surfels_two.size() << " --- " << output_clusters.size() << std::endl;
		}
		else
		{
			output_clusters.push_back(current_surfels);

			++repeat_counter;
			if(repeat_counter > 10)
			{
				repeat_counter = 0;

				max_cluster_size = max_cluster_size * 3 / 4;
				max_variation = max_variation * 3 / 4;
			}
		}
	}

	return output_clusters;
}



scm::math::mat3d reduction_hierarchical_clustering::
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



real reduction_hierarchical_clustering::
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



vec3r reduction_hierarchical_clustering::
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



surfel reduction_hierarchical_clustering::
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
		radius = radius + current_surfel->radius();
	}

	centroid = centroid / surfels_to_sample.size();
	normal = normal / surfels_to_sample.size();
	color_overrun = color_overrun / surfels_to_sample.size();
	//radius = radius * 0.66666;	// TODO: proper radius calculation

	surfel new_surfel;
	new_surfel.pos() = centroid;
	new_surfel.normal() = normal;
	new_surfel.color() = vec3b(color_overrun.x, color_overrun.y, color_overrun.z);
	new_surfel.radius() = radius;

	return new_surfel;
}



real reduction_hierarchical_clustering::
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



void reduction_hierarchical_clustering::
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



void reduction_hierarchical_clustering::
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