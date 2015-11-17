// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_REDUCTION_NORMAL_DEVIATON_CLUSTERING_H_
#define PRE_REDUCTION_NORMAL_DEVIATON_CLUSTERING_H_

#include <lamure/pre/reduction_strategy.h>
#include <lamure/pre/logger.h>

#include <lamure/bounding_box.h>
#include <vector>
#include <list>

namespace lamure {
namespace pre{

class ReductionNormalDeviationClustering: public ReductionStrategy
{
public:

    explicit            ReductionNormalDeviationClustering() {}

    SurfelMemArray      CreateLod(real& reduction_error,
                                  const std::vector<SurfelMemArray*>& input,
                                  const uint32_t surfels_per_node) const override;

private:

    using value_index_pair = std::pair<real, uint16_t>;

    struct surfel_cluster_with_error {
        std::list<Surfel>* cluster;
        float merge_treshold;
    };

    struct OrderBySize {
        bool operator() (const surfel_cluster_with_error& left, 
                         const surfel_cluster_with_error& right) {
            return left.cluster->size() < right.cluster->size();
        }
    };

    static Surfel CreateRepresentative(const std::vector<Surfel>& input);
    
    std::pair<vec3ui, vec3b> compute_grid_dimensions(const std::vector<SurfelMemArray*>& input,
                                                     const BoundingBox& bounding_box,
                                                     const uint32_t surfels_per_node) const;

    static bool Comp (const value_index_pair& l, const value_index_pair& r) {
        return l.first < r.first;
    }

    static bool CompRadius(const Surfel& left, const Surfel& right) {
        return left.radius() < right.radius();
    }
};

} // namespace pre
} // namespace lamure

#endif // PRE_REDUCTION_NORMAL_DEVIATON_CLUSTERING_H_
