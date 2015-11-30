// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef REN_BVH_H_
#define REN_BVH_H_

#include <string>
#include <fstream>
#include <cmath>
#include <iostream>
#include <vector>

#include <lamure/ren/platform.h>
#include <lamure/bounding_box.h>
#include <lamure/types.h>

#include <scm/gl_core/primitives/box.h>

namespace lamure {
namespace ren {

class RENDERING_DLL bvh
{


public:
    enum node_visibility {
       NODE_VISIBLE = 0,
       NODE_INVISIBLE = 1
    };

                        bvh();
                        bvh(const std::string& filename);
    virtual             ~bvh() {}

    const node_t        get_child_id(const node_t node_id, const node_t child_index) const;
    const node_t        get_parent_id(const node_t node_id) const;
    const node_t        GetFirstNodeIdOfDepth(uint32_t depth) const;
    const uint32_t      GetLengthOfDepth(uint32_t depth) const;
    const uint32_t      GetDepthOfNode(const node_t node_id) const;

    const std::string   filename() const { return filename_; }
    
    const uint32_t      num_nodes() const { return num_nodes_; }
    void                set_num_nodes(const uint32_t num_nodes) { num_nodes_ = num_nodes; }

    const uint32_t      fan_factor() const { return fan_factor_; }
    void                set_fan_factor(const uint32_t fan_factor) { fan_factor_ = fan_factor; }

    const uint32_t      depth() const { return depth_; }
    void                set_depth(const uint32_t depth) { depth_ = depth; }

    const uint32_t      surfels_per_node() const { return surfels_per_node_; }
    void                set_surfels_per_node(const uint32_t surfels_per_node) { surfels_per_node_ = surfels_per_node; }

    const uint32_t      size_of_surfel() const { return size_of_surfel_; }
    void                set_size_of_surfel(const uint32_t size_of_surfel) { size_of_surfel_ = size_of_surfel; }

    const vec3f         translation() const { return translation_; }
    void                set_translation(const scm::math::vec3f& translation) { translation_ = translation; }

    const std::vector<scm::gl::boxf>& bounding_boxes() const { return bounding_boxes_; }
    const std::vector<vec3f>& centroids() const { return centroids_; };

    const scm::gl::boxf& Getbounding_box(const node_t node_id) const;
    void                Setbounding_box(const lamure::node_t node_id, const scm::gl::boxf& bounding_box);

    const scm::math::vec3f& GetCentroid(const node_t node_id) const;
    void                SetCentroid(const lamure::node_t node_id, const scm::math::vec3f& centroid);

    const float         GetAvgsurfelRadius(const node_t node_id) const;
    void                SetAvgsurfelRadius(const lamure::node_t node_id, const float radius);

    const node_visibility GetVisibility(const node_t node_id) const;
    void                SetVisibility(const node_t node_id, const node_visibility visibility);

    void                writebvhfile(const std::string& filename);

protected:

    void                loadbvhfile(const std::string& filename);

private:

    uint32_t            num_nodes_;
    uint32_t            fan_factor_;
    uint32_t            depth_;
    uint32_t            surfels_per_node_;
    uint32_t            size_of_surfel_;

    std::vector<scm::gl::boxf> bounding_boxes_;
    std::vector<vec3f>  centroids_;
    std::vector<node_visibility> visibility_;

    std::vector<float>  avg_surfel_radii_;
    std::string         filename_;

    vec3f               translation_;

};


} } // namespace lamure


#endif // REN_BVH_H_

