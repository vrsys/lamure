#include "Renderer.h"
#include <scm/gl_core/render_device/opengl/gl_core.h>
#include <scm/gl_util/data/imaging/texture_loader.h>
#include <scm/gl_util/data/imaging/texture_loader_dds.h>

// Renderer::Renderer()
// {
//     // wie camera erstellen
//     // wie data in buffer laden

//     // 1.
//     // scm context initialisieren (see main.cpp in lamure_rendering (l:123))

//     // 2. create new scm device (app: renderer.cpp l: 1301)

//     // 3. retrieve rendering context from device (app: renderer.cpp l:1303)

//     // 4. VBO (scm::gl::buffer_ptr) && VAO (scm::gl::vertex_array_ptr) (lib: gpu_access.h l:54&&55; gpu_access.cpp, ll:37)

//     // 5. set states & render:
//     // = renderer.cpp (see render_one_pass_LQ func: ll. 270)
// }

void Renderer::init(char **argv, scm::shared_ptr<scm::gl::render_device> device, int width_window, int height_window, std::string name_file_lod, std::string name_file_dense,
                    lamure::ren::Data_Provenance &data_provenance)
{
    // <<<<<<< HEAD
    //     prov::ifstream in_dense(name_file_dense, std::ios::in | std::ios::binary);
    //     prov::ifstream in_dense_meta(name_file_dense + ".meta", std::ios::in | std::ios::binary);

    //     printf("\nUploading dense points data: start\n");

    //     // prov::DenseCache cache_dense(in_dense, in_dense_meta);
    //     // cache_dense.cache();

    //     // prov::SparseOctree::Builder builder;
    //     // prov::SparseOctree sparse_octree = (builder.from(cache_dense)->with_sort(prov::SparseOctree::PDQ_SORT)->with_max_depth(10)->with_min_per_node(8)->build());
    //     // _sparse_octree = &sparse_octree;
    // =======
    _sparse_octree = prov::SparseOctree::load_tree("tree.prov");
    // >>>>>>> 18079d42e0652b76c02ff4b771c776fced1d7b86
    // _sparse_octree = scm::shared_ptr<prov::SparseOctree>(new prov::SparseOctree(cache_dense));

    _data_provenance = data_provenance;
    _camera_view.update_window_size(width_window, height_window);

    _device = device;
    // get main/default context from device
    _context = device->main_context();

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

    // create shader program for lines
    scm::io::read_text_file(root_path + "/nvm_explorer_vertex_lines.glslv", visibility_vs_source);
    scm::io::read_text_file(root_path + "/nvm_explorer_fragment_lines.glslf", visibility_fs_source);
    _program_lines = device->create_program(
        boost::assign::list_of(device->create_shader(scm::gl::STAGE_VERTEX_SHADER, visibility_vs_source))(device->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, visibility_fs_source)));

    // create shader program for surfels brush
    scm::io::read_text_file(root_path + "/nvm_explorer_vertex_surfels_brush.glslv", visibility_vs_source);
    scm::io::read_text_file(root_path + "/nvm_explorer_geometry_surfels_brush.glslg", visibility_gs_source);
    scm::io::read_text_file(root_path + "/nvm_explorer_fragment_surfels_brush.glslf", visibility_fs_source);
    _program_surfels_brush = device->create_program(boost::assign::list_of(device->create_shader(scm::gl::STAGE_VERTEX_SHADER, visibility_vs_source))(
        device->create_shader(scm::gl::STAGE_GEOMETRY_SHADER, visibility_gs_source))(device->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, visibility_fs_source)));

    // create shader program for pixels brush
    scm::io::read_text_file(root_path + "/nvm_explorer_vertex_pixels_brush.glslv", visibility_vs_source);
    scm::io::read_text_file(root_path + "/nvm_explorer_geometry_pixels_brush.glslg", visibility_gs_source);
    scm::io::read_text_file(root_path + "/nvm_explorer_fragment_pixels_brush.glslf", visibility_fs_source);
    _program_pixels_brush = device->create_program(boost::assign::list_of(device->create_shader(scm::gl::STAGE_VERTEX_SHADER, visibility_vs_source))(
        device->create_shader(scm::gl::STAGE_GEOMETRY_SHADER, visibility_gs_source))(device->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, visibility_fs_source)));

    // create shader program for dense points
    scm::io::read_text_file(root_path + "/lq_one_pass.glslv", visibility_vs_source);
    scm::io::read_text_file(root_path + "/lq_one_pass.glslg", visibility_gs_source);
    scm::io::read_text_file(root_path + "/lq_one_pass.glslf", visibility_fs_source);
    _program_points_dense = device->create_program(boost::assign::list_of(device->create_shader(scm::gl::STAGE_VERTEX_SHADER, visibility_vs_source))(
        device->create_shader(scm::gl::STAGE_GEOMETRY_SHADER, visibility_gs_source))(device->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, visibility_fs_source)));

    // scm::io::read_text_file(root_path + "/nvm_explorer_vertex_points_dense_simple.glslv", visibility_vs_source);
    // scm::io::read_text_file(root_path + "/nvm_explorer_geometry_points_dense.glslg", visibility_gs_source);
    // scm::io::read_text_file(root_path + "/nvm_explorer_fragment_points_dense_simple.glslf", visibility_fs_source);
    // _program_points_dense = device->create_program(boost::assign::list_of(device->create_shader(scm::gl::STAGE_VERTEX_SHADER, visibility_vs_source))
    //                                                // (device->create_shader(scm::gl::STAGE_GEOMETRY_SHADER, visibility_gs_source))
    //                                                (device->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, visibility_fs_source)));
    if(!_program_points_dense)
    {
        std::cout << "error creating shader programs" << std::endl;
    }
    _rasterizer_state = device->create_rasterizer_state(scm::gl::FILL_SOLID, scm::gl::CULL_NONE, scm::gl::ORIENT_CCW, false, false, 0.0, false, false, scm::gl::point_raster_state(true));


    // scm::gl::boxf bb;
    lamure::ren::model_database *database = lamure::ren::model_database::get_instance();
    _model_id = database->add_model(name_file_lod, std::to_string(0));

    depth_state_disable_ = _device->create_depth_stencil_state(false);
    depth_state_enable_ = _device->create_depth_stencil_state(true);

    _vertex_buffer_object_surfels_brush = _device->create_buffer(scm::gl::BIND_VERTEX_BUFFER, scm::gl::USAGE_STATIC_DRAW, (sizeof(float) * 6) * _surfels_brush.size(), &_surfels_brush[0]);
    std::vector<scm::gl::vertex_format::element> vertex_format;
    vertex_format.push_back(scm::gl::vertex_format::element(0, 0, scm::gl::TYPE_VEC3F, sizeof(float) * 3 * 2));
    vertex_format.push_back(scm::gl::vertex_format::element(0, 1, scm::gl::TYPE_VEC3F, sizeof(float) * 3 * 2));
    _vertex_array_object_surfels_brush = _device->create_vertex_array(vertex_format, boost::assign::list_of(_vertex_buffer_object_surfels_brush));

    // color_blending_state_ = device_->create_blend_state(true);
    // color_blending_state_ = device_->create_blend_state(true, FUNC_ONE, FUNC_ONE, FUNC_ONE, FUNC_ONE, EQ_FUNC_ADD, EQ_FUNC_ADD);
    // bb = database->get_model(0)->get_bvh()->get_bounding_boxes()[0];

    std::vector<Struct_Camera> vector_vertices;
    // std::vector<scm::math::vec3f> vector_vertices;

    vector_vertices.push_back({scm::math::vec3f(-0.5f, -0.5f, -0.5f)});
    vector_vertices.push_back({scm::math::vec3f(0.5f, -0.5f, -0.5f)});

    vector_vertices.push_back({scm::math::vec3f(0.5f, -0.5f, -0.5f)});
    vector_vertices.push_back({scm::math::vec3f(0.5f, -0.5f, 0.5f)});

    vector_vertices.push_back({scm::math::vec3f(0.5f, -0.5f, 0.5f)});
    vector_vertices.push_back({scm::math::vec3f(-0.5f, -0.5f, 0.5f)});

    vector_vertices.push_back({scm::math::vec3f(-0.5f, -0.5f, 0.5f)});
    vector_vertices.push_back({scm::math::vec3f(-0.5f, -0.5f, -0.5f)});
    // -----------------------------------------------------------------
    vector_vertices.push_back({scm::math::vec3f(-0.5f, 0.5f, -0.5f)});
    vector_vertices.push_back({scm::math::vec3f(0.5f, 0.5f, -0.5f)});

    vector_vertices.push_back({scm::math::vec3f(0.5f, 0.5f, -0.5f)});
    vector_vertices.push_back({scm::math::vec3f(0.5f, 0.5f, 0.5f)});

    vector_vertices.push_back({scm::math::vec3f(0.5f, 0.5f, 0.5f)});
    vector_vertices.push_back({scm::math::vec3f(-0.5f, 0.5f, 0.5f)});

    vector_vertices.push_back({scm::math::vec3f(-0.5f, 0.5f, 0.5f)});
    vector_vertices.push_back({scm::math::vec3f(-0.5f, 0.5f, -0.5f)});
    // -----------------------------------------------------------------
    vector_vertices.push_back({scm::math::vec3f(-0.5f, -0.5f, -0.5f)});
    vector_vertices.push_back({scm::math::vec3f(-0.5f, 0.5f, -0.5f)});

    vector_vertices.push_back({scm::math::vec3f(0.5f, -0.5f, -0.5f)});
    vector_vertices.push_back({scm::math::vec3f(0.5f, 0.5f, -0.5f)});

    vector_vertices.push_back({scm::math::vec3f(0.5f, -0.5f, 0.5f)});
    vector_vertices.push_back({scm::math::vec3f(0.5f, 0.5f, 0.5f)});

    vector_vertices.push_back({scm::math::vec3f(-0.5f, -0.5f, 0.5f)});
    vector_vertices.push_back({scm::math::vec3f(-0.5f, 0.5f, 0.5f)});

    _vertex_buffer_object_box = _device->create_buffer(scm::gl::BIND_VERTEX_BUFFER, scm::gl::USAGE_STATIC_DRAW, vector_vertices.size() * sizeof(float) * 3, &vector_vertices[0]);

    std::vector<scm::gl::vertex_format::element> vertex_format_box;
    vertex_format_box.push_back(scm::gl::vertex_format::element(0, 0, scm::gl::TYPE_VEC3F, sizeof(float) * 3));
    _vertex_array_object_box = _device->create_vertex_array(vertex_format_box, boost::assign::list_of(_vertex_buffer_object_box));
}

