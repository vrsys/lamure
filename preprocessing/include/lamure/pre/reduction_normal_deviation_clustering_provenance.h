// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_REDUCTION_NORMAL_DEVIATON_CLUSTERING_PROV_H_
#define PRE_REDUCTION_NORMAL_DEVIATON_CLUSTERING_PROV_H_

#include <lamure/pre/logger.h>
#include <lamure/pre/reduction_strategy.h>

#include "reduction_strategy_provenance.h"
#include <lamure/bounding_box.h>
#include <list>
#include <vector>

namespace lamure
{
namespace pre
{
class PREPROCESSING_DLL reduction_normal_deviation_clustering_provenance : public reduction_strategy_provenance
{
  public:
    explicit reduction_normal_deviation_clustering_provenance() {}

    surfel_mem_array create_lod(real &reduction_error, const std::vector<surfel_mem_array *> &input, std::vector<LoDMetaData> &deviations, const uint32_t surfels_per_node,
                                const bvh &tree, const size_t start_node_id) const override;

  private:
    using value_index_pair = std::pair<real, uint16_t>;

    struct surfel_cluster_with_error
    {
        std::list<surfel_ext> *cluster;
        float merge_treshold;
    };

    struct order_by_size
    {
        bool operator()(const surfel_cluster_with_error &left, const surfel_cluster_with_error &right) { return left.cluster->size() < right.cluster->size(); }
    };

    struct provenance_cluster
    {
        std::list<reduction_strategy_provenance::LoDMetaData> cluster;
    };

    struct pro_order_by_size
    {
        bool operator()(const provenance_cluster &left, const provenance_cluster &right) { return left.cluster.size() < right.cluster.size(); }
    };

    static surfel_ext create_representative(const std::vector<surfel_ext> &input);

    std::pair<vec3ui, vec3b> compute_grid_dimensions(const std::vector<surfel_mem_array *> &input, const bounding_box &bounding_box, const uint32_t surfels_per_node) const;

    static bool comp(const value_index_pair &l, const value_index_pair &r) { return l.first < r.first; }

    static bool compare_radius(const surfel_ext &left, const surfel_ext &right) { return left.surfel_.radius() < right.surfel_.radius(); }

    reduction_strategy_provenance::LoDMetaData calculate_deviations(surfel_ext& repr, const std::vector<surfel_ext> &input) const;
    std::list<reduction_strategy_provenance::LoDMetaData> generate_provenance_empties(std::list<surfel_ext> *& surfels) const;
};

} // namespace pre
} // namespace lamure

#endif // PRE_REDUCTION_NORMAL_DEVIATON_CLUSTERING_PROV_H_
