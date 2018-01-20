#include <lamure/vt/VTContext.h>
#include <lamure/vt/ext/imgui.h>
#include <lamure/vt/ext/imgui_impl_glfw_gl3.h>
#include <lamure/vt/ren/CutUpdate.h>
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
    _projection_matrix = scm::math::mat4f::identity();

    _scm_core.reset(new scm::core(0, nullptr));

    std::string vs_source;
    std::string fs_source;

    if(!scm::io::read_text_file(std::string(LAMURE_SHADERS_DIR) + "/virtual_texturing.glslv", vs_source) ||
       !scm::io::read_text_file(std::string(LAMURE_SHADERS_DIR) + "/virtual_texturing.glslf", fs_source))
    {
        scm::err() << "error reading shader files" << scm::log::end;
        throw std::runtime_error("Error reading shader files");
    }

    _device.reset(new scm::gl::render_device());
    _render_context = _device->main_context();

    {
        using namespace scm::gl;
        using namespace boost::assign;
        _shader_program = _device->create_program(list_of(_device->create_shader(STAGE_VERTEX_SHADER, vs_source))(_device->create_shader(STAGE_FRAGMENT_SHADER, fs_source)));
    }

    if(!_shader_program)
    {
        scm::err() << "error creating shader program" << scm::log::end;
        throw std::runtime_error("Error creating shader program");
    }

    _dstate_less = _device->create_depth_stencil_state(true, true, scm::gl::COMPARISON_LESS);

    // TODO: gua scenegraph to handle geometry eventually
    _obj.reset(new scm::gl::wavefront_obj_geometry(_device, std::string(LAMURE_PRIMITIVES_DIR) + "/quad.obj"));

    _filter_nearest = _device->create_sampler_state(scm::gl::FILTER_MIN_MAG_NEAREST, scm::gl::WRAP_CLAMP_TO_EDGE);
    _filter_linear = _device->create_sampler_state(scm::gl::FILTER_MIN_MAG_LINEAR, scm::gl::WRAP_CLAMP_TO_EDGE);

    _index_texture_dimension = scm::math::vec2ui(_vtcontext->get_size_index_texture(), _vtcontext->get_size_index_texture());
    _physical_texture_dimension = scm::math::vec2ui(_vtcontext->get_phys_tex_tile_width(), _vtcontext->get_phys_tex_tile_width());

    initialize_index_texture();
    initialize_physical_texture();
    initialize_feedback();

    apply_cut_update();

    _ms_no_cull = _device->create_rasterizer_state(scm::gl::FILL_SOLID, scm::gl::CULL_NONE, scm::gl::ORIENT_CCW, true);
}

