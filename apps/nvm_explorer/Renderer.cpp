#include "Renderer.h"

#include <scm/gl_util/data/imaging/texture_loader.h>
#include <scm/gl_util/data/imaging/texture_loader_dds.h>

Renderer::Renderer()
{
    // wie camera erstellen
    // wie data in buffer laden

    // 1.
    // scm context initialisieren (see main.cpp in lamure_rendering (l:123))

    // 2. create new scm device (app: renderer.cpp l: 1301)

    // 3. retrieve rendering context from device (app: renderer.cpp l:1303)

    // 4. VBO (scm::gl::buffer_ptr) && VAO (scm::gl::vertex_array_ptr) (lib: gpu_access.h l:54&&55; gpu_access.cpp, ll:37)

    // 5. set states & render:
    // = renderer.cpp (see render_one_pass_LQ func: ll. 270)
}

void Renderer::init(char **argv, scm::shared_ptr<scm::gl::render_device> device, int width_window, int height_window)
{
    _device = device;
    // get main/default context from device
    _context = device->main_context();
    _width_window = width_window;
    _height_window = height_window;

    // load shaders
    std::string root_path = LAMURE_SHADERS_DIR;
    std::string visibility_vs_source;
    std::string visibility_gs_source;
    std::string visibility_fs_source;

    // create shader program for sparse points
    scm::io::read_text_file(root_path + "/nvm_explorer_vertex_points.glslv", visibility_vs_source);
    scm::io::read_text_file(root_path + "/nvm_explorer_fragment_points.glslf", visibility_fs_source);
    _program_points = device->create_program(
        boost::assign::list_of(device->create_shader(scm::gl::STAGE_VERTEX_SHADER, visibility_vs_source))(device->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, visibility_fs_source)));

    // create shader program for cameras
    scm::io::read_text_file(root_path + "/nvm_explorer_vertex_cameras.glslv", visibility_vs_source);
    scm::io::read_text_file(root_path + "/nvm_explorer_fragment_cameras.glslf", visibility_fs_source);
    _program_cameras = device->create_program(
        boost::assign::list_of(device->create_shader(scm::gl::STAGE_VERTEX_SHADER, visibility_vs_source))(device->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, visibility_fs_source)));

    // create shader program for images
    scm::io::read_text_file(root_path + "/nvm_explorer_vertex_images.glslv", visibility_vs_source);
    scm::io::read_text_file(root_path + "/nvm_explorer_fragment_images.glslf", visibility_fs_source);
    _program_images = device->create_program(
        boost::assign::list_of(device->create_shader(scm::gl::STAGE_VERTEX_SHADER, visibility_vs_source))(device->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, visibility_fs_source)));

    // create shader program for frustra
    scm::io::read_text_file(root_path + "/nvm_explorer_vertex_frustra.glslv", visibility_vs_source);
    scm::io::read_text_file(root_path + "/nvm_explorer_fragment_frustra.glslf", visibility_fs_source);
    _program_frustra = device->create_program(
        boost::assign::list_of(device->create_shader(scm::gl::STAGE_VERTEX_SHADER, visibility_vs_source))(device->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, visibility_fs_source)));

    // create shader program for dense points
    scm::io::read_text_file(root_path + "/nvm_explorer_vertex_points_dense_simple.glslv", visibility_vs_source);
    scm::io::read_text_file(root_path + "/nvm_explorer_geometry_points_dense.glslg", visibility_gs_source);
    scm::io::read_text_file(root_path + "/nvm_explorer_fragment_points_dense_simple.glslf", visibility_fs_source);
    _program_points_dense = device->create_program(boost::assign::list_of(device->create_shader(scm::gl::STAGE_VERTEX_SHADER, visibility_vs_source))
                                                   // (device->create_shader(scm::gl::STAGE_GEOMETRY_SHADER, visibility_gs_source))
                                                   (device->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, visibility_fs_source)));
    if(!_program_points_dense)
    {
        std::cout << "error creating shader programs" << std::endl;
    }
    _rasterizer_state = device->create_rasterizer_state(scm::gl::FILL_SOLID, scm::gl::CULL_NONE, scm::gl::ORIENT_CCW, false, false, 0.0, false, false, scm::gl::point_raster_state(true));

    lamure::ren::policy *policy = lamure::ren::policy::get_instance();
    policy->set_max_upload_budget_in_mb(64);
    policy->set_render_budget_in_mb(1024 * 6);
    policy->set_out_of_core_budget_in_mb(1024 * 5);
    policy->set_window_width(_width_window);
    policy->set_window_height(_height_window);

    // scm::gl::boxf bb;
    lamure::ren::model_database *database = lamure::ren::model_database::get_instance();
    lamure::model_t model_id = database->add_model("/home/yiro4618/project_provenance/project/MVS.0.bvh", std::to_string(0));
    // bb = database->get_model(0)->get_bvh()->get_bounding_boxes()[0];
}

