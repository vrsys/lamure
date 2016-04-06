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
#include <lamure/atomic_counter.h>

#include <boost/filesystem.hpp>
#include <unordered_set>
#include <atomic>

namespace lamure {
namespace pre {

class reduction_strategy;

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
    uint32_t            get_depth_of_node(const uint32_t node_id) const;
    uint32_t            get_child_id(const uint32_t node_id, const uint32_t child_index) const;
    uint32_t            get_parent_id(const uint32_t node_id) const;
    const node_id_type        get_first_node_id_of_depth(uint32_t depth) const;
    const uint32_t      get_length_of_depth(uint32_t depth) const;

    void                resample_based_on_overlap(surfel_mem_array const&  joined_input,
                                                  surfel_mem_array& output_mem_array,
                                                  std::vector<surfel_id_t> const& resample_candites) const;
    std::vector<surfel_id_t>
                       find_resample_candidates(surfel_mem_array const&  child_mem_array,
                                                const uint32_t node_idx) const;

    /**
     * Get id for the first node at given depth and total number of nodes at this depth.
     *
     * \param[in] depth           Tree layer for the range
     * \return                    Pair that contains first node id and number of nodes
     */
    std::pair<node_id_type, node_id_type> get_node_ranges(const uint32_t depth) const;

    std::vector<std::pair<surfel_id_t, real>>
                        get_nearest_neighbours(
                            const surfel_id_t target_surfel,
                            const uint32_t num_neighbours,
                            const bool do_local_search = false) const;

    std::vector<std::pair<surfel_id_t, real>>
                        get_nearest_neighbours_in_nodes(
                            const surfel_id_t target_surfel,
                            const std::vector<node_id_type>& target_nodes,
                            const uint32_t num_neighbours) const;

    std::vector<std::pair<surfel_id_t, real>>
                        get_natural_neighbours(
                            const surfel_id_t& target_surfel,
                            std::vector<std::pair<surfel_id_t, real>> const& nearest_neighbours) const;

    std::vector<std::pair<surfel, real> >
                        get_locally_natural_neighbours(std::vector<surfel> const& potential_neighbour_vec,
                                                       vec3r const& poi,
                                                       uint32_t num_nearest_neighbours) const;

    std::vector<std::pair<uint32_t, real>>
                        extract_approximate_natural_neighbours(vec3r const& target_surfel,
                        std::vector<vec3r> const& all_nearest_neighbours) const;


    void                print_tree_properties() const;
    const node_id_type  first_leaf() const { return first_leaf_; }

    // processing functions
    void                downsweep(bool adjust_translation,
                                  const std::string& surfels_input_file,
                                  bool bin_all_file_extension = false);

    void                compute_normals_and_radii(const uint16_t number_of_neighbours);

    void                compute_normal_and_radius(const bvh_node* source_node,
                                                  const normal_computation_strategy& normal_computation_strategy,
                                                  const radius_computation_strategy& radius_computation_strategy);

    void                upsweep(const reduction_strategy& reduction_strategy, 
                                const normal_computation_strategy& normal_comp_strategy, 
                                const radius_computation_strategy& radius_comp_strategy,
                                bool recompute_leaf_level = true,
                                bool resample = false);

    surfel_vector       remove_outliers_statistically(uint32_t num_outliers, uint16_t num_neighbours);

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

    void                spawn_create_lod_jobs(const uint32_t first_node_of_level, 
                                              const uint32_t last_node_of_level,
                                              const reduction_strategy& reduction_strgy,
                                              const bool resample);
    void                spawn_compute_attribute_jobs(const uint32_t first_node_of_level, 
                                                     const uint32_t last_node_of_level,
                                                     const normal_computation_strategy& normal_strategy, 
                                                     const radius_computation_strategy& radius_strategy,
                                                     const bool is_leaf_level);
    void                spawn_compute_bounding_boxes_downsweep_jobs(const uint32_t slice_left, 
                                                                    const uint32_t slice_right);
    void                spawn_compute_bounding_boxes_upsweep_jobs(const uint32_t first_node_of_level, 
                                                                  const uint32_t last_node_of_level,
                                                                  const int32_t level);
    void                spawn_split_node_jobs(size_t& slice_left,
                                              size_t& slice_right,
                                              size_t& new_slice_left,
                                              size_t& new_slice_right,
                                              const uint32_t level);
    
    void                thread_remove_outlier_jobs(const uint32_t start_marker,
                                                   const uint32_t end_marker,
                                                   const uint32_t num_outliers,
                                                   const uint16_t num_neighbours,
                                                   std::vector< std::pair<surfel_id_t, real> >&  intermediate_outliers_for_thread);
    void                thread_compute_attributes(const uint32_t start_marker,
                                                  const uint32_t end_marker,
                                                  const bool update_percentage,
                                                  const normal_computation_strategy& normal_strategy, 
                                                  const radius_computation_strategy& radius_strategy,
                                                  const bool is_leaf_level);
    void                thread_create_lod(const uint32_t start_marker,
                                          const uint32_t end_marker,
                                          const bool update_percentage,
                                          const reduction_strategy& reduction_strgy,
                                          const bool resample);
    void                thread_compute_bounding_boxes_downsweep(const uint32_t slice_left,
                                                                const uint32_t slice_right,
                                                                const bool update_percentage,
                                                                const uint32_t num_threads);
    void                thread_compute_bounding_boxes_upsweep(const uint32_t start_marker,
                                                              const uint32_t end_marker,
                                                              const bool update_percentage,
                                                              const int32_t level, 
                                                              const uint32_t num_threads);
    void                thread_split_node_jobs(      size_t& slice_left,
                                                     size_t& slice_right,
                                                     size_t& new_slice_left,
                                                     size_t& new_slice_right,
                                               const bool update_percentage,
                                               const int32_t level,
                                               const uint32_t num_threads);

private:
    atomic_counter<uint32_t> working_queue_head_counter_;

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
                            const node_id_type node,
                            std::vector<node_id_type>& result,
                            const node_id_type first_leaf,
                            const std::unordered_set<size_t>& excluded_leaves) const;
    void                get_descendant_nodes(
                            const node_id_type node,
                            std::vector<node_id_type>& result,
                            const node_id_type desired_depth,
                            const std::unordered_set<size_t>& excluded_nodes) const;
};

using bvh_ptr = std::shared_ptr<bvh>;

} // namespace pre
} // namespace lamure

#endif // PRE_BVH_H_