scm::math::vec3f Renderer::convert_to_world_space(float x, float y, float z)
{
    scm::math::mat4f matrix_inverse = scm::math::inverse(_camera_view.get_matrix_perspective() * _camera_view.get_matrix_view());

    float x_normalized = 2.0f * x / _camera_view.get_width_window() - 1;
    float y_normalized = -2.0f * y / _camera_view.get_height_window() + 1;

    scm::math::vec4f point_screen = scm::math::vec4f(x_normalized, y_normalized, z, 1.0f);
    scm::math::vec4f point_world = matrix_inverse * point_screen;

    return scm::math::vec3f(point_world[0] / point_world[3], point_world[1] / point_world[3], point_world[2] / point_world[3]);
}

void Renderer::start_brushing(float x, float y, Scene &scene)
{
    lamure::ren::model_database *database = lamure::ren::model_database::get_instance();

    scm::math::vec3f point_final_front = convert_to_world_space(x, y, -1.0);
    scm::math::vec3f point_final_middle = convert_to_world_space(x, y, 0.0);
    scm::math::vec3f point_final_back = convert_to_world_space(x, y, 1.0);

    float max_distance = 100000.0f;

    scm::math::vec3f direction_ray = point_final_back - point_final_front;

    lamure::ren::ray ray_brush(point_final_front, direction_ray, max_distance);
    scm::math::mat4f model_transform = database->get_model(_model_id)->transform();

    unsigned int max_depth = 255;
    unsigned int surfel_skip = 1;
    lamure::ren::ray::intersection result_intersection;
    ray_brush.intersect_model(_model_id, model_transform, 1.0f, max_depth, surfel_skip, false, result_intersection);

    // std::cout << "position: " << result_intersection.position_ << std::endl;
    // std::cout << "normal: " << result_intersection.normal_ << std::endl;
    // std::cout << "distance: " << result_intersection.distance_ << std::endl;
    // std::cout << "error: " << result_intersection.error_ << std::endl;
    // std::cout << "error_raw: " << result_intersection.error_raw_ << std::endl;

    if(result_intersection.error_raw_ > 0.05)
    {
        return;
    }

    Struct_Surfel_Brush surfel_brush = {result_intersection.position_ + result_intersection.normal_ * 0.1f, result_intersection.normal_};
    add_surfel_brush(result_intersection.position_, surfel_brush, scene);

    // std::vector<Struct_Line> vector_struct_line;
    // Struct_Line position_point = {point_final_front};
    // vector_struct_line.push_back(position_point);

    // Struct_Line position_point1 = {point_final_back};
    // vector_struct_line.push_back(position_point1);

    // _vertex_buffer_object_lines = _device->create_buffer(scm::gl::BIND_VERTEX_BUFFER, scm::gl::USAGE_STATIC_DRAW, (sizeof(float) * 3) * 2, &vector_struct_line[0]);
    // _vertex_array_object_lines = _device->create_vertex_array(scm::gl::vertex_format(0, 0, scm::gl::TYPE_VEC3F, sizeof(float) * 3), boost::assign::list_of(_vertex_buffer_object_lines));
}

