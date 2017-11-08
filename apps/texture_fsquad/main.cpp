#include <iostream>
#include <stdexcept>

#include <boost/assign/list_of.hpp>

// scism shit
#include <scm/core.h>
#include <scm/log.h>
#include <scm/core/pointer_types.h>
#include <scm/core/io/tools.h>
#include <scm/core/time/accum_timer.h>
#include <scm/core/time/high_res_timer.h>

#include <scm/gl_core.h>

#include <scm/gl_util/data/imaging/texture_loader.h>
#include <scm/gl_util/manipulators/trackball_manipulator.h>
#include <scm/gl_util/primitives/box.h>
#include <scm/gl_util/primitives/quad.h>
#include <scm/gl_util/primitives/wavefront_obj.h>

// Window library
#include <GL/freeglut.h>

// window sizes
static int winx = 1600;
static int winy = 1024;

static const std::string vs_path      = "../../apps/texture_fsquad/shaders/phong_lighting.glslv";
static const std::string fs_path      = "../../apps/texture_fsquad/shaders/phong_lighting.glslf";
static const std::string obj_path     = "../../apps/texture_fsquad/geometry/box.obj";
static const std::string texture_path = "../../apps/texture_fsquad/textures/0001MM_diff.jpg";
// static const std::string texture_path = "../../apps/texture_fsquad/textures/cat-full.png";

// GL context variables
scm::shared_ptr<scm::gl::render_device>     _device;
scm::shared_ptr<scm::gl::render_context>    _context;

scm::gl::program_ptr                        _shader_program;

scm::gl::buffer_ptr                         _index_buffer;
scm::gl::vertex_array_ptr                   _vertex_array;

scm::math::mat4f                            _projection_matrix;

scm::shared_ptr<scm::gl::box_geometry>      _box;
scm::shared_ptr<scm::gl::wavefront_obj_geometry>  _obj;

//////////////////////////
scm::gl::trackball_manipulator _trackball_manip;

float _initx = 0;
float _inity = 0;

bool _lb_down = false;
bool _mb_down = false;
bool _rb_down = false;

float _dolly_sens = 10.0f;

scm::gl::depth_stencil_state_ptr     _dstate_less;
scm::gl::depth_stencil_state_ptr     _dstate_disable;

scm::gl::blend_state_ptr            _no_blend;
scm::gl::blend_state_ptr            _blend_omsa;
scm::gl::blend_state_ptr            _color_mask_green;

scm::gl::texture_2d_ptr             _color_texture;

scm::gl::sampler_state_ptr          _filter_lin_mip;
scm::gl::sampler_state_ptr          _filter_aniso;
scm::gl::sampler_state_ptr          _filter_nearest;
scm::gl::sampler_state_ptr          _filter_linear;

scm::gl::texture_2d_ptr             _color_buffer;
scm::gl::texture_2d_ptr             _color_buffer_resolved;
scm::gl::texture_2d_ptr             _depth_buffer;
scm::gl::frame_buffer_ptr           _framebuffer;
scm::gl::frame_buffer_ptr           _framebuffer_resolved;
scm::shared_ptr<scm::gl::quad_geometry>  _quad;
scm::gl::program_ptr                _pass_through_shader;
scm::gl::depth_stencil_state_ptr    _depth_no_z;
scm::gl::rasterizer_state_ptr       _ms_back_cull;

