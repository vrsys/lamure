//
// Created by moqe3167 on 29/05/18.
//

#ifndef LAMURE_VT_SYSTEM_H
#define LAMURE_VT_SYSTEM_H

//gl
//#include <GL/glew.h>
//#include <GLFW/glfw3.h>

//schism
#include <scm/core.h>
#include <scm/core/math.h>
#include <scm/core/io/tools.h>
#include <scm/core/pointer_types.h>
#include <scm/gl_core/gl_core_fwd.h>
#include <scm/gl_util/primitives/primitives_fwd.h>
#include <scm/core/platform/platform.h>
#include <scm/core/utilities/platform_warning_disable.h>
#include <scm/gl_util/primitives/quad.h>
#include <scm/gl_util/font/font_face.h>
#include <scm/gl_util/font/text.h>
#include <scm/gl_util/font/text_renderer.h>
#include <scm/core/time/accum_timer.h>
#include <scm/core/time/high_res_timer.h>
#include <scm/gl_util/data/imaging/texture_loader.h>

// vt
#include <lamure/vt/VTConfig.h>
#include <lamure/vt/ren/CutDatabase.h>
#include <lamure/vt/ren/CutUpdate.h>
#include <lamure/vt/pre/AtlasFile.h>


//boost
#include <boost/assign/list_of.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

//lamure
#include <lamure/types.h>
#include <lamure/ren/config.h>
#include <lamure/ren/model_database.h>
#include <lamure/ren/cut_database.h>
#include <lamure/ren/dataset.h>
#include <lamure/ren/policy.h>
#include <lamure/ren/controller.h>
#include <lamure/pvs/pvs_database.h>
#include <lamure/ren/ray.h>
#include <lamure/prov/aux.h>
#include <lamure/prov/octree.h>


#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <vector>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <algorithm>
#include <list>
#include "selection.h"

#include "resource.h"


class vt_system {
public:
    vt_system(const boost::shared_ptr<scm::gl::render_device> &device_,
              const boost::shared_ptr<scm::gl::render_context> &context_,
              const std::string &atlas_file_);

    float get_atlas_scale_factor();

    void set_shader_program(scm::gl::program_ptr shader_program);

    void render(lamure::ren::camera *camera, int32_t render_width, int32_t render_height, int32_t num_models,
                    selection selection);

    void set_image_resources(map<uint32_t, resource> resource);

private:
    void init_vt_system();
    void init_vt_database();
    void collect_vt_feedback();
    void apply_vt_cut_update();
    scm::gl::data_format get_tex_format();

    scm::shared_ptr<scm::gl::render_device> device_;
    scm::shared_ptr<scm::gl::render_context> context_;

    uint32_t texture_id_;
    uint16_t view_id_;
    uint16_t context_id_;
    uint64_t cut_id_;
    vt::CutUpdate *cut_update_;

    std::vector<scm::gl::texture_2d_ptr> index_texture_hierarchy_;
    scm::gl::texture_2d_ptr physical_texture_;

    scm::math::vec2ui physical_texture_size_;
    scm::math::vec2ui physical_texture_tile_size_;
    size_t size_feedback_;

    int32_t  *feedback_lod_cpu_buffer_;
    uint32_t *feedback_count_cpu_buffer_;

    scm::gl::buffer_ptr feedback_lod_storage_;
    scm::gl::buffer_ptr feedback_count_storage_;

    std::string atlas_file_;

    int toggle_visualization_;
    bool enable_hierarchy_;

    scm::gl::sampler_state_ptr vt_filter_linear_;
    scm::gl::sampler_state_ptr vt_filter_nearest_;
    scm::gl::program_ptr vis_vt_shader_;
    scm::gl::depth_stencil_state_ptr depth_state_less_;
    scm::gl::rasterizer_state_ptr no_backface_culling_rasterizer_state_;
    scm::gl::blend_state_ptr color_no_blending_state_;
    map<uint32_t, resource> image_plane_resources_;
};


#endif //LAMURE_VT_SYSTEM_H
