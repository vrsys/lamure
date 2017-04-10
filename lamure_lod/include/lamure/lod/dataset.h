// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef LAMURE_LOD_DATASET_H_
#define LAMURE_LOD_DATASET_H_

#include <lamure/types.h>
#include <lamure/platform_lod.h>
#include <lamure/lod/bvh.h>
#include <lamure/lod/config.h>
#include <lamure/math/bounding_box.h>

#include <vector>
#include <fstream>
#include <sstream>

namespace lamure {
namespace lod {

class model_database;

class LOD_DLL dataset {

public:

    struct serialized_surfel {
      float x, y, z;
      uint8_t r, g, b, fake;
      float size;
      float nx, ny, nz;
    };
    struct serialized_triangle {
      float va_x_, va_y_, va_z_;
      float n0_x_, n0_y_, n0_z_;
      float c0_x_, c0_y_;
      float vb_x_, vb_y_, vb_z_;
      float n1_x_, n1_y_, n1_z_;
      float c1_x_, c1_y_;
      float vc_x_, vc_y_, vc_z_;
      float n2_x_, n2_y_, n2_z_;
      float c2_x_, c2_y_;
    };


                        dataset() {};
                        dataset(const std::string& filename);
    virtual             ~dataset();

    const model_t       model_id() const { return model_id_; };
    const lamure::math::bounding_box_t& aabb() const { return aabb_; };
    const bool          is_loaded() const { return is_loaded_; };
    const bvh*          get_bvh() const { return bvh_; };
    
    void                set_transform(const mat4d_t& transform) { transform_ = transform; };
    const mat4d_t       transform() const { return transform_; };

protected:
    void                load(const std::string& filename);

    friend class        model_database;
    model_t             model_id_;

private:
    lamure::math::bounding_box_t aabb_;
    bool                is_loaded_;
    bvh*                bvh_;

    mat4d_t             transform_;

};


} } // namespace lamure


#endif // LAMURE_LOD_DATASET_H_