void initialize() {
    using namespace scm;
    using namespace scm::gl;
    using namespace scm::math;
    using boost::assign::list_of;

    ////////////////////////////////////////////////////////////////////////////
    // Load Shader files

    std::string vs_source; // Vertex Shader
    std::string fs_source; // Fragment Shader

    // load shader files
    if(!scm::io::read_text_file(vs_path, vs_source) || !scm::io::read_text_file(fs_path, fs_source)) {
        throw std::invalid_argument("error while reading shader files");
    }

    _device.reset(new scm::gl::render_device());

    _context        = _device -> main_context();
    _shader_program = _device -> create_program(list_of
        (_device -> create_shader(STAGE_VERTEX_SHADER, vs_source))
        (_device -> create_shader(STAGE_FRAGMENT_SHADER, fs_source))
        );

    // Check if shader program was successfully created
    if(!_shader_program) {
        throw std::runtime_error("Error creating shader program");
    }

    ////////////////////////////////////////////////////////////////////////////
    // Set lightning

    // lightning constants
    const scm::math::vec3f diffuse(0.7f, 0.7f, 0.7f);
    const scm::math::vec3f specular(0.2f, 0.7f, 0.9f);
    const scm::math::vec3f ambient(0.1f, 0.1f, 0.1f);
    const scm::math::vec3f position(1, 1, 1);

    // set light parameters
    _shader_program->uniform("light_ambient", ambient);
    _shader_program->uniform("light_diffuse", diffuse);
    _shader_program->uniform("light_specular", specular);
    _shader_program->uniform("light_position", position);

    _shader_program->uniform("material_ambient", ambient);
    _shader_program->uniform("material_diffuse", diffuse);
    _shader_program->uniform("material_specular", specular);
    _shader_program->uniform("material_shininess", 128.0f);
    _shader_program->uniform("material_opacity", 1.0f);

    ////////////////////////////////////////////////////////////////////////////
    // Some stuff I don't know jet clearly

    // new exciting stuff
    std::vector<scm::math::vec3f> positions_normals;
    std::vector<unsigned short>   indices;

    // fill normals with (useful) data?!
    positions_normals.push_back(scm::math::vec3f(0.0f, 0.0f, 0.0f));
    positions_normals.push_back(scm::math::vec3f(0.0f, 0.0f, 1.0f));

    positions_normals.push_back(scm::math::vec3f(1.0f, 0.0f, 0.0f));
    positions_normals.push_back(scm::math::vec3f(0.0f, 0.0f, 1.0f));

    positions_normals.push_back(scm::math::vec3f(1.0f, 1.0f, 0.0f));
    positions_normals.push_back(scm::math::vec3f(0.0f, 0.0f, 1.0f));

    positions_normals.push_back(scm::math::vec3f(0.0f, 1.0f, 0.0f));
    positions_normals.push_back(scm::math::vec3f(0.0f, 0.0f, 1.0f));

    // point to the normals in the vertex buffer
    indices.push_back(0);
    indices.push_back(1);
    indices.push_back(2);
    indices.push_back(0);
    indices.push_back(2);
    indices.push_back(3);

    buffer_ptr positions_normals_buf;
    positions_normals_buf = _device->create_buffer(
        BIND_VERTEX_BUFFER,
        USAGE_STATIC_DRAW,
        positions_normals.size() * sizeof(scm::math::vec3f),
        &positions_normals.front()
        );

    _index_buffer = _device->create_buffer(
        BIND_INDEX_BUFFER,
        USAGE_STATIC_DRAW,
        indices.size() * sizeof(unsigned short),
        &indices.front()
        );

    _vertex_array = _device->create_vertex_array(
        vertex_format(0, 0, TYPE_VEC3F, 2 * sizeof(scm::math::vec3f))
                     (0, 1, TYPE_VEC3F, 2 * sizeof(scm::math::vec3f)),
        list_of(positions_normals_buf));

    ////////////////////////////////////////////////////////////////////////////
    // Define depth stencil state
    // Depth stencil state controls how the depth buffer and the stencil buffer
    // are used.
    //
    // Depth buffer:
    //   stores for each pixel the z data (floating-point depth)
    //
    // Stencil buffer:
    //   Mask which pixels get saved and which are discarded

    _dstate_less = _device->create_depth_stencil_state(
        true,            // depth test
        true,            // depth mask
        COMPARISON_LESS  // depth func
        );

    depth_stencil_state_desc dstate = _dstate_less->descriptor();
    dstate._depth_test = false;

    _dstate_disable = _device->create_depth_stencil_state(dstate);
    _dstate_disable = _device->create_depth_stencil_state(false);

    ////////////////////////////////////////////////////////////////////////////
    // Define some blend states (only one at a time can be used)
    // Blend states controls how color and alpha values are blended when
    // combining rendered data with existing render target data

    _no_blend = _device->create_blend_state(
        false,     // enabled
        FUNC_ONE,  // src rgb func
        FUNC_ZERO, // dst rgb func
        FUNC_ONE,  // src alpha func
        FUNC_ZERO  // dst alpha func
        );

    _blend_omsa = _device->create_blend_state(
        true,                       // enabled
        FUNC_SRC_ALPHA,             // src rgb func
        FUNC_ONE_MINUS_SRC_ALPHA,   // dst rgb func
        FUNC_ONE,                   // src alpha func
        FUNC_ZERO);                 // dst alpha func

    _color_mask_green   = _device->create_blend_state(
        true,                      // enabled
        FUNC_SRC_ALPHA,            // src rgb func
        FUNC_ONE_MINUS_SRC_ALPHA,  // dst rgb func
        FUNC_ONE,                  // src alpha func
        FUNC_ZERO,                 // dst alpha func
        EQ_FUNC_ADD,               // rgb equation
        EQ_FUNC_ADD,               // alpha equation
        COLOR_GREEN | COLOR_BLUE   // write mask
        );

    ////////////////////////////////////////////////////////////////////////////
    // Create scene objects

    // ONLY _obj will be drawn; here, two ways to create objects

    _box.reset(new box_geometry(_device, vec3f(-0.5f), vec3f(0.5f)));
    _obj.reset(new wavefront_obj_geometry(_device, obj_path));

    ////////////////////////////////////////////////////////////////////////////
    // Load Texture

    texture_loader tex_loader;
    _color_texture = tex_loader.load_texture_2d(
        *_device,     // set device
        texture_path, // image path
        true,         // create mips
        false         // color mips
        );

    _filter_lin_mip = _device->create_sampler_state(FILTER_MIN_MAG_MIP_LINEAR, WRAP_CLAMP_TO_EDGE);
    _filter_aniso   = _device->create_sampler_state(FILTER_ANISOTROPIC, WRAP_CLAMP_TO_EDGE, 16);
    _filter_nearest = _device->create_sampler_state(FILTER_MIN_MAG_NEAREST, WRAP_CLAMP_TO_EDGE);
    _filter_linear  = _device->create_sampler_state(FILTER_MIN_MAG_LINEAR, WRAP_CLAMP_TO_EDGE);

    ////////////////////////////////////////////////////////////////////////////
    // initialize framebuffer

    _color_buffer = _device->create_texture_2d(
        vec2ui(winx, winy) * 1, // size
        FORMAT_RGBA_8,          // format
        1, 1, 8);               // mip levels, array layers, samples

    _depth_buffer = _device->create_texture_2d(
        vec2ui(winx, winy) * 1, // size
        FORMAT_D24,             // format
        1, 1, 8);               // mip levels, array layers, samples

    _framebuffer  = _device->create_frame_buffer();
    _framebuffer->attach_color_buffer(0, _color_buffer);
    _framebuffer->attach_depth_stencil_buffer(_depth_buffer);

    _color_buffer_resolved = _device->create_texture_2d(
        vec2ui(winx, winy) * 1,
        FORMAT_RGBA_8);

    _framebuffer_resolved  = _device->create_frame_buffer();
    _framebuffer_resolved->attach_color_buffer(0, _color_buffer_resolved);

    _quad.reset(new quad_geometry(
        _device,
        vec2f(0.0f, 0.0f), // min vertex
        vec2f(1.0f, 1.0f)  // max vertex
        ));

    _depth_no_z   = _device->create_depth_stencil_state(false, false);
    _ms_back_cull = _device->create_rasterizer_state(FILL_SOLID, CULL_BACK, ORIENT_CCW, true);

    std::string v_pass = "\
        #version 330\n\
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
        #version 330\n\
        \
        in vec2 tex_coord;\
        uniform sampler2D in_texture;\
        layout(location = 0) out vec4 out_color;\
        void main()\
        {\
            out_color = texelFetch(in_texture, ivec2(gl_FragCoord.xy), 0).rgba;\
        }\
        ";

    _pass_through_shader = _device->create_program(list_of
        (_device->create_shader(STAGE_VERTEX_SHADER, v_pass))
        (_device->create_shader(STAGE_FRAGMENT_SHADER, f_pass))
        );

    _trackball_manip.dolly(2.5f);
}

