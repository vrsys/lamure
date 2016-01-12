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

#include <CGAL/Vector_3.h>
#include <CGAL/Cartesian_matrix.h>
#include <CGAL/Cartesian.h>
#include <CGAL/Linear_algebraCd.h>

namespace lamure {
namespace pre {

typedef CGAL::Cartesian<real>::Vector_3 Vector_3;
typedef CGAL::Linear_algebraCd<real> Linear_Algebra;
typedef Linear_Algebra::Matrix Matrix;

class reduction_hierarchical_clustering : public reduction_strategy
{
public:

	explicit reduction_hierarchical_clustering();

    surfel_mem_array create_lod(real& reduction_error,
                                const std::vector<surfel_mem_array*>& input,
                                const uint32_t surfels_per_node) const override;

private:

	std::vector<std::vector<surfel*>> split_point_cloud(const std::vector<surfel*>& input_surfels, const uint32_t& max_cluster_size) const;

	real calculate_variation(const Matrix& covariance_matrix) const;

	Matrix calculate_covariance_matrix(const std::vector<surfel*>& surfels_to_sample) const;

	Vector_3 calculate_centroid(const std::vector<surfel*>& surfels_to_sample) const;

	real maximum_variation_ = 0;
};

} // namespace pre
} // namespace lamure

#endif // PRE_REDUCTION_HIERARCHICAL_CLUSTERING_H_