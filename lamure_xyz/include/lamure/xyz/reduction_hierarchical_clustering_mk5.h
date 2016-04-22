// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_REDUCTION_HIERARCHICAL_CLUSTERING_MK5_H_
#define PRE_REDUCTION_HIERARCHICAL_CLUSTERING_MK5_H_

#include <lamure/xyz/reduction_strategy.h>
#include <lamure/xyz/surfel.h>
#include <lamure/xyz/bvh.h>
#include <queue>

namespace lamure {
namespace xyz {

struct hierarchical_cluster_mk5
{
	std::vector<surfel*> surfels;
	
	vec3r_t centroid_pos;
	vec3r_t centroid_color;

	vec3f_t normal_pos;
	vec3f_t normal_color;
	
	real_t variation_pos;
	real_t variation_color;
};



struct cluster_comparator_mk5
{
  bool operator()(const hierarchical_cluster_mk5& lhs, const hierarchical_cluster_mk5& rhs) const
  {
    if(lhs.surfels.size() != rhs.surfels.size())
	{
		return lhs.surfels.size() < rhs.surfels.size();
	}
	else
	{
		real_t total_variation_left = lhs.variation_pos + lhs.variation_color;
		real_t total_variation_right = rhs.variation_pos + rhs.variation_color;
		return total_variation_left < total_variation_right;
	}
  }
};



class XYZ_DLL reduction_hierarchical_clustering_mk5 : public reduction_strategy
{
public:

	explicit reduction_hierarchical_clustering_mk5();

    surfel_mem_array create_lod(real_t& reduction_error,
    							const std::vector<surfel_mem_array*>& input,
                                const uint32_t surfels_per_node,
          						const bvh& tree,
          						const size_t start_node_id) const override;

private:

	std::vector<std::vector<surfel*>> split_point_cloud(const std::vector<surfel*>& input_surfels, 
														uint32_t max_cluster_size, 
														real_t max_variation_position,
														real_t max_variation_color, 
														const uint32_t& max_clusters) const;

	void split_cluster_by_position(const hierarchical_cluster_mk5& input_cluster,
								const uint32_t& max_cluster_size,
								const real_t& max_variation,
								std::priority_queue<hierarchical_cluster_mk5, std::vector<hierarchical_cluster_mk5>, cluster_comparator_mk5>& cluster_queue) const;

	hierarchical_cluster_mk5 calculate_cluster_data(const std::vector<surfel*>& input_surfels) const;

	real_t calculate_variation(const mat3d_t& covariance_matrix, vec3f_t& normal) const;

	mat3d_t calculate_covariance_matrix(const std::vector<surfel*>& surfels_to_sample, vec3r_t& centroid) const;

	mat3d_t calculate_covariance_matrix_color(const std::vector<surfel*>& surfels_to_sample, vec3r_t& centroid) const;

	vec3r_t calculate_centroid(const std::vector<surfel*>& surfels_to_sample) const;

	vec3r_t calculate_centroid_color(const std::vector<surfel*>& surfels_to_sample) const;

	surfel create_surfel_from_cluster(const std::vector<surfel*>& surfels_to_sample) const;

	real_t point_plane_distance(const vec3r_t& centroid, const vec3f_t& normal, const vec3r_t& point) const;

	vec3r_t transform_RGB_to_XYZ(const vec3b_t& color) const;

	real_t sRGB_to_linearRGB_channel(real_t color_value) const;

	vec3r_t transform_RGB_to_LAB(const vec3b_t& color) const;

	real_t LAB_helper(const real_t& t) const;

	vec3r_t transform_color(const vec3b_t& color) const;

	void jacobi_rotation(const mat3d_t& _matrix, double* eigenvalues, double** eigenvectors) const;

	void eigsrt_jacobi(int dim, double* eigenvalues, double** eigenvectors) const;

	int color_space_mode_;
};

} // namespace xyz
} // namespace lamure

#endif // PRE_REDUCTION_HIERARCHICAL_CLUSTERING_MK5_H_