void display() {
    using namespace scm::gl;
    using namespace scm::math;

    // clear the color and depth buffer
    // glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    mat4f    view_matrix         = _trackball_manip.transform_matrix();
    mat4f    model_matrix        = mat4f::identity();
    //scale(model_matrix, 0.01f, 0.01f, 0.01f);
    mat4f    model_view_matrix   = view_matrix * model_matrix;
    mat4f    mv_inv_transpose    = transpose(inverse(model_view_matrix));

    _shader_program->uniform("projection_matrix", _projection_matrix);
    _shader_program->uniform("model_view_matrix", model_view_matrix);
    _shader_program->uniform("model_view_matrix_inverse_transpose", mv_inv_transpose);
    _shader_program->uniform("color_texture_aniso", 0);
    _shader_program->uniform("color_texture_nearest", 1);

    _context->clear_default_color_buffer(
                FRAMEBUFFER_BACK,
                vec4f(.2f, .2f, .2f, 1.0f));
    _context->clear_default_depth_stencil_buffer();

    _context->reset();

    ////////////////////////////////////////////////////////////////////////////
    // multi sample pass

    context_state_objects_guard csg(_context);
    context_texture_units_guard tug(_context);
    context_framebuffer_guard   fbg(_context);

    _context->clear_color_buffer(_framebuffer, 0, vec4f( .2f, .2f, .2f, 1.0f));
    _context->clear_depth_stencil_buffer(_framebuffer);

    _context->set_frame_buffer(_framebuffer);
    _context->set_viewport(viewport(vec2ui(0, 0), 1 * vec2ui(winx, winy)));

    _context->set_depth_stencil_state(_dstate_less);
    _context->set_blend_state(_no_blend);
    _context->set_rasterizer_state(_ms_back_cull);

    _context->bind_program(_shader_program);

    _context->bind_texture(_color_texture, _filter_aniso,   0);
    _context->bind_texture(_color_texture, _filter_nearest, 1);

    _obj->draw(_context);

    ////////////////////////////////////////////////////////////////////////////

    _context->resolve_multi_sample_buffer(_framebuffer, _framebuffer_resolved);
    _context->generate_mipmaps(_color_buffer_resolved);

    _context->reset();

    mat4f pass_mvp = mat4f::identity();
    ortho_matrix(
        pass_mvp,  // matrix
        0.0f,      // left
        1.0f,      // right
        0.0f,      // bottom
        1.0f,      // top
        -1.0f,     // near z
        1.0f       // far z
        );

    _pass_through_shader->uniform_sampler("in_texture", 0);
    _pass_through_shader->uniform("mvp", pass_mvp);

    _context->set_default_frame_buffer();
    _context->set_depth_stencil_state(_depth_no_z);
    _context->set_blend_state(_no_blend);
    _context->bind_program(_pass_through_shader);
    _context->bind_texture(_color_buffer_resolved, _filter_nearest, 0);

    _quad->draw(_context);

    // swap the back and front buffer, so that the drawn stuff can be seen
    glutSwapBuffers();
}

