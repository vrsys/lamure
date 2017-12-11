#include <lamure/vt/VTContext.h>
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
    _physical_texture_dimension = _vtcontext->calculate_size_physical_texture();

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
    scm::math::mat4f mv_inv_transpose = transpose(inverse(model_view_matrix));

    _shader_program->uniform("projection_matrix", _projection_matrix);
    _shader_program->uniform("model_view_matrix", model_view_matrix);
    _shader_program->uniform("model_view_matrix_inverse_transpose", mv_inv_transpose);

    // upload necessary information to vertex shader
    _shader_program->uniform("in_physical_texture_dim", _physical_texture_dimension);
    _shader_program->uniform("in_index_texture_dim", _index_texture_dimension);
    _shader_program->uniform("in_max_level", ((uint32_t)_vtcontext->get_depth_quadtree()));
    _shader_program->uniform("in_toggle_view", _vtcontext->get_event_handler()->isToggle_phyiscal_texture_image_viewer());

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

        _obj->draw(_render_context, scm::gl::geometry::MODE_SOLID);

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
        _cut_update->feedback(_copy_memory_new);
    }
}

void VTRenderer::apply_cut_update()
{
    Cut *cut = _vtcontext->get_cut_update()->start_reading_cut();

    update_index_texture(cut->get_front_index());

    auto mem_cut = cut->get_front_mem_cut();
    while(!mem_cut.empty())
    {
        auto mem_tile = mem_cut.front();
        mem_cut.pop();

        auto mem_iter = std::find(cut->get_front_mem_slots(), cut->get_front_mem_slots() + cut->get_size_feedback(), mem_tile.first);

        if(mem_iter == cut->get_front_mem_slots() + cut->get_size_feedback())
        {
            continue;
        }

        auto mem_index = (size_t)std::distance(cut->get_front_mem_slots(), mem_iter);
        auto x = (uint8_t)(mem_index % cut->get_size_mem_x());
        auto y = (uint8_t)(mem_index / cut->get_size_mem_x());

        update_physical_texture_blockwise(mem_tile.second, x, y);
    }

    _vtcontext->get_cut_update()->stop_reading_cut();
}

void VTRenderer::initialize_index_texture() { _index_texture = _device->create_texture_2d(_index_texture_dimension, scm::gl::FORMAT_RGB_8UI); }

void VTRenderer::update_index_texture(const uint8_t *buf_cpu)
{
    scm::math::vec3ui origin = scm::math::vec3ui(0, 0, 0);
    scm::math::vec3ui dimensions = scm::math::vec3ui(_index_texture_dimension, 1);

    _render_context->update_sub_texture(_index_texture, scm::gl::texture_region(origin, dimensions), 0, scm::gl::FORMAT_RGB_8UI, buf_cpu);
}

void VTRenderer::initialize_physical_texture() { _physical_texture = _device->create_texture_2d(_physical_texture_dimension * _vtcontext->get_size_tile(), scm::gl::FORMAT_RGBA_8); }

void VTRenderer::update_physical_texture_blockwise(const uint8_t *buf_texel, uint32_t x, uint32_t y)
{
    scm::math::vec3ui origin = scm::math::vec3ui(x * _vtcontext->get_size_tile(), y * _vtcontext->get_size_tile(), 0);
    scm::math::vec3ui dimensions = scm::math::vec3ui(_vtcontext->get_size_tile(), _vtcontext->get_size_tile(), 1);

    _render_context->update_sub_texture(_physical_texture, scm::gl::texture_region(origin, dimensions), 0, scm::gl::FORMAT_RGBA_8, buf_texel);
}

void VTRenderer::initialize_feedback()
{
    _size_copy_buf = _physical_texture_dimension.x * _physical_texture_dimension.y * size_of_format(scm::gl::FORMAT_R_32UI);

    _atomic_feedback_storage_ssbo = _device->create_buffer(scm::gl::BIND_STORAGE_BUFFER, scm::gl::USAGE_STREAM_COPY, _size_copy_buf);

    size_t len = _size_copy_buf / sizeof(uint32_t);

    _copy_memory_new = new uint32_t[len];

    for(size_t i = 0; i < len; ++i)
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