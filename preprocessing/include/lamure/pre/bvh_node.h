// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_BVH_NODE_H_
#define PRE_BVH_NODE_H_

#include <lamure/pre/platform.h>
#include <lamure/types.h>
#include <lamure/bounding_box.h>

#include <lamure/pre/surfel_mem_array.h>
#include <lamure/pre/surfel_disk_array.h>

#include <typeinfo>
#include <iostream>

namespace lamure {
namespace pre
{

class PREPROCESSING_DLL BvhNode /*final*/
{
public:
    explicit            BvhNode()
                            : node_id_(0),
                              depth_(0),
                              bounding_box_(vec3r(0.0), vec3r(0.0)),
                              reduction_error_(0.0),
                              avg_surfel_radius_(0.0),
                              centroid_(vec3r(0.0)),
                              visibility_(NodeVisibility::NodeVisible) {}

    explicit            BvhNode(const NodeIdType id,
                                const uint32_t depth,
                                const BoundingBox& bounding_box)
                            : node_id_(id),
                              depth_(depth),
                              bounding_box_(bounding_box),
                              reduction_error_(0.0),
                              avg_surfel_radius_(0.0),
                              centroid_(vec3r(0.0)),
                              visibility_(NodeVisibility::NodeVisible) {}

    explicit            BvhNode(const NodeIdType id,
                                const uint32_t depth,
                                const BoundingBox& bounding_box,
                                const SurfelMemArray& array);

    explicit            BvhNode(const NodeIdType id,
                                const uint32_t depth,
                                const BoundingBox& bounding_box,
                                const SurfelDiskArray& array);

                        ~BvhNode();

    enum NodeVisibility {
       NodeVisible   = 0,
       NodeInvisible = 1
    };

    const NodeIdType    node_id() const { return node_id_; }

    const BoundingBox&  bounding_box() const { return bounding_box_; }
    BoundingBox&        bounding_box() { return bounding_box_; }

    void                set_bounding_box(const BoundingBox& value) { bounding_box_ = value; }

    const uint32_t      depth() const { return depth_; }

    const real          reduction_error() const { return reduction_error_; }
    void                set_reduction_error(const real value)
                            { reduction_error_ = value; }

    const real          avg_surfel_radius() const { return avg_surfel_radius_; }
    void                set_avg_surfel_radius(const real value)
                            { avg_surfel_radius_ = value; }

    const vec3r         centroid() const { return centroid_; }
    void                set_centroid(const vec3r& value)
                            { centroid_ = value; }

    const NodeVisibility visibility() const { return visibility_; }
    void                set_visibility(const NodeVisibility visibility)
                            { visibility_ = visibility; }

    SurfelMemArray&     mem_array() { return mem_array_; }
    const SurfelMemArray&
                        mem_array() const { return mem_array_; }

    SurfelDiskArray&    disk_array() { return disk_array_; }
    const SurfelDiskArray&
                        disk_array() const { return disk_array_; }

    const bool          IsIC() const { return !mem_array_.is_empty(); }

    const bool          IsOOC() const { return !disk_array_.is_empty(); }

    /**
     * Unbinds any surfel data from the node.
     *
     * The function makes node's surfel arrays empty. If the node is in-core
     * initialized and no other node shares SurfelMemArray, then the underlying
     * SurfelVector is automatically destroyed.
     */
    void                Reset();

    /**
     * Unbinds any surfel data from the node and makes the node in-core by 
     * binding new SurfelMemArray.
     *
     * \param[in] array  An instance of SurfelMemArray.
     */
    void                Reset(const SurfelMemArray& array);

    /**
     * Unbinds any surfel data from the node and makes the node out-of-core by 
     * binding new SurfelDiskArray.
     *
     * \param[in] array  An instance of SurfelDiskArray.
     */
    void                Reset(const SurfelDiskArray& array);

    /**
     * Loads surfel data from the disk (if the node is in out-of-core mode).
     *
     * The function allocates new mem_array_ and loads data from disk_array_.
     */
    void                LoadFromDisk();

    /**
     * Activates out-of-core mode and saves surfel data to disk.
     *
     * The node should be in in-core mode. After calling this function the node
     * goes to out-of-core mode and saves surfels using provided ShareFile.
     * If dealloc_mem_array is true, the node leaves the in-core mode.
     *
     * \param[in] file            The SharedFile that will be used for
     *                            out-of-core processing.
     * \param[in] offset_in_file  The offset in the provided file.
     * \param[in] dealloc_mem_array  If true, the mem_data_will be reset.
     */
    void                FlushToDisk(const SharedFile& file,
                                    const size_t offset_in_file,
                                    const bool dealloc_mem_array);

    /**
     * Saves surfel data to disk.
     *
     * The node should be in both in-core and out-of-core mode.
     * If dealloc_mem_array is true, the node leaves the in-core mode.
     *
     * \param[in] dealloc_mem_array  If true, the mem_data_ will be reset.
     */
    void                FlushToDisk(const bool dealloc_mem_array);

private:

    NodeIdType          node_id_;
    uint32_t            depth_;
    BoundingBox         bounding_box_;
    real                reduction_error_;
    real                avg_surfel_radius_;
    vec3r               centroid_;
    NodeVisibility      visibility_;

    SurfelMemArray      mem_array_;
    SurfelDiskArray     disk_array_;
};

} } // namespace lamure

#endif // PRE_BVH_NODE_H_