void reshape(int w, int h) {
    // safe the new dimensions
    winx = w;
    winy = h;

    // set the new viewport into which now will be rendered
    _context->set_viewport(
        scm::gl::viewport(
            scm::math::vec2ui(0, 0), // position
            scm::math::vec2ui(w, h)) // dimensions
        );

    scm::math::perspective_matrix(
        _projection_matrix,  // matrix
        60.f,                // fovy
        float(w)/float(h),   // aspect
        0.1f,                // near z
        100.0f               // far z
        );
}

void mousefunc(int button, int state, int x, int y) {
    switch (button) {
        case GLUT_LEFT_BUTTON: {
                _lb_down = (state == GLUT_DOWN) ? true : false;
            }break;
        case GLUT_MIDDLE_BUTTON: {
                _mb_down = (state == GLUT_DOWN) ? true : false;
            }break;
        case GLUT_RIGHT_BUTTON: {
                _rb_down = (state == GLUT_DOWN) ? true : false;
            }break;
    }

    _initx = 2.f * float(x - (winx/2))/float(winx);
    _inity = 2.f * float(winy - y - (winy/2))/float(winy);
}

void mousemotion(int x, int y) {
    float nx = 2.f * float(x - (winx/2))/float(winx);
    float ny = 2.f * float(winy - y - (winy/2))/float(winy);

    //std::cout << "nx " << nx << " ny " << ny << std::endl;

    if (_lb_down) {
        _trackball_manip.rotation(_initx, _inity, nx, ny);
    }
    if (_rb_down) {
        _trackball_manip.dolly(_dolly_sens * (ny - _inity));
    }
    if (_mb_down) {
        _trackball_manip.translation(nx - _initx, ny - _inity);
    }

    _inity = ny;
    _initx = nx;
}

int main(int argc, char** argv) {
    scm::shared_ptr<scm::core>      scm_core(new scm::core(argc, argv));

    // init GLUT and create Window
    glutInit(&argc, argv);
    glutInitContextVersion (4,4);
    glutInitContextProfile(GLUT_CORE_PROFILE);

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA | GLUT_ALPHA | GLUT_MULTISAMPLE);

    // set properties from window
    // glutInitWindowPosition(0, 0);
    glutInitWindowSize(winx, winy);
    glutCreateWindow("First test with GLUT and SCHISM");

    // init GL context
    try {
        initialize();
    } catch (std::exception& e) {
        std::cout << e.what() << std::endl;
        return -1;
    }

    // register callbacks
    glutReshapeFunc(reshape);
    glutDisplayFunc(display);
    glutMouseFunc(mousefunc);
    glutMotionFunc(mousemotion);
    glutIdleFunc(glutPostRedisplay); // glutPosRedisplay already given

    // enter GLUT event processing cycle
    glutMainLoop();
    return 0;
}