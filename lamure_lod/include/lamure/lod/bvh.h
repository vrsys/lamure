// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef LAMURE_LOD_BVH_H_
#define LAMURE_LOD_BVH_H_

#include <lamure/platform_lod.h>
#include <lamure/types.h>
#include <lamure/math/bounding_box.h>
#include <lamure/assert.h>

#include <string>
#include <fstream>
#include <cmath>
#include <iostream>
#include <vector>


namespace lamure {
namespace lod {

class LOD_DLL bvh
{


public:
    enum primitive_type {
       POINTCLOUD = 0,
       TRIMESH = 1
    };
    enum node_visibility {
       NODE_VISIBLE = 0,
       NODE_INVISIBLE = 1
    };

                        bvh();
                        bvh(const std::string& filename);
    virtual             ~bvh() {}

    const node_t        get_child_id(const node_t node_id, const node_t child_index) const;
    const node_t        get_parent_id(const node_t node_id) const;
    const node_t        get_first_node_id_of_depth(uint32_t depth) const;
    const uint32_t      get_length_of_depth(uint32_t depth) const;
    const uint32_t      get_depth_of_node(const node_t node_id) const;

    const std::string   get_filename() const { return filename_; }
    const uint32_t      get_num_nodes() const { return num_nodes_; }
    const uint32_t      get_fan_factor() const { return fan_factor_; }
    const uint32_t      get_depth() const { return depth_; }
    const uint32_t      get_primitives_per_node() const { return primitives_per_node_; }
    const uint32_t      get_size_of_primitive() const { return size_of_primitive_; }
    const vec3r_t       get_translation() const { return translation_; }
    const std::vector<lamure::math::bounding_box_t>& get_bounding_boxes() const { return bounding_boxes_; }
    const std::vector<vec3r_t>& get_centroids() const { return centroids_; };
    const lamure::math::bounding_box_t& get_bounding_box(const node_t node_id) const; 
    const vec3r_t&      get_centroid(const node_t node_id) const;
    const float         get_avg_primitive_extent(const node_t node_id) const;
    const node_visibility get_visibility(const node_t node_id) const;
    const primitive_type get_primitive() const { return primitive_; }
    
    void                set_num_nodes(const uint32_t num_nodes) { num_nodes_ = num_nodes; }
    void                set_fan_factor(const uint32_t fan_factor) { fan_factor_ = fan_factor; }
    void                set_depth(const uint32_t depth) { depth_ = depth; }
    void                set_primitives_per_node(const uint32_t primitives_per_node) { primitives_per_node_ = primitives_per_node; }
    void                set_size_of_primitive(const uint32_t size_of_primitive) { size_of_primitive_ = size_of_primitive; }
    void                set_translation(const vec3r_t& translation) { translation_ = translation; }
    void                set_bounding_box(const node_t node_id, const lamure::math::bounding_box_t& bounding_box);
    void                set_centroid(const node_t node_id, const vec3r_t& centroid);
    void                set_avg_primitive_extent(const node_t node_id, const float radius);
    void                set_visibility(const node_t node_id, const node_visibility visibility);
    void                set_primitive(const primitive_type primitive) { primitive_ = primitive; };

    void                write_bvh_file(const std::string& filename);

protected:

    void                load_bvh_file(const std::string& filename);

private:

    uint32_t            num_nodes_;
    uint32_t            fan_factor_;
    uint32_t            depth_;
    uint32_t            primitives_per_node_;
    uint32_t            size_of_primitive_;

    std::vector<lamure::math::bounding_box_t> bounding_boxes_;
    std::vector<vec3r_t> centroids_;
    std::vector<node_visibility> visibility_;

    std::vector<float>  avg_primitive_extent_;
    std::string         filename_;

    vec3r_t             translation_;
   
    primitive_type      primitive_;

};


} } // namespace lamure


#endif // LOD_BVH_H_