void Renderer::draw_points_sparse(Scene scene)
{
    _context->bind_program(_program_points);

    _program_points->uniform("matrix_view", scene.get_camera_view().get_matrix_view());
    _program_points->uniform("matrix_perspective", scene.get_camera_view().get_matrix_perspective());

    _context->bind_vertex_array(scene.get_vertex_array_object_points());
    _context->apply();

    _context->draw_arrays(scm::gl::PRIMITIVE_POINT_LIST, 0, scene.count_points());
}
void Renderer::draw_cameras(Scene scene)
{
    _context->bind_program(_program_cameras);

    _program_cameras->uniform("matrix_view", scene.get_camera_view().get_matrix_view());
    _program_cameras->uniform("matrix_perspective", scene.get_camera_view().get_matrix_perspective());

    _context->bind_vertex_array(scene.get_vertex_array_object_cameras());
    _context->apply();

    _context->draw_arrays(scm::gl::PRIMITIVE_POINT_LIST, 0, scene.count_cameras());
}
void Renderer::draw_images(Scene scene)
{
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
    }
}
void Renderer::draw_frustra(Scene scene)
{
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
    }
}
void Renderer::draw_points_dense(Scene scene)
{
    _context->bind_program(_program_points_dense);

    lamure::ren::model_database *database = lamure::ren::model_database::get_instance();
    lamure::ren::controller *controller = lamure::ren::controller::get_instance();
    lamure::ren::cut_database *cuts = lamure::ren::cut_database::get_instance();

    controller->reset_system();
    lamure::context_t context_id = controller->deduce_context_id(0);

    lamure::model_t m_id = controller->deduce_model_id(std::to_string(0));

    cuts->send_transform(context_id, m_id, scm::math::mat4f(scm::math::mat4d::identity()));
    cuts->send_threshold(context_id, m_id, 1.01f);
    cuts->send_rendered(context_id, m_id);

    database->get_model(m_id)->set_transform(scm::math::mat4f(scm::math::mat4d::identity()));

    // set camera values
    _camera->set_view_matrix(scm::math::mat4d(scene.get_camera_view().get_matrix_view()));
    _camera->set_projection_matrix(60.0f, 1920.0f / 1080.0f, 0.0001f, 1000.0f);

    lamure::view_t cam_id = controller->deduce_view_id(context_id, _camera->view_id());
    cuts->send_camera(context_id, cam_id, *_camera);

    std::vector<scm::math::vec3d> corner_values = _camera->get_frustum_corners();
    double top_minus_bottom = scm::math::length((corner_values[2]) - (corner_values[0]));
    float height_divided_by_top_minus_bottom = lamure::ren::policy::get_instance()->window_height() / top_minus_bottom;
    // std::cout << height_divided_by_top_minus_bottom << std::endl;

    cuts->send_height_divided_by_top_minus_bottom(context_id, cam_id, height_divided_by_top_minus_bottom);

    controller->dispatch(context_id, _device);

    lamure::view_t view_id = controller->deduce_view_id(context_id, _camera->view_id());

    _context->bind_vertex_array(controller->get_context_memory(context_id, lamure::ren::bvh::primitive_type::POINTCLOUD, _device));

    _context->apply();

    lamure::ren::cut &cut = cuts->get_cut(context_id, view_id, 0);
    std::vector<lamure::ren::cut::node_slot_aggregate> renderable = cut.complete_set();

    size_t surfels_per_node = database->get_primitives_per_node();

    _context->apply();

    // std::cout << surfels_per_node << std::endl;
    // std::cout << renderable.size() << std::endl;

    const lamure::ren::bvh *bvh = database->get_model(0)->get_bvh();
    std::vector<scm::gl::boxf> const &bounding_box_vector = bvh->get_bounding_boxes();
    scm::gl::frustum frustum_by_model = _camera->get_frustum_by_model(scm::math::mat4f::identity());

    scm::math::mat4d model_matrix = scm::math::mat4d(scm::math::make_translation(bvh->get_translation()));
    // scm::math::mat4d model_matrix = scm::math::mat4d::identity();
    scm::math::mat4d projection_matrix = scm::math::mat4d(_camera->get_projection_matrix());
    scm::math::mat4d view_matrix = _camera->get_high_precision_view_matrix();
    scm::math::mat4d model_view_matrix = view_matrix * model_matrix;
    scm::math::mat4d model_view_projection_matrix = projection_matrix * model_view_matrix;

    // std::cout << model_view_projection_matrix << std::endl;
    _program_points_dense->uniform("near_plane", 0.01f);
    _program_points_dense->uniform("far_plane", 1000.0f);
    _program_points_dense->uniform("point_size_factor", 5);
    _program_points_dense->uniform("mvp_matrix", scm::math::mat4f(model_view_projection_matrix));
    _program_points_dense->uniform("model_matrix", scm::math::mat4f(model_matrix));
    _program_points_dense->uniform("model_view_matrix", scm::math::mat4f(model_view_matrix));
    _program_points_dense->uniform("inv_mv_matrix", scm::math::mat4f(scm::math::transpose(scm::math::inverse(model_view_matrix))));
    _program_points_dense->uniform("model_radius_scale", 1.f);
    _program_points_dense->uniform("projection_matrix", scm::math::mat4f(projection_matrix));

    for(auto const &node_slot_aggregate : renderable)
    {
        uint32_t node_culling_result = _camera->cull_against_frustum(frustum_by_model, bounding_box_vector[node_slot_aggregate.node_id_]);

        if(node_culling_result != 1)
        {
            _context->draw_arrays(scm::gl::PRIMITIVE_POINT_LIST, (int)(node_slot_aggregate.slot_id_) * (int)surfels_per_node, surfels_per_node);
        }
    }
}

void Renderer::render(Scene scene)
{
    _context->set_rasterizer_state(_rasterizer_state);
    _context->set_viewport(scm::gl::viewport(vec2ui(0, 0), 1 * vec2ui(_width_window, _height_window)));

    _context->clear_default_depth_stencil_buffer();
    _context->clear_default_color_buffer(scm::gl::FRAMEBUFFER_BACK, vec4f(0.0f, 0.0f, 0.0f, 1.0f));

    _context->set_default_frame_buffer();

    draw_points_sparse(scene);
    draw_cameras(scene);
    draw_frustra(scene);
    if(mode_draw_images)
    {
        draw_images(scene);
    }
    if(mode_draw_points_dense)
    {
        draw_points_dense(scene);
    }
}
