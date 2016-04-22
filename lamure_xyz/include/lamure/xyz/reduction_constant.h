// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_REDUCTION_CONSTANT_H_
#define PRE_REDUCTION_CONSTANT_H_

#include <lamure/xyz/reduction_strategy.h>

#include <lamure/math/bounding_box.h>
#include <vector>
#include <list>

namespace lamure {
namespace xyz {

class XYZ_DLL reduction_constant: public reduction_strategy
{
public:

    explicit            reduction_constant() { }

    surfel_mem_array      create_lod(real_t& reduction_error,
                                  const std::vector<surfel_mem_array*>& input,
                                  const uint32_t surfels_per_node,
                                  const bvh& tree,
                                  const size_t start_node_id) const override;

private:

    using value_index_pair = std::pair<real_t, uint16_t>;

    struct surfel_cluster_with_error {
        std::list<surfel>* cluster;
        float merge_treshold;
    };

    struct order_by_size {
        bool operator() (const surfel_cluster_with_error& left, 
                         const surfel_cluster_with_error& right) {
            return left.cluster->size() < right.cluster->size();
        }
    };

    static surfel create_representative(const std::vector<surfel>& input);
    
    static std::pair<vec3ui_t, vec3b_t> compute_grid_dimensions(const std::vector<surfel_mem_array*>& input,
                                                 const math::bounding_box_t& bounding_box,
                                                 const uint32_t surfels_per_node);

    static bool comp (const value_index_pair& l, const value_index_pair& r) {
        return l.first < r.first;
    }

    static bool compare_radius(const surfel& left, const surfel& right) {
        return left.radius() < right.radius();
    }
};

} // namespace xyz
} // namespace lamure

#endif // PRE_REDUCTION_CONSTANT_H_
