// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_REDUCTION_HIERARCHICAL_CLUSTERING_MK2_H_
#define PRE_REDUCTION_HIERARCHICAL_CLUSTERING_MK2_H_

#include <lamure/pre/reduction_strategy.h>
#include <lamure/pre/surfel.h>
#include <lamure/pre/bvh.h>

namespace lamure
{
namespace pre
{

struct hierarchical_cluster_mk2
{
    std::vector<surfel *> surfels;

    vec3r centroid_pos;
    vec3r centroid_normal;
    vec3r centroid_color;

    vec3f normal_pos;
    vec3f normal_normal;
    vec3f normal_color;

    real variation_pos;
    real variation_normal;
    real variation_color;
};

struct cluster_comparator_mk2
{
    bool operator()(const hierarchical_cluster_mk2 &lhs, const hierarchical_cluster_mk2 &rhs) const
    {
        if (lhs.surfels.size() != rhs.surfels.size()) {
            return lhs.surfels.size() < rhs.surfels.size();
        }
        else {
            real total_variation_left = lhs.variation_pos + lhs.variation_normal + lhs.variation_color;
            real total_variation_right = rhs.variation_pos + rhs.variation_normal + rhs.variation_color;
            return total_variation_left < total_variation_right;
        }
    }
};

class PREPROCESSING_DLL reduction_hierarchical_clustering_mk2: public reduction_strategy
{
public:

    explicit reduction_hierarchical_clustering_mk2();

    surfel_mem_array create_lod(real &reduction_error,
                                const std::vector<surfel_mem_array *> &input,
                                const uint32_t surfels_per_node,
                                const bvh &tree,
                                const size_t start_node_id) const override;

private:

    std::vector<std::vector<surfel *>> split_point_cloud(const std::vector<surfel *> &input_surfels, uint32_t max_cluster_size, real max_variation, const uint32_t &max_clusters) const;

    hierarchical_cluster_mk2 calculate_cluster_data(const std::vector<surfel *> &input_surfels) const;

    real calculate_variation(const scm::math::mat3d &covariance_matrix, vec3f &normal) const;

    scm::math::mat3d calculate_covariance_matrix(const std::vector<surfel *> &surfels_to_sample, vec3r &centroid) const;

    scm::math::mat3d calculate_covariance_matrix_normal(const std::vector<surfel *> &surfels_to_sample, vec3r &centroid) const;

    scm::math::mat3d calculate_covariance_matrix_color(const std::vector<surfel *> &surfels_to_sample, vec3r &centroid) const;

    vec3r calculate_centroid(const std::vector<surfel *> &surfels_to_sample) const;

    vec3r calculate_centroid_normal(const std::vector<surfel *> &surfels_to_sample) const;

    vec3r calculate_centroid_color(const std::vector<surfel *> &surfels_to_sample) const;

    surfel create_surfel_from_cluster(const std::vector<surfel *> &surfels_to_sample) const;

    real point_plane_distance(const vec3r &centroid, const vec3f &normal, const vec3r &point) const;

    void jacobi_rotation(const scm::math::mat3d &_matrix, double *eigenvalues, double **eigenvectors) const;

    void eigsrt_jacobi(int dim, double *eigenvalues, double **eigenvectors) const;
};

} // namespace pre
} // namespace lamure

#endif // PRE_REDUCTION_HIERARCHICAL_CLUSTERING_MK2_H_