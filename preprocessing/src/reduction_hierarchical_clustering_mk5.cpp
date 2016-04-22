// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/reduction_hierarchical_clustering_mk5.h>


namespace lamure {
namespace pre {

reduction_hierarchical_clustering_mk5::
reduction_hierarchical_clustering_mk5()
{
	color_space_mode_ = 2;
}



surfel_mem_array reduction_hierarchical_clustering_mk5::
create_lod(real_t& reduction_error,
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

    // Set initial parameters depending on input parameters.
    // These splitting thresholds adapt during the execution of the algorithm.
    // Maximum possible variation is 1/3.
    // TODO: optimize chosen parameters
    uint32_t maximum_cluster_size = (surfels_to_sample.size() / surfels_per_node) * 2;
    real_t maximum_variation_position = -1;
    real_t maximum_variation_color = 0.025;

	std::vector<std::vector<surfel*>> clusters;
	clusters = split_point_cloud(surfels_to_sample, maximum_cluster_size, maximum_variation_position, maximum_variation_color, surfels_per_node);

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



std::vector<std::vector<surfel*>> reduction_hierarchical_clustering_mk5::
split_point_cloud(const std::vector<surfel*>& input_surfels, 
				uint32_t max_cluster_size, 
				real_t max_variation_position,
				real_t max_variation_color, 
				const uint32_t& max_clusters) const
{
	std::priority_queue<hierarchical_cluster_mk5, 
						std::vector<hierarchical_cluster_mk5>, 
						cluster_comparator_mk5> cluster_queue;
	cluster_queue.push(calculate_cluster_data(input_surfels));

	while(cluster_queue.size() < max_clusters)
	{
		hierarchical_cluster_mk5 current_cluster = cluster_queue.top();
		cluster_queue.pop();

		// Initial maximum variation is defined by variation of first cluster.
		if(max_variation_position < 0)
		{
			max_variation_position = current_cluster.variation_pos;
		}

		// Only do color splitting if color variation is above threshold and cluster is small (which means it is deep in the splitting hierarchy).
		if(current_cluster.variation_color > max_variation_color && current_cluster.surfels.size() < (max_cluster_size * 2))
		{
			std::vector<surfel*> new_surfels_one;
			std::vector<surfel*> new_surfels_two;

			// Split the surfels into two sub-groups along splitting plane defined by eigenvector.
			for(uint32_t surfel_index = 0; surfel_index < current_cluster.surfels.size(); ++surfel_index)
			{
				surfel* current_surfel = current_cluster.surfels.at(surfel_index);
				
				vec3r_t color_trans = transform_color(current_surfel->color());
				real_t surfel_side = point_plane_distance(current_cluster.centroid_color, current_cluster.normal_color, color_trans);

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
				split_cluster_by_position(calculate_cluster_data(new_surfels_one), max_cluster_size, max_variation_position, cluster_queue);
			}
			if(new_surfels_two.size() > 0)
			{
				split_cluster_by_position(calculate_cluster_data(new_surfels_two), max_cluster_size, max_variation_position, cluster_queue);
			}
		}
		else if (current_cluster.surfels.size() > max_cluster_size || current_cluster.variation_pos > max_variation_position)
		{
			split_cluster_by_position(current_cluster, max_cluster_size, max_variation_position, cluster_queue);
		}
		else
		{
			cluster_queue.push(current_cluster);

			max_cluster_size = max_cluster_size * 3 / 4;
			max_variation_position = max_variation_position * 0.75;
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



void reduction_hierarchical_clustering_mk5::
split_cluster_by_position(const hierarchical_cluster_mk5& input_cluster,
	const uint32_t& max_cluster_size,
	const real_t& max_variation,
	std::priority_queue<hierarchical_cluster_mk5, std::vector<hierarchical_cluster_mk5>, cluster_comparator_mk5>& cluster_queue) const
{
	if(input_cluster.surfels.size() > max_cluster_size || input_cluster.variation_pos > max_variation)
	{
		std::vector<surfel*> new_surfels_one;
		std::vector<surfel*> new_surfels_two;

		// Split the surfels into two sub-groups along splitting plane defined by eigenvector.
		for(uint32_t surfel_index = 0; surfel_index < input_cluster.surfels.size(); ++surfel_index)
		{
			surfel* current_surfel = input_cluster.surfels.at(surfel_index);
			real_t surfel_side = point_plane_distance(input_cluster.centroid_pos, input_cluster.normal_pos, current_surfel->pos());

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
		cluster_queue.push(input_cluster);
	}			
}



hierarchical_cluster_mk5 reduction_hierarchical_clustering_mk5::
calculate_cluster_data(const std::vector<surfel*>& input_surfels) const
{
	vec3r_t centroid_pos;
	vec3r_t centroid_color;

	mat3d_t covariance_matrix_pos = calculate_covariance_matrix(input_surfels, centroid_pos);
	mat3d_t covariance_matrix_color = calculate_covariance_matrix_color(input_surfels, centroid_color);

	vec3f_t normal_pos;
	vec3f_t normal_color;

	real_t variation_pos = calculate_variation(covariance_matrix_pos, normal_pos);
	real_t variation_color = calculate_variation(covariance_matrix_color, normal_color);

	hierarchical_cluster_mk5 new_cluster;
	new_cluster.surfels = input_surfels;

	new_cluster.centroid_pos = centroid_pos;
	new_cluster.centroid_color = centroid_color;

	new_cluster.normal_pos = normal_pos;
	new_cluster.normal_color = normal_color;

	new_cluster.variation_pos = variation_pos;
	new_cluster.variation_color = variation_color;

	return new_cluster;
}



mat3d_t reduction_hierarchical_clustering_mk5::
calculate_covariance_matrix(const std::vector<surfel*>& surfels_to_sample, vec3r_t& centroid) const
{
    mat3d_t covariance_mat = mat3d_t::zero();
    centroid = calculate_centroid(surfels_to_sample);
    
    // TODO: The rounding is only necessary for some models (infinite loop otherwise), it would be good to get rid of it completely though.
    bool roundingNecessary = true;

    for (uint32_t surfel_index = 0; surfel_index < surfels_to_sample.size(); ++surfel_index)
    {
		surfel* current_surfel = surfels_to_sample.at(surfel_index);
        
        covariance_mat.m00 += std::pow(current_surfel->pos().x_-centroid.x_, 2);
        covariance_mat.m01 += (current_surfel->pos().x_-centroid.x_) * (current_surfel->pos().y_ - centroid.y_);
        covariance_mat.m02 += (current_surfel->pos().x_-centroid.x_) * (current_surfel->pos().z_ - centroid.z_);

        covariance_mat.m03 += (current_surfel->pos().y_-centroid.y_) * (current_surfel->pos().x_ - centroid.x_);
        covariance_mat.m04 += std::pow(current_surfel->pos().y_-centroid.y_, 2);
        covariance_mat.m05 += (current_surfel->pos().y_-centroid.y_) * (current_surfel->pos().z_ - centroid.z_);

        covariance_mat.m06 += (current_surfel->pos().z_-centroid.z_) * (current_surfel->pos().x_ - centroid.x_);
        covariance_mat.m07 += (current_surfel->pos().z_-centroid.z_) * (current_surfel->pos().y_ - centroid.y_);
        covariance_mat.m08 += std::pow(current_surfel->pos().z_-centroid.z_, 2);
    }

    if (roundingNecessary)
    {
    	// Precision limitation because of rounding errors otherwise.
	    for (int index = 0; index < 9; ++ index)
	    {
	    	covariance_mat[index] = round(covariance_mat[index] * std::pow(10.0, 9.0)) / std::pow(10.0, 9.0);
	    }
    }

    return covariance_mat;
}



mat3d_t reduction_hierarchical_clustering_mk5::
calculate_covariance_matrix_color(const std::vector<surfel*>& surfels_to_sample, vec3r_t& centroid) const
{
    mat3d_t covariance_mat = mat3d_t::zero();
    centroid = calculate_centroid_color(surfels_to_sample);

    for (uint32_t surfel_index = 0; surfel_index < surfels_to_sample.size(); ++surfel_index)
    {
		surfel* current_surfel = surfels_to_sample.at(surfel_index);
		vec3r_t color_trans = transform_color(current_surfel->color());

        covariance_mat.m00 += std::pow(color_trans.x_ - centroid.x_, 2);
        covariance_mat.m01 += (color_trans.x_-centroid.x_) * (color_trans.y_ - centroid.y_);
        covariance_mat.m02 += (color_trans.x_-centroid.x_) * (color_trans.z_ - centroid.z_);

        covariance_mat.m03 += (color_trans.y_-centroid.y_) * (color_trans.x_ - centroid.x_);
        covariance_mat.m04 += std::pow(color_trans.y_ - centroid.y_, 2);
        covariance_mat.m05 += (color_trans.y_-centroid.y_) * (color_trans.z_ - centroid.z_);

        covariance_mat.m06 += (color_trans.z_-centroid.z_) * (color_trans.x_ - centroid.x_);
        covariance_mat.m07 += (color_trans.z_-centroid.z_) * (color_trans.y_ - centroid.y_);
        covariance_mat.m08 += std::pow(color_trans.z_ - centroid.z_, 2);
    }

    // Precision limitation because of rounding errors otherwise.
    for (int index = 0; index < 9; ++ index)
    {
    	covariance_mat[index] = round(covariance_mat[index] * std::pow(10.0, 9.0)) / std::pow(10.0, 9.0);
    }

    return covariance_mat;
}



real_t reduction_hierarchical_clustering_mk5::
calculate_variation(const mat3d_t& covariance_matrix, vec3f_t& normal) const
{
	//solve for eigenvectors
    real_t* eigenvalues = new real_t[3];
    real_t** eigenvectors = new real_t*[3];
    for (int i = 0; i < 3; ++i) {
       eigenvectors[i] = new real_t[3];
    }

    jacobi_rotation(covariance_matrix, eigenvalues, eigenvectors);

    real_t variation = eigenvalues[0] / (eigenvalues[0] + eigenvalues[1] + eigenvalues[2]);

    // Use eigenvector with highest magnitude as splitting plane normal.
    normal = vec3f_t(eigenvectors[0][2], eigenvectors[1][2], eigenvectors[2][2]);

    delete[] eigenvalues;
    for (int i = 0; i < 3; ++i) {
       delete[] eigenvectors[i];
    }
    delete[] eigenvectors;

	return variation;
}



vec3r_t reduction_hierarchical_clustering_mk5::
calculate_centroid(const std::vector<surfel*>& surfels_to_sample) const
{
	vec3r_t centroid = vec3r_t(0, 0, 0);

	for(uint32_t surfel_index = 0; surfel_index < surfels_to_sample.size(); ++surfel_index)
	{
		surfel* current_surfel = surfels_to_sample.at(surfel_index);
		centroid = centroid + current_surfel->pos();
	}

	return centroid / surfels_to_sample.size();
}



vec3r_t reduction_hierarchical_clustering_mk5::
calculate_centroid_color(const std::vector<surfel*>& surfels_to_sample) const
{
	vec3r_t centroid = vec3r_t(0, 0, 0);

	for(uint32_t surfel_index = 0; surfel_index < surfels_to_sample.size(); ++surfel_index)
	{
		surfel* current_surfel = surfels_to_sample.at(surfel_index);
		vec3r_t color_trans = transform_color(current_surfel->color());
		centroid = centroid + color_trans;
	}

	return centroid / surfels_to_sample.size();
}



surfel reduction_hierarchical_clustering_mk5::
create_surfel_from_cluster(const std::vector<surfel*>& surfels_to_sample) const
{
	vec3r_t centroid = vec3r_t(0, 0, 0);
	vec3f_t normal = vec3f_t(0, 0, 0);
	vec3r_t color_overrun = vec3r_t(0, 0, 0);
	real_t radius = 0;

	for(uint32_t surfel_index = 0; surfel_index < surfels_to_sample.size(); ++surfel_index)
	{
		surfel* current_surfel = surfels_to_sample.at(surfel_index);

		centroid = centroid + current_surfel->pos();
		normal = normal + current_surfel->normal();
		color_overrun = color_overrun + current_surfel->color();
	}

	centroid = centroid / (double)surfels_to_sample.size();
	normal = normal / surfels_to_sample.size();
	color_overrun = color_overrun / surfels_to_sample.size();

	// Compute radius by taking max radius of cluster surfels and max distance from centroid.
	real_t highest_distance = 0;
	for(uint32_t surfel_index = 0; surfel_index < surfels_to_sample.size(); ++surfel_index)
	{
		surfel* current_surfel = surfels_to_sample.at(surfel_index);
		real_t distance_centroid_surfel = lamure::math::length(centroid - current_surfel->pos());

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
	new_surfel.color() = vec3b_t(color_overrun.x_, color_overrun.y_, color_overrun.z_);
	new_surfel.radius() = (radius + highest_distance);

	return new_surfel;
}



real_t reduction_hierarchical_clustering_mk5::
point_plane_distance(const vec3r_t& centroid, const vec3f_t& normal, const vec3r_t& point) const
{
	vec3f_t normalized_normal = lamure::math::normalize(normal);
	vec3r_t w = centroid - point;
	real_t a = normalized_normal.x_;
	real_t b = normalized_normal.y_;
	real_t c = normalized_normal.z_;

	real_t distance = (a * w.x_ + b * w.y_ + c * w.z_) / sqrt(pow(a, 2) + pow(b, 2) + pow(c, 2));
	return distance;
}



vec3r_t reduction_hierarchical_clustering_mk5::
transform_RGB_to_XYZ(const vec3b_t& color) const
{
	vec3r_t color_rgb(color.x_, color.y_, color.z_);

	// http://stackoverflow.com/questions/12524623/what-are-the-practical-differences-when-working-with-colors-in-a-linear-vs-a-no
	// Conversion to linear RGB.
	color_rgb = color_rgb / 255.0;
	color_rgb.x_ = sRGB_to_linearRGB_channel(color_rgb.x_);
	color_rgb.y_ = sRGB_to_linearRGB_channel(color_rgb.y_);
	color_rgb.z_ = sRGB_to_linearRGB_channel(color_rgb.z_);
	color_rgb = color_rgb * 100.0;

	// CIE-RGB conversion matrix.
	//mat3r_t conversion_mat(0.49, 0.31, 0.20, 0.17697, 0.81240, 0.01063, 0.00, 0.01, 0.99);
	//vec3r_t color_xyz = (conversion_mat / 0.17697) * color_rgb;
	
	// http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
	// sRGB conversion matrix.
	mat3r_t conversion_mat(0.4124, 0.2126, 0.0193, 0.3576, 0.7152, 0.1192, 0.1805, 0.0722, 0.9505);
	vec3r_t color_xyz = conversion_mat * color_rgb;

	return color_xyz;
}



real_t reduction_hierarchical_clustering_mk5::
sRGB_to_linearRGB_channel(real_t color_value) const
{
	if(color_value <= 0.04045)
	{
		return color_value / 12.92;
	}
	else
	{
		return std::pow((color_value + 0.055) / 1.055, 2.4);
	}
}



vec3r_t reduction_hierarchical_clustering_mk5::
transform_RGB_to_LAB(const vec3b_t& color) const
{
	vec3r_t color_xyz = transform_RGB_to_XYZ(color);
	vec3r_t reference_white(95.047, 100.000, 108.883);

	real_t var_x = LAB_helper(color_xyz.x_ / reference_white.x_);
	real_t var_y = LAB_helper(color_xyz.y_ / reference_white.y_);
	real_t var_z = LAB_helper(color_xyz.z_ / reference_white.z_);

	real_t L = 116.0 * var_y - 16.0;
	real_t a = 500.0 * (var_x - var_y);
	real_t b = 200.0 * (var_y - var_z);

	return vec3r_t(L, a, b);
}



real_t reduction_hierarchical_clustering_mk5::
LAB_helper(const real_t& t) const
{
	real_t result;

	if (t > std::pow(6.0/29.0, 3.0))
	{
		result = std::pow(t, 1.0/3.0);
	}
	else
	{
		result = 1.0/3.0 * std::pow(6.0/29.0, 2.0) * t + 4.0 / 29.0;
	}

	return result;
}



vec3r_t reduction_hierarchical_clustering_mk5::
transform_color(const vec3b_t& color) const
{
	vec3r_t color_transformed(color.x_, color.y_, color.z_);
	
	if (color_space_mode_ == 1)
	{
		color_transformed = transform_RGB_to_XYZ(color);
	}
	else if (color_space_mode_ == 2)
	{
		color_transformed = transform_RGB_to_LAB(color);
	}

	return color_transformed;
}



void reduction_hierarchical_clustering_mk5::
jacobi_rotation(const mat3d_t& _matrix, double* eigenvalues, double** eigenvectors) const
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



void reduction_hierarchical_clustering_mk5::
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
