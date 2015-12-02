// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef REN_RAW_POINT_CLOUD_H_
#define REN_RAW_POINT_CLOUD_H_

#include <vector>
#include <fstream>
#include <sstream>

#include <lamure/ren/platform.h>
#include <lamure/utils.h>
#include <lamure/types.h>

#include <scm/gl_core/primitives/box.h>

namespace lamure {
namespace ren {

class RENDERING_DLL raw_point_cloud
{

public:

    struct serialized_surfel
    {
        float x, y, z;
        uint8_t r, g, b, fake;
        float size;
        float nx, ny, nz;
    };

                        raw_point_cloud(const model_t model_id);
                        raw_point_cloud(const raw_point_cloud&) = delete;
                        raw_point_cloud& operator=(const raw_point_cloud&) = delete;
    virtual             ~raw_point_cloud();


    const model_t       model_id() const { return model_id_; };
    const scm::gl::boxf& aabb() const { return aabb_; };
    const bool          is_loaded() const { return is_loaded_; };
    const size_t        num_surfels() const { return data_.size(); };

    const std::vector<serialized_surfel>& data() const { return data_; };

    const bool          load(const std::string& filename);

protected:

private:
    /* data */
    model_t             model_id_;
    scm::gl::boxf       aabb_;
    bool                is_loaded_;
    std::vector<serialized_surfel> data_;


};


} } // namespace lamure


#endif // REN_RAW_POINT_CLOUD_H_
