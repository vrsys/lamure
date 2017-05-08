#include "Renderer.h"


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
	scm::math::vec3f center = scm::math::vec3f(0.0f, 0.0f, 0.0f);
    scm::math::mat4 reset_matrix = scm::math::make_look_at_matrix(center+scm::math::vec3f(0.f, 0.1f,10.01f), center, scm::math::vec3f(0.f, 1.f,0.f));
    float reset_diameter = 10;

	_active_camera = new lamure::ren::camera(0, reset_matrix, reset_diameter, false, false);
}

void Renderer::init(char** argv)
{

	// initialize context
    scm::shared_ptr<scm::core> scm_core(new scm::core(1, argv));
    // initialize device
    _device.reset(new scm::gl::render_device());
    // get main/default context from device
    _context = _device->main_context();

    // load shaders
    std::string root_path = LAMURE_SHADERS_DIR;
    std::string visibility_vs_source;
    std::string visibility_fs_source;
    scm::io::read_text_file(root_path +  "/nvm_explorer_vertex.glslv", visibility_vs_source);
    scm::io::read_text_file(root_path + "/nvm_explorer_fragment.glslf", visibility_fs_source);

    // create shader program
    _program = _device->create_program(
		boost::assign::list_of(_device->create_shader(scm::gl::STAGE_VERTEX_SHADER, visibility_vs_source))
		(_device->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, visibility_fs_source))
	);

    no_backface_culling_rasterizer_state_ = _device->create_rasterizer_state(scm::gl::FILL_SOLID, scm::gl::CULL_NONE, scm::gl::ORIENT_CCW, false, false, 0.0, false, false, scm::gl::point_raster_state(true));

    std::vector<float> vec;
    vec.push_back(0.0);
    vec.push_back(0.0);
    vec.push_back(0.0);

    vec.push_back(0.5);
    vec.push_back(0.5);
    vec.push_back(0.0);

    vec.push_back(-0.5);
    vec.push_back(-0.5);
    vec.push_back(0.0);

    _buffer = _device->create_buffer(scm::gl::BIND_VERTEX_BUFFER,
                                    scm::gl::USAGE_STATIC_DRAW,
                                    sizeof(float) * 3 * 3,
                                    &vec[0]);

    _pcl_memory = _device->create_vertex_array(scm::gl::vertex_format
        (0, 0, scm::gl::TYPE_VEC3F, sizeof(float) * 3),
        boost::assign::list_of(_buffer));

    _device->main_context()->apply();

}

void Renderer::render(Scene scene) 
{

    // _program->uniform("matrix_view", near_plane_);
    // _program->uniform("matrix_perspective", far_plane_);
      _context->set_rasterizer_state(no_backface_culling_rasterizer_state_);

	_context->clear_default_depth_stencil_buffer();
	_context->clear_default_color_buffer(scm::gl::FRAMEBUFFER_BACK, vec4f(0.0f, 0.0f, 0.0f, 1.0f));
  
	_context->set_default_frame_buffer();

    _context->bind_program(_program);

    _context->bind_vertex_array(_pcl_memory);
    _context->apply();


    _context->draw_arrays(scm::gl::PRIMITIVE_POINT_LIST, 0, 3);

  	_context->set_viewport(scm::gl::viewport(vec2ui(0, 0), 1 * vec2ui(400, 300)));


}

