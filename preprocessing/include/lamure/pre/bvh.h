// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_BVH_H_
#define PRE_BVH_H_

#include <lamure/pre/platform.h>
#include <lamure/pre/common.h>
#include <lamure/pre/io/file.h>
#include <lamure/pre/bvh_node.h>
#include <lamure/pre/node_serializer.h>
#include <lamure/pre/reduction_strategy.h>
#include <lamure/pre/normal_computation_strategy.h>
#include <lamure/pre/radius_computation_strategy.h>
#include <lamure/pre/logger.h>

#include <boost/filesystem.hpp>
#include <unordered_set>
#include <atomic>

namespace lamure {
namespace pre {

class normal_computation_strategy;
class radius_computation_strategy;

class PREPROCESSING_DLL bvh
{
public:

    enum class state_type {
        null            = 0, // null tree
        empty           = 1, // initialized, but empty tree
        after_downsweep = 2, // after downsweep
        after_upsweep   = 3, // after upsweep
        serialized      = 4  // serialized surfel data
    };

    explicit            bvh(const size_t memory_limit,  // in bytes
                            const size_t buffer_size,   // in bytes
                            const rep_radius_algorithm rep_radius_algo = rep_radius_algorithm::geometric_mean)
        : memory_limit_(memory_limit),
          buffer_size_(buffer_size),
          rep_radius_algo_(rep_radius_algo) {}

    virtual             ~bvh() {}

                        bvh(const bvh& other) = delete;
    bvh&                operator=(const bvh& other) = delete;

    void                init_tree(const std::string& surfels_input_file,
                                 const uint32_t max_fan_factor,
                                 const size_t desired_surfels_per_node,
                                 const boost::filesystem::path& base_path);

    bool                load_tree(const std::string& kdn_input_file);

    state_type               state() const { return state_; }
    uint8_t             fan_factor() const { return fan_factor_; }
    uint32_t            depth() const { return depth_; }
    size_t              max_surfels_per_node() const { return max_surfels_per_node_; }
    vec3r               translation() const { return translation_; }

    boost::filesystem::path base_path() const { return base_path_; }

    const std::vector<bvh_node>& nodes() const { return nodes_; }
    std::vector<bvh_node>& nodes() { return nodes_; }

    // helper funtions
    uint32_t            get_child_id(const uint32_t node_id, const uint32_t child_index) const;
    uint32_t            get_parent_id(const uint32_t node_id) const;
    const node_t        get_first_node_id_of_depth(uint32_t depth) const;
    const uint32_t      get_length_of_depth(uint32_t depth) const;

    /**
     * Get id for the first node at given depth and total number of nodes at this depth.
     *
     * \param[in] depth           Tree layer for the range
     * \return                    Pair that contains first node id and number of nodes
     */
    std::pair<node_id_type, node_id_type> get_node_ranges(const uint32_t depth) const;

    std::vector<std::pair<surfel, real>>
                        get_nearest_neighbours(
                            const size_t node_id,
                            const size_t surfel_id,
                            const uint32_t num_neighbours) const;

    void                print_tree_properties() const;
    const node_id_type  first_leaf() const { return first_leaf_; }

    // processing functions
    void                downsweep(bool adjust_translation,
                                  const std::string& surfels_input_file,
                                  bool bin_all_file_extension = false);

    void                compute_normals_and_radii(const uint16_t number_of_neighbours);

    void                compute_normal_and_radius(const normal_computation_strategy&  normal_computation_strategy,
                                                  const radius_computation_strategy&  radius_computation_strategy);

    void                upsweep(const reduction_strategy& strategy);
    void                upsweep_new(const reduction_strategy& reduction_strategy, 
                                    const normal_computation_strategy& normal_comp_strategy, 
                                    const radius_computation_strategy& radius_comp_strategy);

    void                serialize_tree_to_file(const std::string& output_file,
                                            bool write_intermediate_data);

    void                serialize_surfels_to_file(const std::string& output_file,
                                               const size_t buffer_size) const;

    /* resets all nodes and deletes temp files
     */
    void                reset_nodes();

    static std::string  state_to_string(state_type state);

protected:
    friend class bvh_stream;
    void                set_depth(const uint32_t depth) { depth_ = depth; };
    void                set_fan_factor(const uint8_t fan_factor) { fan_factor_ = fan_factor; };
    void                set_max_surfels_per_node(const size_t max_surfels_per_node) {
                            max_surfels_per_node_ = max_surfels_per_node;
                        }
    void                set_translation(const vec3r& translation) { translation_ = translation; };
    //void                set_working_directory(const std::string& working_directory) { 
    //                        working_directory_ = working_directory;
    //                   }
    //void                set basename(const std::string& basename) { basename_ = basename; };
    void                set_base_path(const boost::filesystem::path& base_path) { base_path_ = base_path; };
    void                set_nodes(const std::vector<bvh_node>& nodes) { nodes_ = nodes; };
    void                set_first_leaf(const node_id_type first_leaf) { first_leaf_ = first_leaf; };
    void                set_state(const state_type state) { state_ = state; };

private:
    state_type          state_ = state_type::null;

    std::vector<bvh_node>
                        nodes_;
    uint8_t             fan_factor_ = 0;

    uint32_t            depth_ = 0; ///< number of the last tree layer

    size_t              max_surfels_per_node_ = 0;

    node_id_type          first_leaf_;

    //std::string         working_directory_;
    //std::string         basename_;
    boost::filesystem::path base_path_;

    size_t              memory_limit_;
    size_t              buffer_size_;
    rep_radius_algorithm  rep_radius_algo_;

    vec3r               translation_ = vec3r(0.0); ///< translation of surfels

    void                downsweep_subtree_in_core(
                            const bvh_node& node,
                            size_t& disk_leaf_destination,
                            uint32_t& processed_nodes,
                            uint8_t& percent_processed,
                            shared_file leaf_level_access);

    void                get_descendant_leaves(
                            const size_t node,
                            std::vector<size_t>& result,
                            const size_t first_leaf,
                            const std::unordered_set<size_t>& excluded_leaves) const;
    void                get_descendant_nodes(
                            const size_t node,
                            std::vector<size_t>& result,
                            const size_t desired_depth,
                            const std::unordered_set<size_t>& excluded_nodes) const;

    void                upsweep_r(
                            bvh_node& node,
                            const reduction_strategy& reduction_strategy,
                            std::vector<shared_file>& level_temp_files,
                            std::atomic_uint& ctr);

};

using bvh_ptr = std::shared_ptr<bvh>;

} // namespace pre
} // namespace lamure

#endif // PRE_BVH_H_