void VTRenderer::render()
{
    _render_context->set_viewport(scm::gl::viewport(scm::math::vec2ui(0, 0), 1 * scm::math::vec2ui(_width, _height)));

    scm::math::mat4f view_matrix = _vtcontext->get_event_handler()->get_trackball_manip().transform_matrix();
    scm::math::mat4f model_matrix = scm::math::mat4f::identity();

    scm::math::mat4f model_view_matrix = view_matrix * model_matrix;

    _shader_program->uniform("projection_matrix", _projection_matrix);
    _shader_program->uniform("model_view_matrix", model_view_matrix);

    // upload necessary information to vertex shader
    _shader_program->uniform("in_physical_texture_dim", _physical_texture_dimension);
    _shader_program->uniform("in_index_texture_dim", _index_texture_dimension);
    _shader_program->uniform("in_max_level", ((uint32_t)_vtcontext->get_depth_quadtree()));
    _shader_program->uniform("in_toggle_view", _vtcontext->get_event_handler()->isToggle_phyiscal_texture_image_viewer());

    _shader_program->uniform("in_tile_size", (uint32_t)_vtcontext->get_size_tile());
    _shader_program->uniform("in_tile_padding", (uint32_t)_vtcontext->get_size_padding());

    _render_context->clear_default_color_buffer(scm::gl::FRAMEBUFFER_BACK, scm::math::vec4f(.6f, .2f, .2f, 1.0f));
    _render_context->clear_default_depth_stencil_buffer();

    _render_context->apply();

    {
        // multi sample pass
        scm::gl::context_state_objects_guard csg(_render_context);
        scm::gl::context_texture_units_guard tug(_render_context);
        scm::gl::context_framebuffer_guard fbg(_render_context);

        _render_context->set_depth_stencil_state(_dstate_less);

        // don't perform backface culling
        _render_context->set_rasterizer_state(_ms_no_cull);

        _render_context->bind_program(_shader_program);

        apply_cut_update();

        // bind our texture and tell the graphics card to filter the samples linearly
        // TODO physical texture later with linear filter
        _render_context->bind_texture(_physical_texture, _filter_nearest, 0);
        _render_context->bind_texture(_index_texture, _filter_nearest, 1);

        // bind feedback
        _render_context->bind_storage_buffer(_atomic_feedback_storage_ssbo, 0);

        _render_context->apply();

        scm::gl::timer_query_ptr timer_query = _device->create_timer_query();

        _render_context->begin_query(timer_query);

        _obj->draw(_render_context, scm::gl::geometry::MODE_SOLID);

        _render_context->end_query(timer_query);

        //////////////////////////////////////////////////////////////////////////////
        // FEEDBACK STUFF
        //////////////////////////////////////////////////////////////////////////////

        auto data = _render_context->map_buffer(_atomic_feedback_storage_ssbo, scm::gl::ACCESS_READ_ONLY);

        if(data)
        {
            memcpy(_copy_memory_new, data, _size_copy_buf);
        }

        _render_context->unmap_buffer(_atomic_feedback_storage_ssbo);

        _render_context->clear_buffer_data(_atomic_feedback_storage_ssbo, scm::gl::FORMAT_R_32UI, nullptr);

        std::stringstream stream_feedback;
        size_t phys_tex_tile_width = _vtcontext->get_phys_tex_tile_width();

        for(size_t x = 0; x < phys_tex_tile_width; ++x)
        {
            for(size_t y = 0; y < phys_tex_tile_width; ++y)
            {
                stream_feedback << _copy_memory_new[x + y * phys_tex_tile_width] << " ";
            }

            stream_feedback << std::endl;
        }

        _vtcontext->get_debug()->set_feedback_string(stream_feedback.str());

        _cut_update->feedback(_copy_memory_new);
    }
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
        stream_usage << "Physical texture slots usage: " << _vtcontext->get_debug()->get_mem_slots_busy() << "%";

        ImGui::ProgressBar(_vtcontext->get_debug()->get_mem_slots_busy(), ImVec2(0, 80), stream_usage.str().c_str());
    }

    ImGui::SetNextTreeNodeOpen(true);

    if(ImGui::CollapsingHeader("Cut update metrics"))
    {
        auto max_swap = *std::max_element(_vtcontext->get_debug()->get_cut_swap_times().begin(), _vtcontext->get_debug()->get_cut_swap_times().end());
        auto max_disp = *std::max_element(_vtcontext->get_debug()->get_cut_dispatch_times().begin(), _vtcontext->get_debug()->get_cut_dispatch_times().end());
        auto max_apply = *std::max_element(_vtcontext->get_debug()->get_apply_times().begin(), _vtcontext->get_debug()->get_apply_times().end());

        auto min_swap = *std::min_element(_vtcontext->get_debug()->get_cut_swap_times().begin(), _vtcontext->get_debug()->get_cut_swap_times().end());
        auto min_disp = *std::min_element(_vtcontext->get_debug()->get_cut_dispatch_times().begin(), _vtcontext->get_debug()->get_cut_dispatch_times().end());
        auto min_apply = *std::min_element(_vtcontext->get_debug()->get_apply_times().begin(), _vtcontext->get_debug()->get_apply_times().end());

        ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0), _vtcontext->get_debug()->get_cut_string().c_str());
        ImGui::Text(_vtcontext->get_debug()->get_mem_slots_string().c_str());
        ImGui::Text(_vtcontext->get_debug()->get_feedback_string().c_str());
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 1.0f, 1.0), _vtcontext->get_debug()->get_index_string().c_str());
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0), ("RAM pointer count: " + std::to_string(_vtcontext->get_debug()->get_size_mem_cut())).c_str());

        std::stringstream stream_swap_max;
        stream_swap_max << "Max: " << max_swap << " microsec";
        ImGui::PlotLines("Swap time, microsec", &_vtcontext->get_debug()->get_cut_swap_times()[0], VTContext::Debug::SWAP_S, 0, stream_swap_max.str().c_str(), min_swap, max_swap, plot_dims);

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
    Cut *cut = _vtcontext->get_cut_update()->start_reading_cut();

    auto start = std::chrono::high_resolution_clock::now();

    scm::gl::timer_query_ptr timer_query = _device->create_timer_query();

    _render_context->begin_query(timer_query);

    update_index_texture(cut->get_front_index());

    for(auto iter = cut->get_back_updated_nodes().begin(); iter != cut->get_back_updated_nodes().end(); iter++)
    {
        auto mem_iter = std::find(cut->get_front_mem_slots(), cut->get_front_mem_slots() + cut->get_size_feedback(), *iter);

        if(mem_iter == cut->get_front_mem_slots() + cut->get_size_feedback())
        {
            throw std::runtime_error("Updated node not in memory slots");
        }

        auto mem_slot = (size_t)std::distance(cut->get_front_mem_slots(), mem_iter);
        auto mem_cut_iter = cut->get_front_mem_cut().find(mem_slot);

        if(mem_cut_iter == cut->get_front_mem_cut().end())
        {
            throw std::runtime_error("Updated node not in memory cut");
        }

        // auto x = (uint8_t)((*mem_cut_iter).first % cut->get_size_mem_x());
        // auto y = (uint8_t)((*mem_cut_iter).first / cut->get_size_mem_x());
        auto slot = (*mem_cut_iter).first;

        update_physical_texture_blockwise((*mem_cut_iter).second, slot);
    }

    _render_context->end_query(timer_query);

    auto end = std::chrono::high_resolution_clock::now();
    _apply_time = std::chrono::duration<float, std::milli>(end - start).count();

    extract_debug_data(cut);

    _vtcontext->get_cut_update()->stop_reading_cut();
}

