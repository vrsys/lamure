#ifndef VT_RENDERER_H
#define VT_RENDERER_H

#include <lamure/vt/common.h>

namespace vt
{
class VTContext;
class CutUpdate;
class VTRenderer
{
  public:
    VTRenderer(vt::VTContext *context, uint32_t _width, uint32_t _height);
    ~VTRenderer();

    void render();

    void resize(int _width, int _height);

    void render_debug_view();

  private:
    std::chrono::time_point<std::chrono::high_resolution_clock> _start;

    VTContext *_vtcontext;

    CutUpdate *_cut_update;

    scm::shared_ptr<scm::core> _scm_core;
    scm::shared_ptr<scm::gl::render_context> _render_context;

    scm::gl::texture_2d_ptr _physical_texture;
    scm::gl::texture_2d_ptr _index_texture;

    // necessary for feedback
    scm::gl::buffer_ptr _atomic_feedback_storage_ssbo;
    uint32_t *_copy_memory_new;
    size_t _size_copy_buf;

    scm::shared_ptr<scm::gl::render_device> _device;
    scm::gl::program_ptr _shader_vt, _shader_atmosphere, _shader_textured_quad;
    scm::gl::buffer_ptr _index_buffer;
    scm::gl::vertex_array_ptr _vertex_array;
    scm::math::mat4 _projection_matrix;
    scm::shared_ptr<scm::gl::wavefront_obj_geometry> _obj;
    scm::shared_ptr<scm::gl::quad_geometry> _quad;
    scm::gl::depth_stencil_state_ptr _dstate_less;
    scm::gl::sampler_state_ptr _filter_nearest;
    scm::gl::sampler_state_ptr _filter_linear;
    scm::gl::rasterizer_state_ptr _ms_no_cull, _ms_cull;
    scm::gl::blend_state_ptr _blend_state;


    scm::gl::blend_state_ptr _blend_state_halo;
    scm::gl::depth_stencil_state_ptr _dstate_disable;
    uint32_t _halo_res = 4096;


    scm::gl::frame_buffer_ptr                   _fbo_halo ;
    scm::gl::texture_2d_ptr                     _color_halo ;
    scm::gl::texture_2d_ptr                     _depth_halo ;

    uint32_t _width, _height;

    scm::math::vec2ui _index_texture_dimension;
    scm::math::vec2ui _physical_texture_dimension;

    float _apply_time;

    void init();
    void initialize_index_texture();
    void initialize_physical_texture();
    void initialize_feedback();
    void update_index_texture(const uint8_t *buf_cpu);
    void update_physical_texture_blockwise(const uint8_t *buf_texel, size_t slot_id);
    void apply_cut_update();
    void extract_debug_cut(Cut *cut);
    void extract_debug_feedback();

    scm::gl::data_format get_tex_format();
};
}

#endif // VT_RENDERER_H