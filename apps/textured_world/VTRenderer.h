#ifndef VT_RENDERER_H
#define VT_RENDERER_H

#include <chrono>
#include <lamure/vt/common.h>

#include <scm/core.h>
#include <scm/core/io/tools.h>
#include <scm/core/pointer_types.h>
#include <scm/core/time/accum_timer.h>
#include <scm/core/time/high_res_timer.h>
#include <scm/log.h>

#include <scm/gl_core.h>

#include <scm/gl_core/render_device/context.h>
#include <scm/gl_core/texture_objects/texture_objects_fwd.h>

#include <scm/gl_util/data/imaging/texture_loader.h>
#include <scm/gl_util/manipulators/trackball_manipulator.h>
#include <scm/gl_util/primitives/box.h>
#include <scm/gl_util/primitives/quad.h>
#include <scm/gl_util/primitives/wavefront_obj.h>

namespace vt
{
class VTContext;
class CutUpdate;
class VTRenderer
{
  public:
    explicit VTRenderer(CutUpdate *cut_update);
    ~VTRenderer();

    void render(uint32_t color_data_id, uint32_t elevation_data_id, uint16_t view_id, uint16_t context_id);
    void update_view(uint16_t view_id, uint32_t width, uint32_t height, float scale, const scm::math::mat4f &view_matrix);

    void add_data(uint64_t cut_id, uint32_t data_id);
    void add_view(uint16_t view_id, uint32_t width, uint32_t height, float scale);
    void add_context(uint16_t context_id);

    void extract_debug_cut(uint32_t data_id, uint16_t view_id, uint16_t context_id);
    void extract_debug_context(uint16_t context_id);

    void render_debug_cut(uint32_t data_id, uint16_t view_id, uint16_t context_id);
    void render_debug_context(uint16_t context_id);

    void collect_feedback(uint16_t context_id);
    void clear_buffers(uint16_t context_id);
private:
    CutUpdate *_cut_update;

    scm::shared_ptr<scm::core> _scm_core;

    scm::shared_ptr<scm::gl::render_device> _device;
    scm::gl::depth_stencil_state_ptr _dstate_less;
    scm::gl::sampler_state_ptr _filter_nearest;
    scm::gl::sampler_state_ptr _filter_linear;
    scm::gl::rasterizer_state_ptr _ms_no_cull, _ms_cull;
    scm::gl::blend_state_ptr _blend_state;

    scm::gl::program_ptr _shader_vt;
    scm::shared_ptr<scm::gl::wavefront_obj_geometry> _obj;

    float _apply_time;

    struct debug_cut
    {
        debug_cut(){}

        std::string _string_cut;
        std::string _string_index;
    };

    struct debug_context
    {
        static const int FPS_S = 60;
        static const int DISP_S = 60;
        static const int APPLY_S = 60;

        debug_context() : _fps(FPS_S, 0.0f), _times_cut_dispatch(DISP_S, 0.0f), _times_apply(APPLY_S, 0.0f) { _mem_slots_busy = 0.0f; }

        std::string _string_mem_slots;
        std::string _string_feedback;
        std::deque<float> _fps;
        std::deque<float> _times_cut_dispatch;
        std::deque<float> _times_apply;
        float _mem_slots_busy;
    };

    struct data_resource
    {
        // TODO: change to std::map<uint_64_t, std::vector<scm::gl::texture_2d_ptr>> ; get rid of dimensions (dynamic)
        scm::gl::texture_2d_ptr _index_texture;
        scm::math::vec2ui _index_texture_dimension;

        std::vector<scm::gl::texture_2d_ptr> _index_hierarchy;
    };

    struct ctxt_resource
    {
        scm::shared_ptr<scm::gl::render_context> _render_context;
        scm::gl::texture_2d_ptr _physical_texture;
        scm::math::vec2ui _physical_texture_dimension;

        size_t _size_feedback;
        int32_t *_feedback_cpu_buffer;
        scm::gl::buffer_ptr _feedback_storage;
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
    void update_index_texture(uint32_t data_id, uint16_t context_id, const uint8_t *buf_cpu);
    void update_physical_texture_blockwise(uint16_t context_id, const uint8_t *buf_texel, size_t slot_position);

    scm::gl::data_format get_tex_format();

    std::map<uint32_t, data_resource *> _data_resources;
    std::map<uint16_t, ctxt_resource *> _ctxt_resources;
    std::map<uint16_t, view_resource *> _view_resources;
    std::map<uint64_t, debug_cut *> _cut_debug_outputs;
    std::map<uint64_t, debug_context *> _ctxt_debug_outputs;
};
}

#endif // VT_RENDERER_H
