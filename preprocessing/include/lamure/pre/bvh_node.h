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
#include <lamure/math/bounding_box.h>
#include <lamure/assert.h>
#include <lamure/pre/surfel_mem_array.h>
#include <lamure/pre/surfel_disk_array.h>
#include <lamure/pre/node_statistics.h>

#include <typeinfo>
#include <iostream>

namespace lamure {
namespace pre
{

class PREPROCESSING_DLL bvh_node
{
public:
    explicit            bvh_node()
                            : node_id_(0),
                              depth_(0),
                              bounding_box_(vec3r_t(0.0), vec3r_t(0.0)),
                              reduction_error_(0.0),
                              avg_surfel_radius_(0.0),
                              centroid_(vec3r_t(0.0)),
                              visibility_(node_visibility::node_visible) {}

    explicit            bvh_node(const node_id_t id,
                                const uint32_t depth,
                                const math::bounding_box_t& bounding_box)
                            : node_id_(id),
                              depth_(depth),
                              bounding_box_(bounding_box),
                              reduction_error_(0.0),
                              avg_surfel_radius_(0.0),
                              centroid_(vec3r_t(0.0)),
                              visibility_(node_visibility::node_visible) {}

    explicit            bvh_node(const node_id_t id,
                                const uint32_t depth,
                                const math::bounding_box_t& bounding_box,
                                const surfel_mem_array& array);

    explicit            bvh_node(const node_id_t id,
                                const uint32_t depth,
                                const math::bounding_box_t& bounding_box,
                                const surfel_disk_array& array);

                        ~bvh_node();

    enum node_visibility {
       node_visible   = 0,
       node_invisible = 1
    };

    const node_id_t     node_id() const { return node_id_; }

    const math::bounding_box_t& get_bounding_box() const { return bounding_box_; }
    math::bounding_box_t& get_bounding_box() { return bounding_box_; }

    void                set_bounding_box(const math::bounding_box_t& value) { bounding_box_ = value; }

    const uint32_t      depth() const { return depth_; }

    const real_t          reduction_error() const { return reduction_error_; }
    void                set_reduction_error(const real_t value)
                            { reduction_error_ = value; }

    const real_t          avg_surfel_radius() const { return avg_surfel_radius_; }
    void                set_avg_surfel_radius(const real_t value)
                            { avg_surfel_radius_ = value; }

    const vec3r_t         centroid() const { return centroid_; }
    void                set_centroid(const vec3r_t& value)
                            { centroid_ = value; }

    const node_visibility visibility() const { return visibility_; }
    void                set_visibility(const node_visibility visibility)
                            { visibility_ = visibility; }

    void calculate_statistics();

    node_statistics&      node_stats() { return node_stats_;}

    surfel_mem_array&     mem_array() { return mem_array_; }
    const surfel_mem_array&
                        mem_array() const { return mem_array_; }

    surfel_disk_array&    disk_array() { return disk_array_; }
    const surfel_disk_array&
                        disk_array() const { return disk_array_; }

    const bool          is_in_core() const { return !mem_array_.is_empty(); }

    const bool          is_out_of_core() const { return !disk_array_.is_empty(); }

    /**
     * Unbinds any surfel data from the node.
     *
     * The function makes node's surfel arrays empty. If the node is in-core
     * initialized and no other node shares surfel_mem_array, then the underlying
     * surfel_vector is automatically destroyed.
     */
    void                reset();

    /**
     * Unbinds any surfel data from the node and makes the node in-core by 
     * binding new surfel_mem_array.
     *
     * \param[in] array  An instance of surfel_mem_array.
     */
    void                reset(const surfel_mem_array& array);

    /**
     * Unbinds any surfel data from the node and makes the node out-of-core by 
     * binding new surfel_disk_array.
     *
     * \param[in] array  An instance of surfel_disk_array.
     */
    void                reset(const surfel_disk_array& array);

    /**
     * loads surfel data from the disk (if the node is in out-of-core mode).
     *
     * The function allocates new mem_array_ and loads data from disk_array_.
     */
    void                load_from_disk();

    /**
     * Activates out-of-core mode and saves surfel data to disk.
     *
     * The node should be in in-core mode. After calling this function the node
     * goes to out-of-core mode and saves surfels using provided Sharefile.
     * If dealloc_mem_array is true, the node leaves the in-core mode.
     *
     * \param[in] file            The shared_file that will be used for
     *                            out-of-core processing.
     * \param[in] offset_in_file  The offset in the provided file.
     * \param[in] dealloc_mem_array  If true, the mem_data_will be reset.
     */
    void                flush_to_disk(const shared_file& file,
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
    void                flush_to_disk(const bool dealloc_mem_array);

private:

    node_id_t            node_id_;
    uint32_t             depth_;
    math::bounding_box_t bounding_box_;
    real_t               reduction_error_;
    real_t               avg_surfel_radius_;
    vec3r_t              centroid_;
    node_visibility      visibility_;

    surfel_mem_array     mem_array_;
    surfel_disk_array    disk_array_;

    node_statistics      node_stats_;
};

} } // namespace lamure

#endif // PRE_BVH_NODE_H_

