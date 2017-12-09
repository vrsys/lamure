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
    VTRenderer(vt::VTContext *context, uint32_t _width, uint32_t _height, CutUpdate *cut_update);
    ~VTRenderer();

    void render();

    void resize(int _width, int _height);

    void update_index_texture(uint8_t *cpu_buffer);
    void update_index_texture(std::vector<uint8_t> const &cpu_buffer);
    void update_physical_texture_blockwise(char *buffer, uint32_t x, uint32_t y);

  private:
    VTContext *_vtcontext;

    CutUpdate *_cut_update;

    scm::shared_ptr<scm::core> _scm_core;
    scm::shared_ptr<scm::gl::render_context> _render_context;

    scm::gl::texture_2d_ptr _physical_texture;
    scm::gl::texture_2d_ptr _index_texture;

    // necessary for feedback
    scm::gl::buffer_ptr _atomic_feedback_storage_ssbo;
    std::vector<uint32_t> _copy_memory;
    uint32_t *_copy_memory_new;
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
    uint8_t *_indexBuffer;
    std::atomic<bool> _indexBufferReady;
    std::mutex _indexBufferLock;

    scm::math::vec2ui _index_texture_dimension;
    scm::math::vec2ui _physical_texture_dimension;

    void init();
    void initialize_index_texture();
    void initialize_physical_texture();
    void initialize_feedback();
    void physical_texture_test_layout();
};
}

#endif // VT_RENDERER_H
