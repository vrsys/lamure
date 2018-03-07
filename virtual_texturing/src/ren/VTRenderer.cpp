#include <lamure/vt/VTContext.h>
#include <lamure/vt/ext/imgui.h>
#include <lamure/vt/ext/imgui_impl_glfw_gl3.h>
#include <lamure/vt/ren/CutUpdate.h>
#include <lamure/vt/ren/CutDatabase.h>
#include <lamure/vt/ren/VTRenderer.h>

namespace vt
{
VTRenderer::VTRenderer(vt::VTContext *context, uint32_t width, uint32_t height)
{
    this->_vtcontext = context;
    this->_cut_update = context->get_cut_update();
    this->_width = width;
    this->_height = height;
    this->init();
}

void VTRenderer::init()
{
    _start = std::chrono::system_clock::now();

    _projection_matrix = scm::math::mat4f::identity();

    _scm_core.reset(new scm::core(0, nullptr));

    std::string vs_vt, fs_vt, vs_atmosphere, fs_atmosphere;

    if(!scm::io::read_text_file(std::string(LAMURE_SHADERS_DIR) + "/virtual_texturing.glslv", vs_vt) || !scm::io::read_text_file(std::string(LAMURE_SHADERS_DIR) + "/virtual_texturing.glslf", fs_vt) ||
       !scm::io::read_text_file(std::string(LAMURE_SHADERS_DIR) + "/atmosphere.glslv", vs_atmosphere) || !scm::io::read_text_file(std::string(LAMURE_SHADERS_DIR) + "/atmosphere.glslf", fs_atmosphere))
    {
        scm::err() << "error reading shader files" << scm::log::end;
        throw std::runtime_error("Error reading shader files");
    }

    _device.reset(new scm::gl::render_device());
    _render_context = _device->main_context();

    std::string v_pass = "\
        #version 440\n\
        \
        uniform mat4 mvp;\
        out vec2 tex_coord;\
        layout(location = 0) in vec3 in_position;\
        layout(location = 2) in vec2 in_texture_coord;\
        void main()\
        {\
            gl_Position = mvp * vec4(in_position, 1.0);\
            tex_coord = in_texture_coord;\
        }\
        ";
    std::string f_pass = "\
        #version 440\n\
        \
        in vec2 tex_coord;\
        uniform sampler2D in_texture;\
        uniform int halo_res;\
        layout(location = 0) out vec4 out_color;\
        void main()\
        {\
            out_color = texelFetch(in_texture, ivec2(tex_coord.xy*halo_res), 0).rgba;\
        }\
        ";
    {
        using namespace scm::gl;
        using namespace boost::assign;

        _shader_vt = _device->create_program(list_of(_device->create_shader(STAGE_VERTEX_SHADER, vs_vt))(_device->create_shader(STAGE_FRAGMENT_SHADER, fs_vt)));
        _shader_atmosphere = _device->create_program(list_of(_device->create_shader(STAGE_VERTEX_SHADER, vs_atmosphere))(_device->create_shader(STAGE_FRAGMENT_SHADER, fs_atmosphere)));
        _shader_textured_quad = _device->create_program(list_of(_device->create_shader(STAGE_VERTEX_SHADER, v_pass))(_device->create_shader(STAGE_FRAGMENT_SHADER, f_pass)));
    }

    if(!_shader_vt || !_shader_atmosphere)
    {
        scm::err() << "error creating shader program" << scm::log::end;
        throw std::runtime_error("Error creating shader program");
    }

    _dstate_less = _device->create_depth_stencil_state(true, true, scm::gl::COMPARISON_LESS);
    _blend_state = _device->create_blend_state(true, scm::gl::FUNC_SRC_COLOR, scm::gl::FUNC_ONE_MINUS_SRC_ALPHA, scm::gl::FUNC_SRC_ALPHA, scm::gl::FUNC_ONE_MINUS_SRC_ALPHA);

    // TODO: gua scenegraph to handle geometry eventually
    _obj.reset(new scm::gl::wavefront_obj_geometry(_device, std::string(LAMURE_PRIMITIVES_DIR) + "/world_smooth.obj"));
    // _obj.reset(new scm::gl::wavefront_obj_geometry(_device, std::string(LAMURE_PRIMITIVES_DIR) + "/world_smooth_finer.obj"));
    _quad.reset(new scm::gl::quad_geometry(_device, scm::math::vec2f(-2.f * _halo_res / _halo_res, -2.f), scm::math::vec2f(2.f * _halo_res / _halo_res, 2.f)));

    _filter_nearest = _device->create_sampler_state(scm::gl::FILTER_MIN_MAG_NEAREST, scm::gl::WRAP_CLAMP_TO_EDGE);
    _filter_linear = _device->create_sampler_state(scm::gl::FILTER_MIN_MAG_LINEAR, scm::gl::WRAP_CLAMP_TO_EDGE);

    _index_texture_dimension = scm::math::vec2ui(_vtcontext->get_size_index_texture(), _vtcontext->get_size_index_texture());
    _physical_texture_dimension = scm::math::vec2ui(_vtcontext->get_phys_tex_tile_width(), _vtcontext->get_phys_tex_tile_width());

    initialize_index_texture();
    initialize_physical_texture();
    initialize_feedback();

    _ms_no_cull = _device->create_rasterizer_state(scm::gl::FILL_SOLID, scm::gl::CULL_NONE, scm::gl::ORIENT_CCW, true);
    _ms_cull = _device->create_rasterizer_state(scm::gl::FILL_SOLID, scm::gl::CULL_BACK, scm::gl::ORIENT_CCW, true);

    _fbo_halo = _device->create_frame_buffer();

    _depth_halo = _device->create_texture_2d(scm::math::vec2ui(_halo_res, _halo_res) * 1, scm::gl::FORMAT_D32F, 1, 1, 1);
    _color_halo = _device->create_texture_2d(scm::math::vec2ui(_halo_res, _halo_res) * 1, scm::gl::FORMAT_RGBA_32F, 1, 1, 1);

    _fbo_halo->attach_color_buffer(0, _color_halo);
    _fbo_halo->attach_depth_stencil_buffer(_depth_halo);

    _dstate_disable = _device->create_depth_stencil_state(false, true, scm::gl::COMPARISON_NEVER);

    _blend_state_halo = _device->create_blend_state(true, scm::gl::FUNC_ONE, scm::gl::FUNC_ONE, scm::gl::FUNC_ONE, scm::gl::FUNC_ONE, scm::gl::EQ_FUNC_ADD, scm::gl::EQ_FUNC_ADD);

    apply_cut_update();
}

void VTRenderer::render()
{
    float scale = _vtcontext->get_event_handler()->get_scale();
    scm::math::perspective_matrix(_projection_matrix, 10.f + scale * 100.f, float(_width) / float(_height), 0.01f, 1000.0f);
    std::chrono::duration<double> elapsed_seconds = std::chrono::high_resolution_clock::now() - _start;

    _render_context->set_default_frame_buffer();

    scm::math::mat4f view_matrix = _vtcontext->get_event_handler()->get_trackball_manip().transform_matrix();
    scm::math::mat4f model_matrix = scm::math::mat4f::identity() * scm::math::make_rotation((float)elapsed_seconds.count(), 0.f, 1.f, 0.f); // * scm::math::make_scale(scale, scale, scale);

    scm::math::mat4f model_view_matrix = view_matrix * model_matrix;
    _shader_vt->uniform("projection_matrix", _projection_matrix);
    _shader_vt->uniform("model_view_matrix", model_view_matrix);

    // upload necessary information to vertex shader
    _shader_vt->uniform("in_physical_texture_dim", _physical_texture_dimension);
    _shader_vt->uniform("in_index_texture_dim", _index_texture_dimension);
    _shader_vt->uniform("in_max_level", ((uint32_t)_vtcontext->get_depth_quadtree()));
    _shader_vt->uniform("in_toggle_view", _vtcontext->get_event_handler()->isToggle_phyiscal_texture_image_viewer());

    _shader_vt->uniform("in_tile_size", (uint32_t)_vtcontext->get_size_tile());
    _shader_vt->uniform("in_tile_padding", (uint32_t)_vtcontext->get_size_padding());

    _render_context->clear_default_color_buffer(scm::gl::FRAMEBUFFER_BACK, scm::math::vec4f(.0f, .0f, .0f, 1.0f));
    _render_context->clear_default_depth_stencil_buffer();

    _render_context->apply();

    scm::gl::context_state_objects_guard csg(_render_context);
    scm::gl::context_texture_units_guard tug(_render_context);
    scm::gl::context_framebuffer_guard fbg(_render_context);

    _render_context->set_viewport(scm::gl::viewport(scm::math::vec2ui(0, 0), 1 * scm::math::vec2ui(_width, _height)));

    _render_context->set_depth_stencil_state(_dstate_less);
    _render_context->set_rasterizer_state(_ms_cull);
    _render_context->set_blend_state(_blend_state);

    _render_context->bind_program(_shader_vt);

    _render_context->sync();

    apply_cut_update();

    // bind our texture and tell the graphics card to filter the samples linearly
    _render_context->bind_texture(_physical_texture, _filter_linear, 0);
    _render_context->bind_texture(_index_texture, _filter_nearest, 1);

    // bind feedback
    _render_context->bind_storage_buffer(_atomic_feedback_storage_ssbo, 0);

    _render_context->apply();

    _obj->draw(_render_context, scm::gl::geometry::MODE_SOLID);

    auto data = _render_context->map_buffer(_atomic_feedback_storage_ssbo, scm::gl::ACCESS_READ_ONLY);

    if(data)
    {
        memcpy(_copy_memory_new, data, _size_copy_buf);
    }

    _render_context->unmap_buffer(_atomic_feedback_storage_ssbo);

    _render_context->clear_buffer_data(_atomic_feedback_storage_ssbo, scm::gl::FORMAT_R_32UI, nullptr);

    _render_context->sync();

    _cut_update->feedback(_copy_memory_new);
}

void VTRenderer::render_debug_view()
{
    ImGui_ImplGlfwGL3_NewFrame();

    ImVec2 plot_dims(0, 160);

    ImGui::SetNextTreeNodeOpen(true);

    if(ImGui::CollapsingHeader("Performance metrics"))
    {
        auto max_fps = *std::max_element(_vtcontext->get_debug()->get_fps().begin(), _vtcontext->get_debug()->get_fps().end());
        auto min_fps = *std::min_element(_vtcontext->get_debug()->get_fps().begin(), _vtcontext->get_debug()->get_fps().end());

        std::stringstream stream_average;
        stream_average << "Application average " << 1000.0f / ImGui::GetIO().Framerate << " ms/frame (" << ImGui::GetIO().Framerate << " FPS)\"";

        ImGui::PlotHistogram("FPS", &_vtcontext->get_debug()->get_fps()[0], VTContext::Debug::FPS_S, 0, stream_average.str().c_str(), min_fps, max_fps, plot_dims);

        std::stringstream stream_usage;
        stream_usage << "Physical texture slots usage: " << _vtcontext->get_debug()->get_mem_slots_busy() * 100 << "%";

        ImGui::ProgressBar(_vtcontext->get_debug()->get_mem_slots_busy(), ImVec2(0, 80), stream_usage.str().c_str());
    }

    ImGui::SetNextTreeNodeOpen(true);

    if(ImGui::CollapsingHeader("Cut update metrics"))
    {
        auto max_disp = *std::max_element(_vtcontext->get_debug()->get_cut_dispatch_times().begin(), _vtcontext->get_debug()->get_cut_dispatch_times().end());
        auto max_apply = *std::max_element(_vtcontext->get_debug()->get_apply_times().begin(), _vtcontext->get_debug()->get_apply_times().end());

        auto min_disp = *std::min_element(_vtcontext->get_debug()->get_cut_dispatch_times().begin(), _vtcontext->get_debug()->get_cut_dispatch_times().end());
        auto min_apply = *std::min_element(_vtcontext->get_debug()->get_apply_times().begin(), _vtcontext->get_debug()->get_apply_times().end());

        ImGui::Text(_vtcontext->get_debug()->get_feedback_string().c_str());

        std::stringstream stream_dispatch_max;
        stream_dispatch_max << "Max: " << max_disp << " msec";
        ImGui::PlotLines("Dispatch time, msec", &_vtcontext->get_debug()->get_cut_dispatch_times()[0], VTContext::Debug::DISP_S, 0, stream_dispatch_max.str().c_str(), min_disp, max_disp, plot_dims);

        std::stringstream stream_apply_max;
        stream_apply_max << "Max: " << max_apply << " msec";
        ImGui::PlotLines("Apply time, msec", &_vtcontext->get_debug()->get_apply_times()[0], VTContext::Debug::APPLY_S, 0, stream_apply_max.str().c_str(), min_apply, max_apply, plot_dims);
    }

    ImGui::Render();
}

void VTRenderer::apply_cut_update()
{
    auto start = std::chrono::high_resolution_clock::now();

    CutDatabase *cut_db = _vtcontext->get_cut_update()->get_cut_db();

    cut_db->start_reading();

    for(cut_map_entry_type cut_entry : (*cut_db->get_cut_map()))
    {
        Cut *cut = cut_db->start_reading_cut(cut_entry.first);

        update_index_texture(cut->get_front().get_index());

        for (auto position_slot_updated : cut->get_front().get_mem_slots_updated())
        {
            const mem_slot_type *mem_slot_updated = &cut_db->get_front().at(position_slot_updated.second);

            if (mem_slot_updated == nullptr || !mem_slot_updated->updated || !mem_slot_updated->locked || mem_slot_updated->pointer == nullptr)
            {
                if (mem_slot_updated == nullptr)
                {
                    std::cerr << "Mem slot at " << position_slot_updated.second << " is null" << std::endl;
                }
                else
                {
                    std::cerr << "Mem slot at " << position_slot_updated.second << std::endl;
                    std::cerr << "Mem slot #" << mem_slot_updated->position << std::endl;
                    std::cerr << "Tile id: " << mem_slot_updated->tile_id << std::endl;
                    std::cerr << "Locked: " << mem_slot_updated->locked << std::endl;
                    std::cerr << "Updated: " << mem_slot_updated->updated << std::endl;
                    std::cerr << "Pointer valid: " << (mem_slot_updated->pointer != nullptr) << std::endl;
                }

                throw std::runtime_error("updated mem slot inconsistency");
            }

            update_physical_texture_blockwise(mem_slot_updated->pointer, mem_slot_updated->position);
        }

        cut_db->stop_reading_cut(cut_entry.first);
    }

    cut_db->stop_reading();

    _render_context->sync();

    auto end = std::chrono::high_resolution_clock::now();
    _apply_time = std::chrono::duration<float, std::milli>(end - start).count();

    if (_vtcontext->is_show_debug_view())
    {
        extract_debug_metrics();
    }
}

void VTRenderer::extract_debug_metrics()
{
    _vtcontext->get_debug()->get_fps().push_back(ImGui::GetIO().Framerate);
    _vtcontext->get_debug()->get_fps().pop_front();

    _vtcontext->get_debug()->get_cut_dispatch_times().push_back(_cut_update->get_dispatch_time());
    _vtcontext->get_debug()->get_cut_dispatch_times().pop_front();

    _vtcontext->get_debug()->get_apply_times().push_back(_apply_time);
    _vtcontext->get_debug()->get_apply_times().pop_front();
}

void VTRenderer::initialize_index_texture() { _index_texture = _device->create_texture_2d(_index_texture_dimension, scm::gl::FORMAT_RGBA_8UI); }

void VTRenderer::update_index_texture(const uint8_t *buf_cpu)
{
    scm::math::vec3ui origin = scm::math::vec3ui(0, 0, 0);
    scm::math::vec3ui dimensions = scm::math::vec3ui(_index_texture_dimension, 1);

    _render_context->update_sub_texture(_index_texture, scm::gl::texture_region(origin, dimensions), 0, scm::gl::FORMAT_RGBA_8UI, buf_cpu);
}

void VTRenderer::initialize_physical_texture()
{
    scm::math::vec2ui dimensions(_vtcontext->get_phys_tex_px_width(), _vtcontext->get_phys_tex_px_width());
    _physical_texture = _device->create_texture_2d(dimensions, get_tex_format(), 0, _vtcontext->get_phys_tex_layers() + 1);
}

scm::gl::data_format VTRenderer::get_tex_format()
{
    switch(_vtcontext->get_format_texture())
    {
    case VTContext::Config::R8:
        return scm::gl::FORMAT_R_8;
    case VTContext::Config::RGB8:
        return scm::gl::FORMAT_RGB_8;
    case VTContext::Config::RGBA8:
    default:
        return scm::gl::FORMAT_RGBA_8;
    }
}

void VTRenderer::update_physical_texture_blockwise(const uint8_t *buf_texel, size_t slot_position)
{
    size_t slots_per_texture = _vtcontext->get_phys_tex_tile_width() * _vtcontext->get_phys_tex_tile_width();
    size_t layer = slot_position / slots_per_texture;
    size_t rel_slot_position = slot_position - layer * slots_per_texture;
    size_t x_tile = rel_slot_position % _vtcontext->get_phys_tex_tile_width();
    size_t y_tile = rel_slot_position / _vtcontext->get_phys_tex_tile_width();

    scm::math::vec3ui origin = scm::math::vec3ui((uint32_t)x_tile * _vtcontext->get_size_tile(), (uint32_t)y_tile * _vtcontext->get_size_tile(), (uint32_t)layer);
    scm::math::vec3ui dimensions = scm::math::vec3ui(_vtcontext->get_size_tile(), _vtcontext->get_size_tile(), 1);

    _render_context->update_sub_texture(_physical_texture, scm::gl::texture_region(origin, dimensions), 0, get_tex_format(), buf_texel);
}

void VTRenderer::initialize_feedback()
{
    size_t copy_buffer_len = _vtcontext->get_phys_tex_tile_width() * _vtcontext->get_phys_tex_tile_width() * _vtcontext->get_phys_tex_layers();
    _size_copy_buf = copy_buffer_len * size_of_format(scm::gl::FORMAT_R_32UI);

    _atomic_feedback_storage_ssbo = _device->create_buffer(scm::gl::BIND_STORAGE_BUFFER, scm::gl::USAGE_STREAM_COPY, _size_copy_buf);

    _copy_memory_new = new uint32_t[copy_buffer_len];

    for(size_t i = 0; i < copy_buffer_len; ++i)
    {
        _copy_memory_new[i] = 0;
    }
}

VTRenderer::~VTRenderer()
{
    _fbo_halo.reset();
    _color_halo.reset();
    _depth_halo.reset();

    _shader_vt.reset();
    _index_buffer.reset();
    _vertex_array.reset();

    _obj.reset();
    _quad.reset();

    _filter_nearest.reset();
    _filter_linear.reset();

    _ms_no_cull.reset();

    _render_context.reset();
    _device.reset();
    _scm_core.reset();

    _shader_textured_quad.reset();
}
void VTRenderer::resize(int _width, int _height)
{
    this->_width = static_cast<uint32_t>(_width);
    this->_height = static_cast<uint32_t>(_height);
    _render_context->set_viewport(scm::gl::viewport(scm::math::vec2ui(0, 0), scm::math::vec2ui(this->_width, this->_height)));
    scm::math::perspective_matrix(_projection_matrix, 10.f, float(_width) / float(_height), 0.01f, 1000.0f);
}
}