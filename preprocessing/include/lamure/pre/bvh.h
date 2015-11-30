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

class NormalComputationStrategy;
class RadiusComputationStrategy;

class PREPROCESSING_DLL Bvh
{
public:

    enum class State {
        Null           = 0, // null tree
        Empty          = 1, // initialized, but empty tree
        AfterDownsweep = 2, // after downsweep
        AfterUpsweep   = 3, // after upsweep
        Serialized     = 4  // serialized surfel data
    };

    explicit            Bvh(const size_t memory_limit,  // in bytes
                                const size_t buffer_size,   // in bytes
                                const RepRadiusAlgorithm rep_radius_algo = RepRadiusAlgorithm::GeometricMean)
        : memory_limit_(memory_limit),
          buffer_size_(buffer_size),
          rep_radius_algo_(rep_radius_algo) {}

    virtual             ~Bvh() {}

                        Bvh(const Bvh& other) = delete;
    Bvh&            operator=(const Bvh& other) = delete;

    void                InitTree(const std::string& surfels_input_file,
                                 const uint32_t max_fan_factor,
                                 const size_t desired_surfels_per_node,
                                 const boost::filesystem::path& base_path);

    bool                LoadTree(const std::string& kdn_input_file);

    State               state() const { return state_; }
    uint8_t             fan_factor() const { return fan_factor_; }
    uint32_t            depth() const { return depth_; }
    size_t              max_surfels_per_node() const { return max_surfels_per_node_; }
    vec3r               translation() const { return translation_; }

    boost::filesystem::path base_path() const { return base_path_; }

    const std::vector<BvhNode>& nodes() const { return nodes_; }
    std::vector<BvhNode>& nodes() { return nodes_; }

    // helper funtions
    uint32_t            GetChildId(const uint32_t node_id, const uint32_t child_index) const;
    uint32_t            GetParentId(const uint32_t node_id) const;

    /**
     * Get id for the first node at given depth and total number of nodes at this depth.
     *
     * \param[in] depth           Tree layer for the range
     * \return                    Pair that contains first node id and number of nodes
     */
    std::pair<NodeIdType, NodeIdType> GetNodeRanges(const uint32_t depth) const;
    void                PrintTreeProperties() const;
    const NodeIdType    first_leaf() const { return first_leaf_; }

    // processing functions
    void                Downsweep(bool adjust_translation,
                                  const std::string& surfels_input_file,
                                  bool bin_all_file_extension = false);
    void                ComputeNormalsAndRadii(const uint16_t number_of_neighbours);

    void                compute_normal_and_radius(const size_t node_id,
                                                  const size_t surfel_id,
                                                  const NormalComputationStrategy&  normal_computation_strategy,
                                                  const RadiusComputationStrategy&  radius_computation_strategy);

    void                Upsweep(const ReductionStrategy& reduction_strategy);


    void                SerializeTreeToFile(const std::string& output_file,
                                            bool write_intermediate_data);

    void                SerializeSurfelsToFile(const std::string& output_file,
                                               const size_t buffer_size) const;

    /* Resets all nodes and deletes temp files
     */
    void                ResetNodes();

    static std::string  StateToString(State state);

protected:
    friend class BvhStream;
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
    void                set_nodes(const std::vector<BvhNode>& nodes) { nodes_ = nodes; };
    void                set_first_leaf(const NodeIdType first_leaf) { first_leaf_ = first_leaf; };
    void                set_state(const State state) { state_ = state; };

private:
    State               state_ = State::Null;

    std::vector<BvhNode>
                        nodes_;
    uint8_t             fan_factor_ = 0;

    uint32_t            depth_ = 0; ///< number of the last tree layer

    size_t              max_surfels_per_node_ = 0;

    NodeIdType          first_leaf_;

    //std::string         working_directory_;
    //std::string         basename_;
    boost::filesystem::path base_path_;

    size_t              memory_limit_;
    size_t              buffer_size_;
    RepRadiusAlgorithm  rep_radius_algo_;

    vec3r               translation_ = vec3r(0.0); ///< translation of surfels

    void                DownsweepSubtreeInCore(
                            const BvhNode& node,
                            size_t& disk_leaf_destination,
                            uint32_t& processed_nodes,
                            uint8_t& percent_processed,
                            SharedFile leaf_level_access);

    std::vector<std::pair<Surfel, real>>
                        GetNearestNeighbours(
                            const size_t node_id,
                            const size_t surfel_id,
                            const uint32_t num_neighbours) const;

    void                GetDescendantLeaves(
                            const size_t node,
                            std::vector<size_t>& result,
                            const size_t first_leaf,
                            const std::unordered_set<size_t>& excluded_leaves) const;
    void                GetDescendantNodes(
                            const size_t node,
                            std::vector<size_t>& result,
                            const size_t desired_depth,
                            const std::unordered_set<size_t>& excluded_nodes) const;

    void                UpsweepR(
                            BvhNode& node,
                            const ReductionStrategy& reduction_strategy,
                            std::vector<SharedFile>& level_temp_files,
                            std::atomic_uint& ctr);

};

using SharedBvh = std::shared_ptr<Bvh>;

} // namespace pre
} // namespace lamure

#endif // PRE_BVH_H_