void Renderer::reset_surfels_brush(Scene &scene)
{
    _set_seen_cameras.clear();

    _surfels_brush.clear();
    _buffer_surfels_brush_size = 0;

    _vertex_buffer_object_surfels_brush = _device->create_buffer(scm::gl::BIND_VERTEX_BUFFER, scm::gl::USAGE_STATIC_DRAW, (sizeof(float) * 6) * _surfels_brush.size(), &_surfels_brush[0]);
    std::vector<scm::gl::vertex_format::element> vertex_format;
    vertex_format.push_back(scm::gl::vertex_format::element(0, 0, scm::gl::TYPE_VEC3F, sizeof(float) * 3 * 2));
    vertex_format.push_back(scm::gl::vertex_format::element(0, 1, scm::gl::TYPE_VEC3F, sizeof(float) * 3 * 2));
    _vertex_array_object_surfels_brush = _device->create_vertex_array(vertex_format, boost::assign::list_of(_vertex_buffer_object_surfels_brush));

    for(Camera_Custom &camera : scene.get_vector_camera())
    {
        camera.reset_pixels_brush(_device);
    }
}

void Renderer::add_surfel_brush(scm::math::vec3f position, Struct_Surfel_Brush const &surfel_brush, Scene &scene)
{
    // _initial_buffer_size

    // if the next surfel would not fit into the buffer
    // if(_surfels_brush.size() == 0)

    if(_surfels_brush.size() == _buffer_surfels_brush_size)
    {
        _buffer_surfels_brush_size *= 2;

        if(_buffer_surfels_brush_size == 0)
        {
            _buffer_surfels_brush_size = 100;
        }
        // create the new buffer
        scm::gl::buffer_ptr tmp_buffer = _device->create_buffer(scm::gl::BIND_VERTEX_BUFFER, scm::gl::USAGE_STATIC_DRAW, _buffer_surfels_brush_size * (sizeof(float) * 6), 0);
        // copy the data from the old buffer to the new buffer
        _device->main_context()->copy_buffer_data(tmp_buffer, _vertex_buffer_object_surfels_brush, 0, 0, _surfels_brush.size() * (sizeof(float) * 6));

        _vertex_buffer_object_surfels_brush = tmp_buffer;
        tmp_buffer.reset();

        std::vector<scm::gl::vertex_format::element> vertex_format;
        vertex_format.push_back(scm::gl::vertex_format::element(0, 0, scm::gl::TYPE_VEC3F, sizeof(float) * 3 * 2));
        vertex_format.push_back(scm::gl::vertex_format::element(0, 1, scm::gl::TYPE_VEC3F, sizeof(float) * 3 * 2));
        _vertex_array_object_surfels_brush = _device->create_vertex_array(vertex_format, boost::assign::list_of(_vertex_buffer_object_surfels_brush));

        std::cout << "increased buffer (new size: " << _buffer_surfels_brush_size << ")" << std::endl;
    }

    // if(_surfels_brush.size() % threshold_buffer == 0)
    // {
    //     int count_chunks = _surfels_brush.size() / threshold_buffer;
    //     int size_buffer = (count_chunks + 1) * (sizeof(float) * 6) * threshold_buffer;

    // }

    _surfels_brush.push_back(surfel_brush);

    //    std::cout << "1" << std::endl;
    Struct_Surfel_Brush *buffer_mapped = (Struct_Surfel_Brush *)_device->main_context()->map_buffer(_vertex_buffer_object_surfels_brush, scm::gl::ACCESS_READ_WRITE);
    //    std::cout << "2" << std::endl;

    buffer_mapped[_surfels_brush.size() - 1] = surfel_brush;
    //    std::cout << "3" << std::endl;

    _device->main_context()->unmap_buffer(_vertex_buffer_object_surfels_brush);
    //    std::cout << "4" << std::endl;

    // add pixel
    prov::DenseMetaData metadata = search_tree(position, scene);
    // std::vector<uint32_t> vector_ids_cameras = std::vector<uint32_t>(search_tree(position, scene));
    // //    std::cout << "6" << std::endl;

    for(uint32_t id_camera : metadata.get_images_seen())
    {
        _set_seen_cameras.insert(id_camera);
        Camera_Custom &camera = scene.get_vector_camera()[id_camera];
        camera.add_pixel_brush(surfel_brush.position, _device, true);
    }

    for(uint32_t id_camera : metadata.get_images_not_seen())
    {
        _set_seen_cameras.insert(id_camera);
        Camera_Custom &camera = scene.get_vector_camera()[id_camera];
        camera.add_pixel_brush(surfel_brush.position, _device, false);
    }
}

