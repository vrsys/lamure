#include <lamure/vt/ren/VTRenderer.h>
#include <lamure/vt/VTContext.h>
#include <lamure/vt/ren/CutUpdate.h>

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

    _shader_program =
        _device->create_program(boost::assign::list_of(_device->create_shader(scm::gl::STAGE_VERTEX_SHADER, vs_source))(_device->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, fs_source)));

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
    // TODO: size dynamisch rein
    //_physical_texture_dimension = scm::math::vec2ui(_vtcontext->get_size_physical_texture(), _vtcontext->get_size_physical_texture());
    _physical_texture_dimension = _vtcontext->calculate_size_physical_texture();

    initialize_index_texture();
    initialize_physical_texture();
    initialize_feedback();

    _ms_no_cull = _device->create_rasterizer_state(scm::gl::FILL_SOLID, scm::gl::CULL_NONE, scm::gl::ORIENT_CCW, true);
}

void VTRenderer::render()
{
    _render_context->set_viewport(scm::gl::viewport(scm::math::vec2ui(0, 0), 1 * scm::math::vec2ui(_width, _height)));

    scm::math::mat4f view_matrix = _vtcontext->get_event_handler()->get_trackball_manip().transform_matrix();
    scm::math::mat4f model_matrix = scm::math::mat4f::identity();

    // model_matrix = scm::math::make_translation(0.0f, 0.0f, -2.0f);
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


        auto idx = _vtcontext->get_cut_update()->start_reading_idx();

            _render_context->update_sub_texture(_index_texture,
                                                scm::gl::texture_region(scm::math::vec3ui(0, 0, 0),
                                                                        scm::math::vec3ui(_index_texture_dimension, 1)),
                                                0, scm::gl::FORMAT_RGB_8UI,
                                                idx);

        _vtcontext->get_cut_update()->stop_reading_idx();


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
            //memcpy(&_copy_memory[0], data, _copy_buffer_size);
            memcpy(_copy_memory_new, data, _copy_buffer_size);
        }

        _render_context->unmap_buffer(_atomic_feedback_storage_ssbo);

        _render_context->clear_buffer_data(_atomic_feedback_storage_ssbo, scm::gl::FORMAT_R_32UI, 0);
        _cut_update->feedback(_copy_memory_new);
    }
}

void VTRenderer::initialize_index_texture()
{
    int img_size = _index_texture_dimension.x * _index_texture_dimension.y * 3;
    _index_texture = _device->create_texture_2d(_index_texture_dimension, scm::gl::FORMAT_RGB_8UI);
    _indexBuffer = new uint8_t[img_size];

    for(size_t i = 0; i < img_size; ++i){
        _indexBuffer[i] = 0;
    }

    update_index_texture(_indexBuffer);
    _indexBufferReady.store(false);
}

    /*void VTRenderer::update_index_texture(uint8_t *cpu_buffer)
    {
        _render_context->update_sub_texture(_index_texture,
                                            scm::gl::texture_region(scm::math::vec3ui(0, 0, 0),
                                                                    scm::math::vec3ui(_index_texture_dimension, 1)),
                                            0, scm::gl::FORMAT_RGB_8UI,
                                            cpu_buffer);
    }*/

    void VTRenderer::update_index_texture(uint8_t *cpu_buffer)
    {
        std::lock_guard<mutex> lock(_indexBufferLock);
        _indexBuffer = cpu_buffer;
        _indexBufferReady.store(true);
    }

void VTRenderer::update_index_texture(std::vector<uint8_t> const &cpu_buffer)
{
    _render_context->update_sub_texture(_index_texture, scm::gl::texture_region(scm::math::vec3ui(0, 0, 0), scm::math::vec3ui(_index_texture_dimension, 1)), 0, scm::gl::FORMAT_RGB_8UI,
                                        &cpu_buffer[0]);
}

void VTRenderer::initialize_physical_texture()
{
    _physical_texture = _device->create_texture_2d(_physical_texture_dimension * _vtcontext->get_size_tile(), scm::gl::FORMAT_RGBA_8);
    physical_texture_test_layout();
}

void VTRenderer::update_physical_texture_blockwise(char *buffer, uint32_t x, uint32_t y)
{
    _render_context->update_sub_texture(_physical_texture,
                                        scm::gl::texture_region(scm::math::vec3ui(x * _vtcontext->get_size_tile(),
                                                                                  y * _vtcontext->get_size_tile(), 0),
                                                                scm::math::vec3ui(_vtcontext->get_size_tile(),
                                                                                  _vtcontext->get_size_tile(), 1)),
                                        0,
                                        scm::gl::FORMAT_RGBA_8, buffer);
}

void VTRenderer::physical_texture_test_layout() {
    int tilesize = _vtcontext->get_size_tile() * _vtcontext->get_size_tile() * 4;

    std::ifstream is(_vtcontext->get_name_mipmap() + ".data", std::ios::binary | std::ios::ate);
    auto len = is.tellg();
    is.seekg(0);

    std::cout << "file len: " << len << std::endl;

    if(is)
    {
        auto *buffer = new char[tilesize];
        for(unsigned y = 0; y < _physical_texture_dimension.y && is.tellg() < len; ++y)
        {
            for(unsigned x = 0; x < _physical_texture_dimension.x && is.tellg() < len; ++x)
            {
                std::cout << is.tellg() << std::endl;
                is.read(buffer, tilesize);
                //update_physical_texture_blockwise(buffer, x, y);
            }
        }

        delete[] buffer;
    };
}

void VTRenderer::initialize_feedback()
{
    using namespace scm::gl;
    using namespace scm::math;
    _copy_buffer_size = _physical_texture_dimension.x * _physical_texture_dimension.y * size_of_format(scm::gl::FORMAT_R_32UI);

    _atomic_feedback_storage_ssbo = _device->create_buffer(scm::gl::BIND_STORAGE_BUFFER, scm::gl::USAGE_STREAM_COPY, _copy_buffer_size);

    size_t len = _copy_buffer_size / sizeof(uint32_t);

    //_copy_memory = std::vector<uint32_t>(len, 0);
    _copy_memory_new = new uint32_t[len];

    for(size_t i = 0; i < len; ++i){
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