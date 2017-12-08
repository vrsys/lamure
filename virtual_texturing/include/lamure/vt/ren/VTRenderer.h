#ifndef VT_RENDERER_H
#define VT_RENDERER_H

#include <lamure/vt/VTContext.h>
#include <lamure/vt/common.h>

namespace vt
{
class VTRenderer
{
  public:
    VTRenderer(VTContext *context, uint32_t _width, uint32_t _height, CutUpdate *cut_update);
    ~VTRenderer();

    void render();
    void render_feedback();

    void resize(int _width, int _height);

    void update_index_texture(std::vector<uint8_t> const &cpu_buffer);

  private:
    VTContext *_vtcontext;
    CutUpdate *_cut_update;

    scm::shared_ptr<scm::core> _scm_core;
    scm::shared_ptr<scm::gl::render_context> _render_context;
    scm::gl::texture_2d_ptr _physical_texture;
    scm::gl::texture_2d_ptr _index_texture;
    scm::gl::texture_2d_ptr _feedback_image;

    // necessary for feedback
    scm::gl::frame_buffer_ptr _copy_framebuffer;
    scm::gl::buffer_ptr _copy_buffer_0;
    scm::gl::buffer_ptr _copy_buffer_1;
    scm::gl::sync_ptr _capture_finished;
    shared_ptr<uint32_t> _copy_memory;
    size_t _copy_buffer_size;

    scm::shared_ptr<scm::gl::render_device> _device;
    scm::gl::program_ptr _shader_program;
    scm::gl::buffer_ptr _index_buffer;
    scm::gl::vertex_array_ptr _vertex_array;
    scm::math::mat4f _projection_matrix;
    scm::shared_ptr<scm::gl::wavefront_obj_geometry> _obj;
    scm::gl::depth_stencil_state_ptr _dstate_less;
    scm::gl::sampler_state_ptr _filter_nearest;
    scm::gl::sampler_state_ptr _filter_linear;
    scm::gl::rasterizer_state_ptr _ms_no_cull;

    uint32_t _width, _height;

    scm::math::vec2ui _index_texture_dimension;
    scm::math::vec2ui _physical_texture_dimension;

    void init();
    void initialize_index_texture();
    void initialize_physical_texture();
    void initialize_feedback();
    void update_physical_texture_blockwise(char *buffer, uint32_t x, uint32_t y);
    void physical_texture_test_layout();
    void reset_feedback_image();
};
}

#endif // VT_RENDERER_H