prov::DenseMetaData &Renderer::search_tree(scm::math::vec3f const &surfel_brush, Scene &scene)
{
    prov::OctreeNode *node_ptr = _sparse_octree.lookup_node_at_position(scm::math::vec3f(surfel_brush));
    return node_ptr->get_aggregate_metadata();
}

void Renderer::handle_mouse_movement(float x, float y)
{
    // scm::math::vec3f point_final = convert_to_world_space(x, y, 0.0);
    // std::cout << point_final << std::endl;
}

void Renderer::update_state_lense()
{
    if(_data_provenance.get_size_in_bytes() > 0)
    {
        _state_lense = !_state_lense;
    }
}

void Renderer::translate_sphere(scm::math::vec3f offset) { _position_sphere += offset; }
void Renderer::update_radius_sphere(float offset) { _radius_sphere += offset; }

// void Renderer::translate_sphere_screen(scm::math::vec3f offset) { _position_sphere_screen += scm::math::vec2f(offset[0], -offset[2]); }
// void Renderer::update_radius_sphere_screen(float offset) { _radius_sphere_screen += offset; }
void Renderer::draw_points_sparse(Scene &scene)
{
    _context->bind_program(_program_points);

    _program_points->uniform("matrix_view", _camera_view.get_matrix_view());
    _program_points->uniform("matrix_perspective", _camera_view.get_matrix_perspective());
    _program_points->uniform("size_point", _size_point_sparse);

    _context->bind_vertex_array(scene.get_vertex_array_object_points());
    _context->apply();

    _context->draw_arrays(scm::gl::PRIMITIVE_POINT_LIST, 0, scene.count_points());
}
void Renderer::draw_cameras(Scene &scene)
{
    _context->bind_program(_program_cameras);

    int active_camera = -1;
    if(is_camera_active)
    {
        active_camera = index_current_image_camera;
    }

    _program_cameras->uniform("matrix_view", _camera_view.get_matrix_view());
    _program_cameras->uniform("matrix_perspective", _camera_view.get_matrix_perspective());
    _program_cameras->uniform("active_camera", active_camera);

    _context->bind_vertex_array(scene.get_vertex_array_object_cameras());
    _context->apply();

    // _context->draw_arrays(scm::gl::PRIMITIVE_POINT_LIST, 0, 2);
    _context->draw_arrays(scm::gl::PRIMITIVE_POINT_LIST, 0, scene.count_cameras());
}
void Renderer::draw_images(Scene &scene)
{
    _context->bind_program(_program_images);

    _program_images->uniform("matrix_view", _camera_view.get_matrix_view());
    _program_images->uniform("matrix_perspective", _camera_view.get_matrix_perspective());

    for(Camera_Custom &camera : scene.get_vector_camera())
    {
        if(!is_camera_active || index_current_image_camera == camera.get_index())
        {
            _program_images->uniform("matrix_model", camera.get_transformation_image_plane());
            camera.bind_texture(_context);
            _program_images->uniform_sampler("in_color_texture", 0);
            _context->apply();
            scene.get_quad()->draw(_context);
        }
    }
}
void Renderer::draw_lines_test()
{
    _context->bind_program(_program_lines);

    _program_lines->uniform("matrix_view", _camera_view.get_matrix_view());
    _program_lines->uniform("matrix_perspective", _camera_view.get_matrix_perspective());

    _context->bind_vertex_array(_vertex_array_object_lines);
    _context->apply();

    _context->draw_arrays(scm::gl::PRIMITIVE_LINE_LIST, 0, 2);
}
void Renderer::draw_lines(Scene &scene)
{
    _context->bind_program(_program_lines);

    _program_lines->uniform("matrix_view", _camera_view.get_matrix_view());
    _program_lines->uniform("matrix_perspective", _camera_view.get_matrix_perspective());

    for(Camera_Custom &camera : scene.get_vector_camera())
    {
        if(!is_camera_active || index_current_image_camera == camera.get_index())
        {
            _context->bind_vertex_array(camera.get_vertex_array_object_lines());
            _context->apply();
            _context->draw_arrays(scm::gl::PRIMITIVE_LINE_LIST, 0, (int)(camera.get_count_lines() * this->_line_density / 100.0f));
        }
    }
}
void Renderer::draw_frustra(Scene &scene)
{
    _context->bind_program(_program_frustra);

    _program_frustra->uniform("matrix_view", _camera_view.get_matrix_view());
    _program_frustra->uniform("matrix_perspective", _camera_view.get_matrix_perspective());

    for(std::vector<Camera_Custom>::iterator it = scene.get_vector_camera().begin(); it != scene.get_vector_camera().end(); ++it)
    {
        Camera_Custom camera = (*it);

        if(!is_camera_active || index_current_image_camera == camera.get_index())
        {
            std::set<uint32_t>::iterator it = _set_seen_cameras.find(camera.get_index());
            if(it != _set_seen_cameras.end())
            {
                _program_frustra->uniform("has_pixels", true);
            }
            else
            {
                _program_frustra->uniform("has_pixels", false);
            }

            _program_frustra->uniform("matrix_model", camera.get_transformation());

            _context->bind_vertex_array(camera.get_frustum().get_vertex_array_object());
            _context->apply();

            _context->draw_arrays(scm::gl::PRIMITIVE_LINE_LIST, 0, 24);
        }
    }
}
bool Renderer::draw_points_dense(Scene &scene)
{
    _context->bind_program(_program_points_dense);
    _context->apply();

    lamure::ren::model_database *database = lamure::ren::model_database::get_instance();
    lamure::ren::controller *controller = lamure::ren::controller::get_instance();
    lamure::ren::cut_database *cuts = lamure::ren::cut_database::get_instance();

    // controller->reset_system(_data_provenance);
    lamure::context_t context_id = controller->deduce_context_id(0);

    lamure::model_t m_id = controller->deduce_model_id(std::to_string(0));

    const lamure::ren::bvh *bvh = database->get_model(m_id)->get_bvh();
    scm::math::mat4d model_matrix = scm::math::mat4d(scm::math::make_translation(bvh->get_translation()));

    database->get_model(m_id)->set_transform(scm::math::mat4f(model_matrix));
    cuts->send_transform(context_id, m_id, scm::math::mat4f(model_matrix));
    cuts->send_threshold(context_id, m_id, 1.01f);
    cuts->send_rendered(context_id, m_id);

    // set camera values
    // std::cout << scm::math::mat4d::identity() << std::endl;
    // std::cout << scm::math::mat4d(_camera_view.get_matrix_view()) << std::endl;
    // _camera->set_view_matrix(scm::math::mat4d::identity());

    _camera->set_view_matrix(scm::math::mat4d(_camera_view.get_matrix_view()));
    // std::cout << "this" << std::endl;
    // std::cout << _camera->get_high_precision_view_matrix() << std::endl;
    // std::cout << _camera->get_view_matrix() << std::endl;

    _camera->set_projection_matrix(_camera_view._fov, float(_camera_view.get_width_window()) / float(_camera_view.get_height_window()), _camera_view._plane_near, _camera_view._plane_far);

    lamure::view_t cam_id = controller->deduce_view_id(context_id, _camera->view_id());
    cuts->send_camera(context_id, cam_id, *_camera);

    std::vector<scm::math::vec3d> corner_values = _camera->get_frustum_corners();
    float top_minus_bottom = (float)scm::math::length((corner_values[2]) - (corner_values[0]));
    float height_divided_by_top_minus_bottom = lamure::ren::policy::get_instance()->window_height() / top_minus_bottom;

    cuts->send_height_divided_by_top_minus_bottom(context_id, cam_id, height_divided_by_top_minus_bottom);

    if(dispatch)
        controller->dispatch(context_id, _device, _data_provenance);
    lamure::view_t view_id = controller->deduce_view_id(context_id, _camera->view_id());

    scm::gl::vertex_array_ptr memory = controller->get_context_memory(context_id, lamure::ren::bvh::primitive_type::POINTCLOUD, _device, _data_provenance);
    _context->bind_vertex_array(memory);

    _context->apply();
    lamure::ren::cut &cut = cuts->get_cut(context_id, view_id, 0);
    std::vector<lamure::ren::cut::node_slot_aggregate> renderable = cut.complete_set();

    size_t surfels_per_node = database->get_primitives_per_node();

    _context->apply();

    std::vector<scm::gl::boxf> const &bounding_box_vector = bvh->get_bounding_boxes();
    scm::gl::frustum frustum_by_model = _camera->get_frustum_by_model(scm::math::mat4f(model_matrix));

    scm::math::mat4d projection_matrix = scm::math::mat4d(_camera->get_projection_matrix());
    scm::math::mat4d view_matrix = _camera->get_high_precision_view_matrix();
    scm::math::mat4d model_view_matrix = view_matrix * model_matrix;
    scm::math::mat4d model_view_projection_matrix = projection_matrix * model_view_matrix;

    _program_points_dense->uniform("near_plane", 0.01f);
    _program_points_dense->uniform("far_plane", 1000.0f);
    _program_points_dense->uniform("point_size_factor", 5.0f);
    _program_points_dense->uniform("mvp_matrix", scm::math::mat4f(model_view_projection_matrix));
    _program_points_dense->uniform("model_matrix", scm::math::mat4f(model_matrix));
    _program_points_dense->uniform("model_view_matrix", scm::math::mat4f(model_view_matrix));
    _program_points_dense->uniform("inv_mv_matrix", scm::math::mat4f(scm::math::transpose(scm::math::inverse(model_view_matrix))));
    // std::cout << scm::math::mat4f(scm::math::transpose(scm::math::inverse(model_view_matrix))) << std::endl;
    _program_points_dense->uniform("model_radius_scale", _size_point_dense);
    _program_points_dense->uniform("projection_matrix", scm::math::mat4f(projection_matrix));
    _program_points_dense->uniform("width_window", _camera_view.get_width_window());
    _program_points_dense->uniform("height_window", _camera_view.get_height_window());

    _program_points_dense->uniform("state_lense", _state_lense);
    _program_points_dense->uniform("render_normals", _render_normals);
    _program_points_dense->uniform("radius_sphere", _radius_sphere);
    _program_points_dense->uniform("position_sphere", _position_sphere);
    // _program_points_dense->uniform("radius_sphere_screen", _radius_sphere_screen);
    // _program_points_dense->uniform("position_sphere_screen", _position_sphere_screen);
    _program_points_dense->uniform("mode_prov_data", mode_prov_data);

    _program_points_dense->uniform("heatmap_min", _heatmap_min);
    _program_points_dense->uniform("heatmap_max", _heatmap_max);

    _program_points_dense->uniform("heatmap_min_color", scm::math::vec3f(_heatmap_min_color[0], _heatmap_min_color[1], _heatmap_min_color[2]));
    _program_points_dense->uniform("heatmap_max_color", scm::math::vec3f(_heatmap_max_color[0], _heatmap_max_color[1], _heatmap_max_color[2]));

    // std::cout << mode_prov_data << " " << _state_lense << std::endl;
    // _radius_sphere = 1.0;
    // scm::math::vec3f _position_sphere

    _context->apply();
    int counter = 0;

    for(auto const &node_slot_aggregate : renderable)
    {
        uint32_t node_culling_result = _camera->cull_against_frustum(frustum_by_model, bounding_box_vector[node_slot_aggregate.node_id_]);

        if(node_culling_result != 1)
        {
            _context->draw_arrays(scm::gl::PRIMITIVE_POINT_LIST, (int)(node_slot_aggregate.slot_id_) * (int)surfels_per_node, (int)surfels_per_node);
        }
    }

    return true;
}

