// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef REN_LOD_POINT_CLOUD_H_
#define REN_LOD_POINT_CLOUD_H_

#include <vector>
#include <fstream>
#include <sstream>

#include <lamure/utils.h>
#include <lamure/types.h>
#include <lamure/ren/platform.h>
#include <lamure/ren/bvh.h>
#include <lamure/ren/config.h>
#include <scm/gl_core/primitives/box.h>

namespace lamure {
namespace ren {

class Modeldatabase;

class RENDERING_DLL lod_point_cloud
{

public:

    struct serialized_surfel
    {
        float x, y, z;
        uint8_t r, g, b, fake;
        float size;
        float nx, ny, nz;
    };

                        lod_point_cloud() {};
                        lod_point_cloud(const std::string& filename);
    virtual             ~lod_point_cloud();

    const model_t       model_id() const { return model_id_; };
    const scm::gl::boxf& aabb() const { return aabb_; };
    const bool          is_loaded() const { return is_loaded_; };
    const bvh*          get_bvh() const { return bvh_; };

    void                set_transform(const scm::math::mat4f& transform) { transform_ = transform; };
    const scm::math::mat4f transform() const { return transform_; };

protected:
    void                load(const std::string& filename);

    friend class        Modeldatabase;
    model_t             model_id_;

private:
    scm::gl::boxf       aabb_;
    bool                is_loaded_;
    bvh*                bvh_;

    scm::math::mat4f    transform_;

};


} } // namespace lamure


#endif // REN_LOD_POINT_CLOUD_H_