void VTRenderer::extract_debug_data(Cut *cut)
{
    _vtcontext->get_debug()->get_fps().push_back(ImGui::GetIO().Framerate);
    _vtcontext->get_debug()->get_fps().pop_front();

    _vtcontext->get_debug()->set_mem_slots_busy((cut->get_size_feedback() - cut->get_front_mem_slots_free().size()) / (float)cut->get_size_feedback());

    std::stringstream stream_cut;
    stream_cut << "Cut { ";
    for(id_type iter : cut->get_front_cut())
    {
        stream_cut << iter << " ";
    }
    stream_cut << "}" << std::endl;

    _vtcontext->get_debug()->set_cut_string(stream_cut.str());

    std::stringstream stream_mem_slots;
    for(size_t x = 0; x < cut->get_size_mem_x(); ++x)
    {
        for(size_t y = 0; y < cut->get_size_mem_y(); ++y)
        {
            if(cut->get_front_mem_slots()[x + y * cut->get_size_mem_x()] == UINT64_MAX)
            {
                stream_mem_slots << "F ";
            }
            else
            {
                stream_mem_slots << cut->get_front_mem_slots()[x + y * cut->get_size_mem_x()] << " ";
            }
        }

        stream_mem_slots << std::endl;
    }
    _vtcontext->get_debug()->set_mem_slots_string(stream_mem_slots.str());

    std::stringstream stream_index_string;
    for(size_t x = 0; x < _vtcontext->get_size_index_texture(); ++x)
    {
        for(size_t y = 0; y < _vtcontext->get_size_index_texture(); ++y)
        {
            auto ptr = &cut->get_front_index()[y * _vtcontext->get_size_index_texture() * 3 + x * 3];

            stream_index_string << (int)ptr[0] << "." << (int)ptr[1] << "." << (int)ptr[2] << " ";
        }

        stream_index_string << std::endl;
    }

    _vtcontext->get_debug()->set_mem_slots_string(stream_mem_slots.str());

    _vtcontext->get_debug()->set_index_string(stream_index_string.str());

    _vtcontext->get_debug()->get_cut_dispatch_times().push_back(_cut_update->get_dispatch_time());
    _vtcontext->get_debug()->get_cut_dispatch_times().pop_front();

    _vtcontext->get_debug()->get_cut_swap_times().push_back(cut->get_swap_time());
    _vtcontext->get_debug()->get_cut_swap_times().pop_front();

    _vtcontext->get_debug()->get_apply_times().push_back(_apply_time);
    _vtcontext->get_debug()->get_apply_times().pop_front();

    _vtcontext->get_debug()->set_size_mem_cut(cut->get_front_mem_cut().size());
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

    _physical_texture = _device->create_texture_2d(dimensions, get_tex_format(), 1, _vtcontext->get_phys_tex_layers());
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

void VTRenderer::update_physical_texture_blockwise(const uint8_t *buf_texel, size_t slot_id)
{
    size_t phys_tex_tile_width = _vtcontext->get_phys_tex_tile_width();
    size_t tile_px_width = _vtcontext->get_size_tile();

    size_t slots_per_texture = phys_tex_tile_width * phys_tex_tile_width;
    size_t layer = slot_id / slots_per_texture;
    size_t rel_slot_id = slot_id - layer * slots_per_texture;
    size_t x_tile = rel_slot_id % phys_tex_tile_width;
    size_t y_tile = rel_slot_id / phys_tex_tile_width;

    scm::math::vec3ui origin = scm::math::vec3ui(x_tile * tile_px_width, y_tile * tile_px_width, 0);

    scm::math::vec3ui dimensions = scm::math::vec3ui(tile_px_width, tile_px_width, 1);

    _render_context->update_sub_texture(_physical_texture, scm::gl::texture_region(origin, dimensions), layer, get_tex_format(), buf_texel);
}

void VTRenderer::update_physical_texture_blockwise(const uint8_t *buf_texel, uint32_t x, uint32_t y)
{
    scm::math::vec3ui origin = scm::math::vec3ui(x * _vtcontext->get_size_tile(), y * _vtcontext->get_size_tile(), 0);
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
    _shader_program.reset();
    _index_buffer.reset();
    _vertex_array.reset();

    _obj.reset();

    _filter_nearest.reset();
    _filter_linear.reset();

    _ms_no_cull.reset();

    _render_context.reset();
    _device.reset();
    _scm_core.reset();
}
void VTRenderer::resize(int _width, int _height)
{
    this->_width = static_cast<uint32_t>(_width);
    this->_height = static_cast<uint32_t>(_height);
    _render_context->set_viewport(scm::gl::viewport(scm::math::vec2ui(0, 0), scm::math::vec2ui(this->_width, this->_height)));
    scm::math::perspective_matrix(_projection_matrix, 60.f, float(_width) / float(_height), 0.01f, 1000.0f);
}
}