void Renderer::update_vector_nodes()
{
    std::queue<prov::OctreeNode *> queue_nodes;
    // std::queue<std::shared_ptr<prov::OctreeNode>> queue_nodes;
    // std::queue<std::shared_ptr<prov::OctreeNode>> queue_nodes;
    // std::cout << 1 << std::endl;
    queue_nodes.push(&_sparse_octree);
    // std::cout << 2 << std::endl;
    _vector_nodes.clear();
    _vector_nodes.reserve((unsigned long)std::pow(8, _depth_octree));
    // std::cout << 3 << std::endl;
    while(!queue_nodes.empty())
    {
        // std::cout << 4 << std::endl;
        prov::OctreeNode *node = queue_nodes.front();
        // std::cout << 5 << std::endl;

        if(node->get_depth() == _depth_octree)
        {
            // std::cout << 6 << std::endl;
            _vector_nodes.push_back(node);
            // std::cout << 7 << std::endl;
        }
        else
        {
            // std::cout << 8 << std::endl;
            // std::vector<prov::OctreeNode> vector_partitions = node->get_partitions();
            // std::cout << 9 << std::endl;
            for(prov::OctreeNode &partition : node->get_partitions())
            {
                // std::cout << 10 << std::endl;
                prov::OctreeNode *node_ptr = &partition;
                queue_nodes.push(node_ptr);
                // queue_nodes.push(std::make_shared<prov::OctreeNode>(partition));
                // std::cout << 11 << std::endl;
            }
        }

        queue_nodes.pop();
    }
}

