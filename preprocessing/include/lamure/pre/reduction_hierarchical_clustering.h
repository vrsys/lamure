// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_REDUCTION_HIERARCHICAL_CLUSTERING_H_
#define PRE_REDUCTION_HIERARCHICAL_CLUSTERING_H_

#include <lamure/pre/reduction_strategy.h>
#include <lamure/pre/surfel.h>
#include <lamure/pre/bvh.h>

namespace lamure {
namespace pre {

class reduction_hierarchical_clustering : public reduction_strategy
{
public:

	explicit reduction_hierarchical_clustering();

    surfel_mem_array create_lod(real& reduction_error,
                                const std::vector<surfel_mem_array*>& input,
                                const uint32_t surfels_per_node) const override;

private:

	std::vector<std::vector<surfel*>> split_point_cloud(const std::vector<surfel*>& input_surfels, const uint32_t& max_cluster_size, const real& max_variation, const uint32_t& max_clusters) const;

	real calculate_variation(const scm::math::mat3d& covariance_matrix) const;

	scm::math::mat3d calculate_covariance_matrix(const std::vector<surfel*>& surfels_to_sample) const;

	vec3r calculate_centroid(const std::vector<surfel*>& surfels_to_sample) const;

	surfel create_surfel_from_cluster(const std::vector<surfel*>& surfels_to_sample) const;

	void jacobi_rotation(const scm::math::mat3d& _matrix, double* eigenvalues, double** eigenvectors) const;

	void eigsrt_jacobi(int dim, double* eigenvalues, double** eigenvectors) const;
};

} // namespace pre
} // namespace lamure

#endif // PRE_REDUCTION_HIERARCHICAL_CLUSTERING_H_