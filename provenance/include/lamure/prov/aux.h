// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PROV_AUX_H_
#define PROV_AUX_H_

#include <string>
#include <fstream>
#include <cmath>
#include <iostream>
#include <vector>

#include <lamure/types.h>

namespace lamure {
namespace prov {

class aux {
public:

    struct feature {
      uint32_t camera_id_;
      uint32_t using_count_;
      scm::math::vec2f coords_;
    };

    struct sparse_point {
      scm::math::vec3f pos_;
      uint8_t r_;
      uint8_t g_;
      uint8_t b_;
      uint8_t a_;
      std::vector<feature> features_;
    };

    struct view {
      uint32_t camera_id_;
      scm::math::vec3f position_;
      scm::math::mat4f transform_; //trans + rot
      float focal_length_;
      float distortion_;
      uint32_t image_width_;
      uint32_t image_height_;
      uint32_t tex_atlas_id_;
      std::string camera_name_;
      std::string image_file_;
    };
                        aux();
                        aux(const std::string& filename);
    virtual             ~aux() {}

    const std::string   get_filename() const { return filename_; }
    const uint32_t      get_num_views() const { return views_.size(); }
    const uint64_t      get_num_sparse_points() const { return sparse_points_.size(); }

    const view&         get_view(uint32_t id) const;
    const sparse_point& get_sparse_point(uint64_t id) const;
    
    void                add_view(const view& view);
    void                add_sparse_point(const sparse_point& point);

    void                write_aux_file(const std::string& filename);

protected:

    void                load_aux_file(const std::string& filename);

private:

    std::vector<view> views_;
    std::vector<sparse_point>  sparse_points_;
    std::string         filename_;

};


} } // namespace lamure


#endif // PROV_AUX_H_