void Renderer::draw_sparse_octree()
{
    _context->bind_program(_program_frustra);

    _program_frustra->uniform("matrix_view", _camera_view.get_matrix_view());
    _program_frustra->uniform("matrix_perspective", scm::math::mat4f(_camera_view.get_matrix_perspective()));

    for(prov::OctreeNode *node : _vector_nodes)
    {
        scm::math::vec3f vector_bounds = node->get_max() - node->get_min();
        scm::math::mat4f matrix_translation = scm::math::make_translation(scm::math::vec3f(node->get_center()));
        matrix_translation = matrix_translation * scm::math::make_scale(vector_bounds);
        _program_frustra->uniform("matrix_model", matrix_translation);
        // for(std::vector<Camera_Custom>::iterator it = scene.get_vector_camera().begin(); it != scene.get_vector_camera().end(); ++it)
        // {
        //     Camera_Custom camera = (*it);

        //     if(!is_camera_active || index_current_image_camera == camera.get_index())
        //     {
        //         _program_frustra->uniform("matrix_model", camera.get_transformation());
        _context->bind_vertex_array(_vertex_array_object_box);
        _context->apply();

        // _context->draw_arrays(scm::gl::PRIMITIVE_POINT_LIST, 0, 10);
        _context->draw_arrays(scm::gl::PRIMITIVE_LINE_LIST, 0, 24);
        // _context->draw_arrays(scm::gl::PRIMITIVE_LINE_LOOP, 0, 10);
        // _context->draw_arrays(scm::gl::PRIMITIVE_LINE_STRIP, 0, 10);
        //     }
        // }
    }
}

void Renderer::draw_surfels_brush()
{
    lamure::ren::model_database *database = lamure::ren::model_database::get_instance();

    const lamure::ren::bvh *bvh = database->get_model(_model_id)->get_bvh();
    scm::math::mat4f model_matrix = scm::math::mat4f(scm::math::make_translation(scm::math::vec3d()));
    // scm::math::mat4d model_matrix = scm::math::mat4d(scm::math::make_translation(bvh->get_translation()));
    scm::math::mat4f projection_matrix = scm::math::mat4f(_camera_view.get_matrix_perspective());
    scm::math::mat4f view_matrix = _camera_view.get_matrix_view();
    scm::math::mat4f model_view_matrix = view_matrix * model_matrix;
    scm::math::mat4f model_view_projection_matrix = projection_matrix * model_view_matrix;

    _context->bind_program(_program_surfels_brush);

    _program_surfels_brush->uniform("point_size_factor", 0.1f);
    _program_surfels_brush->uniform("mvp_matrix", scm::math::mat4f(model_view_projection_matrix));
    _program_surfels_brush->uniform("model_view_matrix", scm::math::mat4f(model_view_matrix));
    _program_surfels_brush->uniform("color_brush_surfels", scm::math::vec3f(_color_brush_surfels[0], _color_brush_surfels[1], _color_brush_surfels[2]));

    _context->bind_vertex_array(_vertex_array_object_surfels_brush);
    _context->apply();

    _context->draw_arrays(scm::gl::PRIMITIVE_POINT_LIST, 0, (int)_surfels_brush.size());
}

