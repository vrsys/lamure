#ifndef VT_RENDERER_H
#define VT_RENDERER_H

#include <chrono>
#include <lamure/vt/common.h>
#include <scm/core.h>
#include <scm/gl_core/render_device/context.h>
#include <scm/gl_core/texture_objects/texture_objects_fwd.h>
#include <scm/gl_util/primitives/wavefront_obj.h>

namespace vt
{
class VTContext;
class CutUpdate;
class VTRenderer
{
  public:
    VTRenderer(CutUpdate *cut_update);
    ~VTRenderer();

    void render(uint32_t data_id, uint16_t view_id, uint16_t context_id);
    void update_view(uint16_t view_id, uint32_t width, uint32_t height, float scale, const scm::math::mat4f & view_matrix);

    void add_data(uint64_t cut_id, uint32_t dataset_id, const std::string &file_geometry);
    void add_view(uint16_t view_id, uint32_t width, uint32_t height, float scale);
    void add_context(uint16_t context_id);

  private:
    CutUpdate *_cut_update;

    scm::shared_ptr<scm::core> _scm_core;

    scm::shared_ptr<scm::gl::render_device> _device;
    scm::gl::program_ptr _shader_vt;
    scm::gl::depth_stencil_state_ptr _dstate_less;
    scm::gl::sampler_state_ptr _filter_nearest;
    scm::gl::sampler_state_ptr _filter_linear;
    scm::gl::rasterizer_state_ptr _ms_no_cull, _ms_cull;
    scm::gl::blend_state_ptr _blend_state;

    struct dataset_resource
    {
        scm::gl::texture_2d_ptr _index_texture;
        scm::math::vec2ui _index_texture_dimension;
        scm::shared_ptr<scm::gl::wavefront_obj_geometry> _obj;
    };

    struct context_resource
    {
        scm::shared_ptr<scm::gl::render_context> _render_context;
        scm::gl::texture_2d_ptr _physical_texture;
        scm::math::vec2ui _physical_texture_dimension;

        size_t _size_copy_buf;
        uint32_t *_copy_memory_new;
        scm::gl::buffer_ptr _atomic_feedback_storage_ssbo;
    };

    struct view_resource
    {
        uint32_t _width, _height;
        float _scale;
        scm::math::mat4f _view_matrix;
        std::chrono::time_point<std::chrono::high_resolution_clock> _start;
    };

    void init();

    void apply_cut_update(uint16_t context_id);
    void update_index_texture(uint32_t dataset_id, uint16_t context_id, const uint8_t *buf_cpu);
    void update_physical_texture_blockwise(uint16_t context_id, const uint8_t *buf_texel, size_t slot_position);

    scm::gl::data_format get_tex_format();

    std::map<uint32_t, dataset_resource*> _dataset_resources;
    std::map<uint16_t, context_resource*> _context_resources;
    std::map<uint16_t, view_resource*> _view_resources;
};
}

#endif // VT_RENDERER_H
