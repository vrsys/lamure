#include "Renderer.h"

#include <scm/gl_util/data/imaging/texture_loader.h>
#include <scm/gl_util/data/imaging/texture_loader_dds.h>

Renderer::Renderer()
{
	// wie camera erstellen
	// wie data in buffer laden

	//1.
	//scm context initialisieren (see main.cpp in lamure_rendering (l:123))

	//2. create new scm device (app: renderer.cpp l: 1301)

	//3. retrieve rendering context from device (app: renderer.cpp l:1303)

	//4. VBO (scm::gl::buffer_ptr) && VAO (scm::gl::vertex_array_ptr) (lib: gpu_access.h l:54&&55; gpu_access.cpp, ll:37)

	//5. set states & render: 
	// = renderer.cpp (see render_one_pass_LQ func: ll. 270)
}

void Renderer::init(char** argv, scm::shared_ptr<scm::gl::render_device> device, int width_window, int height_window)
{
    // get main/default context from device
    _context = device->main_context();
    _width_window = width_window;
    _height_window = height_window;

    // load shaders
    std::string root_path = LAMURE_SHADERS_DIR;
    std::string visibility_vs_source;
    std::string visibility_fs_source;

    // create shader program for points
    scm::io::read_text_file(root_path +  "/nvm_explorer_vertex_points.glslv", visibility_vs_source);
    scm::io::read_text_file(root_path + "/nvm_explorer_fragment_points.glslf", visibility_fs_source);
    _program_points = device->create_program(
		boost::assign::list_of(device->create_shader(scm::gl::STAGE_VERTEX_SHADER, visibility_vs_source))
		(device->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, visibility_fs_source))
	);

    // create shader program for cameras
    scm::io::read_text_file(root_path +  "/nvm_explorer_vertex_cameras.glslv", visibility_vs_source);
    scm::io::read_text_file(root_path + "/nvm_explorer_fragment_cameras.glslf", visibility_fs_source);
    _program_cameras = device->create_program(
        boost::assign::list_of(device->create_shader(scm::gl::STAGE_VERTEX_SHADER, visibility_vs_source))
        (device->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, visibility_fs_source))
    );

    // create shader program for images
    scm::io::read_text_file(root_path +  "/nvm_explorer_vertex_images.glslv", visibility_vs_source);
    scm::io::read_text_file(root_path + "/nvm_explorer_fragment_images.glslf", visibility_fs_source);
    _program_images = device->create_program(
        boost::assign::list_of(device->create_shader(scm::gl::STAGE_VERTEX_SHADER, visibility_vs_source))
        (device->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, visibility_fs_source))
    );

    // create shader program for frustra
    scm::io::read_text_file(root_path +  "/nvm_explorer_vertex_frustra.glslv", visibility_vs_source);
    scm::io::read_text_file(root_path + "/nvm_explorer_fragment_frustra.glslf", visibility_fs_source);
    _program_frustra = device->create_program(
        boost::assign::list_of(device->create_shader(scm::gl::STAGE_VERTEX_SHADER, visibility_vs_source))
        (device->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, visibility_fs_source))
    );

    no_backface_culling_rasterizer_state_ = device->create_rasterizer_state(scm::gl::FILL_SOLID, scm::gl::CULL_NONE, scm::gl::ORIENT_CCW, false, false, 0.0, false, false, scm::gl::point_raster_state(true));
}

void Renderer::render(Scene scene) 
{   

    _context->set_rasterizer_state(no_backface_culling_rasterizer_state_);
    _context->set_viewport(scm::gl::viewport(vec2ui(0, 0), 1 * vec2ui(_width_window, _height_window)));

    _context->clear_default_depth_stencil_buffer();
    _context->clear_default_color_buffer(scm::gl::FRAMEBUFFER_BACK, vec4f(0.0f, 0.0f, 0.0f, 1.0f));
  
    _context->set_default_frame_buffer();

    // draw points
    _context->bind_program(_program_points);

    _program_points->uniform("matrix_view", scene.get_camera_view().get_matrix_view());
    _program_points->uniform("matrix_perspective", scene.get_camera_view().get_matrix_perspective());

    _context->bind_vertex_array(scene.get_vertex_array_object_points());
    _context->apply();

    _context->draw_arrays(scm::gl::PRIMITIVE_POINT_LIST, 0, scene.count_points());

    // draw cameras
    _context->bind_program(_program_cameras);

    _program_cameras->uniform("matrix_view", scene.get_camera_view().get_matrix_view());
    _program_cameras->uniform("matrix_perspective", scene.get_camera_view().get_matrix_perspective());

    _context->bind_vertex_array(scene.get_vertex_array_object_cameras());
    _context->apply();

    // _context->draw_arrays(scm::gl::PRIMITIVE_POINT_LIST, 0, 1);
    _context->draw_arrays(scm::gl::PRIMITIVE_POINT_LIST, 0, scene.count_cameras());

    // draw images
    _context->bind_program(_program_images);

    _program_images->uniform("matrix_view", scene.get_camera_view().get_matrix_view());
    _program_images->uniform("matrix_perspective", scene.get_camera_view().get_matrix_perspective());

    for(std::vector<Camera>::iterator it = scene.get_vector_camera().begin(); it != scene.get_vector_camera().end(); ++it) 
    {
        Camera camera = (*it);
        _program_images->uniform("matrix_model", camera.get_still_image().get_transformation());
        camera.bind_texture(_context);
        _program_images->uniform_sampler("in_color_texture", 0);
        _context->apply();
        scene.get_quad()->draw(_context);
    // PRIMITIVE_LINES
    }

    // draw frustra
    _context->bind_program(_program_frustra);

    _program_frustra->uniform("matrix_view", scene.get_camera_view().get_matrix_view());
    _program_frustra->uniform("matrix_perspective", scene.get_camera_view().get_matrix_perspective());

    for(std::vector<Camera>::iterator it = scene.get_vector_camera().begin(); it != scene.get_vector_camera().end(); ++it) 
    {
        Camera camera = (*it);
        _program_frustra->uniform("matrix_model", camera.get_transformation());
        _context->bind_vertex_array(camera.get_frustum().get_vertex_array_object());
        _context->apply();
        _context->draw_arrays(scm::gl::PRIMITIVE_LINE_LIST, 0, 24);
        // _context->draw_arrays(scm::gl::PRIMITIVE_POINT_LIST, 0, 2);
        // _context->draw_arrays(scm::gl::PRIMITIVE_LINE_STRIP, 0, 2);
    }

}