void Renderer::draw_pixels_brush(Scene &scene)
{
    // lamure::ren::model_database *database = lamure::ren::model_database::get_instance();

    // const lamure::ren::bvh *bvh = database->get_model(_model_id)->get_bvh();
    // scm::math::mat4f model_matrix = scm::math::mat4f(scm::math::make_translation(scm::math::vec3d()));
    // // scm::math::mat4d model_matrix = scm::math::mat4d(scm::math::make_translation(bvh->get_translation()));
    // scm::math::mat4f projection_matrix = scm::math::mat4f(_camera_view.get_matrix_perspective());
    // scm::math::mat4f view_matrix = _camera_view.get_matrix_view();
    // scm::math::mat4f model_view_matrix = view_matrix * model_matrix;
    // scm::math::mat4f model_view_projection_matrix = projection_matrix * model_view_matrix;

    _context->bind_program(_program_pixels_brush);

    _program_pixels_brush->uniform("matrix_view", _camera_view.get_matrix_view());
    _program_pixels_brush->uniform("matrix_perspective", _camera_view.get_matrix_perspective());

    // for(std::vector<Camera_Custom>::iterator it = scene.get_vector_camera().begin(); it != scene.get_vector_camera().end(); ++it)
    for(Camera_Custom &camera : scene.get_vector_camera())
    {
        if(!is_camera_active || index_current_image_camera == camera.get_index())
        {
            // if(!_mode_depth_test_surfels_brush && index_current_image_camera == camera.get_index())
            // {
            //     _context->set_depth_stencil_state(depth_state_disable_);
            // }

            _program_pixels_brush->uniform("matrix_model", camera.get_transformation_image_plane());
            // _program_pixels_brush->uniform("scale", 0.5f);
            // _program_pixels_brush->uniform("scale", 1.12f / 1000.0f);
            _program_pixels_brush->uniform("scale", _size_pixels_brush_current);

            _context->bind_vertex_array(camera.get_vertex_array_object_pixels());
            _program_pixels_brush->uniform("seen", true);
            _context->apply();
            _context->draw_arrays(scm::gl::PRIMITIVE_POINT_LIST, 0, (int)camera.get_vector_pixels_brush().size());

            _context->bind_vertex_array(camera.get_vertex_array_object_pixels_not_seen());
            _program_pixels_brush->uniform("seen", false);
            _context->apply();
            _context->draw_arrays(scm::gl::PRIMITIVE_POINT_LIST, 0, (int)camera.get_vector_pixels_not_seen_brush().size());

            _context->set_depth_stencil_state(depth_state_enable_);
        }
    }

    //     _program_surfels_brush->uniform("point_size_factor", 0.1f);
    //     _program_surfels_brush->uniform("mvp_matrix", scm::math::mat4f(model_view_projection_matrix));
    //     _program_surfels_brush->uniform("model_view_matrix", scm::math::mat4f(model_view_matrix));

    //     _context->bind_vertex_array(_vertex_array_object_surfels_brush);
    //     _context->apply();

    //     _context->draw_arrays(scm::gl::PRIMITIVE_POINT_LIST, 0, _surfels_brush.size());
}

bool Renderer::render(Scene &scene)
{
    _camera_view._mode_is_ego = _mode_is_ego;

    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
    _context->set_rasterizer_state(_rasterizer_state);
    _context->set_viewport(scm::gl::viewport(scm::math::vec2f(0, 0), scm::math::vec2f(_camera_view.get_width_window(), _camera_view.get_height_window())));

    _context->clear_default_depth_stencil_buffer();
    _context->clear_default_color_buffer(scm::gl::FRAMEBUFFER_BACK, scm::math::vec4f(0.0f, 0.0f, 0.0f, 1.0f));

    _context->set_default_frame_buffer();

    _context->set_depth_stencil_state(depth_state_enable_);

    if(!dense_points_only)
    {
        if(mode_draw_cameras)
        {
            draw_cameras(scene);
            draw_frustra(scene);
        }
        if(mode_draw_lines)
        {
            draw_lines(scene);
        }
        if(mode_draw_images)
        {
            draw_images(scene);
            draw_pixels_brush(scene);
        }
        if(mode_draw_points_dense)
        {
            if(!draw_points_dense(scene))
            {
                return true;
            }
        }
        else
        {
            draw_points_sparse(scene);
        }
    }
    else
    {
        if(!draw_points_dense(scene))
        {
            return true;
        }
    }

    // if(_mode_depth_test_surfels_brush)
    // {
    //     _context->set_depth_stencil_state(depth_state_disable_);
    // }
    draw_surfels_brush();

    draw_sparse_octree();

    render_menu(scene);

    // draw_lines_test();
    return false;
    // std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
    // auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    // std::cout << duration / 1000.0 << std::endl;
}

void Renderer::render_menu(Scene &scene)
{
    ImGui_ImplGlfwGL3_NewFrame();

    ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiSetCond_Once);

    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiSetCond_Once);

    ImGui::Begin("Settings");
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

    ImGui::Checkbox("Render LoD", &dispatch);
    ImGui::Checkbox("Show dense points", &mode_draw_points_dense);
    if(mode_draw_points_dense)
    {
        ImGui::Checkbox("Show normals", &_render_normals);
    }
    ImGui::Checkbox("Show images", &mode_draw_images);
    ImGui::Checkbox("Show cameras", &mode_draw_cameras);
    ImGui::Checkbox("Show lines", &mode_draw_lines);
    if(mode_draw_lines)
    {
        ImGui::SliderFloat("Line density", &_line_density, 0.0f, 100.0f, "%.4f %%\045", 2.71828381f);
    }
    ImGui::SliderFloat("Pixel size", &_size_pixels_brush_current, _size_pixels_brush_minimum, 1.0f, "%.4f", 2.71828381f);

    // const char *listbox_items = &scene.get_vector_camera()[0];
    // const char* listbox_items[] = { "Apple", "Banana", "Cherry", "Kiwi", "Mango", "Orange", "Pineapple", "Strawberry", "Watermelon" };

    std::vector<std::string> vector_strings;
    const char *listbox_items[scene.get_vector_camera().size()];
    int index = 0;
    for(Camera_Custom &camera : scene.get_vector_camera())
    {
        std::string name_camera = (std::string)camera.get_file_name();
        // std::string name_camera = "camera " + std::to_string(camera.get_index());
        // std::cout << name_camera << std::endl;
        vector_strings.push_back(name_camera);
        // listbox_items[index++] = name_camera.c_str();
    }

    for(std::string const &test : vector_strings)
    {
        listbox_items[index++] = test.c_str();
    }

    ImGui::Checkbox("Show only one camera", &is_camera_active);

    if(is_camera_active)
    {
        ImGui::ListBox("Cameras", &index_current_image_camera, listbox_items, (int)scene.get_vector_camera().size(), 10);
        if(ImGui::Button("Jump to active camera"))
        {
            toggle_camera(scene);
        }
    }

    if(ImGui::CollapsingHeader("Provenance overlay"))
    {
        ImGui::Checkbox("Render provenance data", &_state_lense);

        ImGui::BeginGroup();
        ImGui::RadioButton("Mean absolute deviation", &mode_prov_data, 0);
        ImGui::RadioButton("Standard deviation", &mode_prov_data, 1);
        ImGui::RadioButton("Coefficient of variation [x 0.5]", &mode_prov_data, 2);
        ImGui::RadioButton("Scene matching [DEBUG]", &mode_prov_data, 3);
        ImGui::EndGroup();

        if(ImGui::TreeNode("Heatmap"))
        {
            ImGui::SliderFloat("Heatmap min", &_heatmap_min, 0.0f, 1.0f, "%.3f");
            ImGui::ColorPicker3("Color", (float *)&_heatmap_min_color);
            ImGui::Separator();
            ImGui::SliderFloat("Heatmap max", &_heatmap_max, 0.0f, 1.0f, "%.3f");
            ImGui::ColorPicker3("Color", (float *)&_heatmap_max_color);
            ImGui::TreePop();
        }

        if(ImGui::TreeNode("Brush Surfels"))
        {
            if(ImGui::Button("Clear"))
            {
                reset_surfels_brush(scene);
            }
            ImGui::ColorPicker3("Color", (float *)&_color_brush_surfels);
            ImGui::TreePop();
        }
    }

    if(ImGui::CollapsingHeader("Navigation"))
    {
        // ImGui::Checkbox("Ego mode", &_mode_is_ego);
        // if(!_mode_is_ego)
        // {
        //     ImGui::SliderFloat3("Rotation center", _center_non_ego_mode, 0.0f, 1.0f, "%.2f");
        // }
        ImGui::SliderFloat("Movement speed", &_speed, 0.0f, 0.4f, "%.2f");
        ImGui::SliderFloat("Yaw speed", &_speed_yaw, 0.0f, 100.0f, "%.0f");
        ImGui::SliderFloat("Pitch speed", &_speed_pitch, 0.0f, 100.0f, "%.0f");
    }

    ImGui::End();

    ImGui::Render();
}

void Renderer::update_size_point(float offset)
{
    if(mode_draw_points_dense)
    {
        float new_scale = std::max(_size_point_dense + offset, 0.0f);
        _size_point_dense = new_scale;
    }
    else
    {
        _size_point_sparse_float = std::max(_size_point_sparse_float + offset * 20.0f, 0.0f);
        _size_point_sparse = int(_size_point_sparse_float);
    }
}

void Renderer::update_size_pixels_brush(float scale)
{
    _size_pixels_brush_current += scale;
    _size_pixels_brush_current = std::max(_size_pixels_brush_current, _size_pixels_brush_minimum);
}

void Renderer::toggle_camera(Scene &scene)
{
    is_default_camera = !is_default_camera;
    if(is_default_camera)
    {
        _camera_view.reset();
    }
    else
    {
        Camera_Custom camera = scene.get_vector_camera()[index_current_image_camera];
        _camera_view.set_position(scm::math::vec3f(camera.get_translation()));
        _camera_view.set_rotation(camera.get_orientation());
        _mode_depth_test_surfels_brush = false;
    }
}
void Renderer::toggle_is_camera_active() { is_camera_active = !is_camera_active; }

void Renderer::previous_camera(Scene scene)
{
    if(index_current_image_camera == 0)
    {
        index_current_image_camera = scene.get_vector_camera().size() - 1;
    }
    else
    {
        index_current_image_camera--;
    }
}

void Renderer::next_camera(Scene scene)
{
    index_current_image_camera++;
    if(index_current_image_camera == scene.get_vector_camera().size())
    {
        index_current_image_camera = 0;
    }
}

Camera_View &Renderer::get_camera_view() { return _camera_view; }

// int &Renderer::get_size_point_sparse() { return _size_point_sparse; }
// float &Renderer::get_size_point_dense() { return _size_point_dense; }
