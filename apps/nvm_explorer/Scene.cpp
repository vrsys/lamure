#include "Scene.h"

// Scene::Scene(const Scene& scene):
// {
//     cache_sparse = scene.cache_sparse;
//     _vertex_array_object_points = scene._vertex_array_object_points;
//     _vertex_array_object_cameras = scene._vertex_array_object_cameras;
//     _vertex_array_object_lines = scene._vertex_array_object_lines;
//     _quad = scene._quad;
//     _camera_view = scene._camera_view;
// }
// Scene::Scene(prov::SparseCache const& cache_sparse) : _cache_sparse(cache_sparse)
// {}
// Scene::Scene(Scene const& scene) : _cache_sparse(scene._cache_sparse)
// {
// }
Scene::Scene(std::vector<prov::SparsePoint> vector_point, std::vector<prov::Camera> vector_camera) : _vector_camera(vector_camera), _vector_point(vector_point) {}

// Scene::Scene(prov::SparseCache const &cache_sparse) : _cache_sparse(cache_sparse)
// {
// }
bool Scene::update(int time_delta)
{
    float foroo = 0.0f;
    // for(std::vector<prov::Camera>::iterator const it = _cache_sparse.get_cameras().begin(); it != _cache_sparse.get_cameras().end(); ++it)
    // {
    //     prov::Camera &camera = (*it);
    //     scm::math::quat<double> old_orientation = camera.get_orientation();
    //     scm::math::quat<double> new_orientation = scm::math::quat<double>::from_axis(0.1 * time_delta, scm::math::vec3d(0.0, 1.0, 0.0));

    //     camera.set_orientation(old_orientation * new_orientation);
    // }
}

void Scene::init(scm::shared_ptr<scm::gl::render_device> device, int width_window, int height_window)
{
    _camera_view.update_window_size(width_window, height_window);
    // create buffer for the points
    std::vector<Struct_Point> vector_struct_point = convert_points_to_struct_point();
    scm::gl::buffer_ptr vertex_buffer_object_points =
        device->create_buffer(scm::gl::BIND_VERTEX_BUFFER, scm::gl::USAGE_STATIC_DRAW, (sizeof(float) * 6) * vector_struct_point.size(), &vector_struct_point[0]);
    _vertex_array_object_points = device->create_vertex_array(scm::gl::vertex_format(0, 0, scm::gl::TYPE_VEC3F, sizeof(float) * 6)(0, 1, scm::gl::TYPE_VEC3F, sizeof(float) * 6),
                                                              boost::assign::list_of(vertex_buffer_object_points));

    // create buffer for the cameras
    std::vector<Struct_Camera> vector_struct_camera = convert_cameras_to_struct_camera();
    scm::gl::buffer_ptr vertex_buffer_object_cameras =
        device->create_buffer(scm::gl::BIND_VERTEX_BUFFER, scm::gl::USAGE_STATIC_DRAW, (sizeof(float) * 3) * vector_struct_camera.size(), &vector_struct_camera[0]);
    _vertex_array_object_cameras = device->create_vertex_array(scm::gl::vertex_format(0, 0, scm::gl::TYPE_VEC3D, sizeof(float) * 3), boost::assign::list_of(vertex_buffer_object_cameras));

    // create buffer for the lines connecting the sparse points with the projection center
    std::vector<Struct_Line> vector_struct_line = convert_lines_to_struct_line();
    scm::gl::buffer_ptr vertex_buffer_object_lines =
        device->create_buffer(scm::gl::BIND_VERTEX_BUFFER, scm::gl::USAGE_STATIC_DRAW, (sizeof(float) * 3) * vector_struct_line.size(), &vector_struct_line[0]);
    _vertex_array_object_lines = device->create_vertex_array(scm::gl::vertex_format(0, 0, scm::gl::TYPE_VEC3F, sizeof(float) * 3), boost::assign::list_of(vertex_buffer_object_lines));

    _quad.reset(new scm::gl::quad_geometry(device, scm::math::vec2f(-1.0f, -1.0f), scm::math::vec2f(1.0f, 1.0f)));

    int counter = 0;
    // for(std::vector<Camera>::iterator it = _vector_camera.begin(); it != _vector_camera.end(); ++it)
    // {
    //     Camera &camera = (*it);
    //     camera.init(device);
    //     counter++;
    //     // if(counter == 20)
    //     // {
    //     //  break;
    //     // }
    // }
    device->main_context()->apply();
}

std::vector<Struct_Line> Scene::convert_lines_to_struct_line()
{
    std::vector<Struct_Line> vector_struct_line;

    // for(std::vector<prov::SparsePoint>::iterator it = _vector_point.begin(); it != _vector_point.end(); ++it)
    // {
    //     prov::SparsePoint &point = (*it);
    //     // std::vector<prov::SparsePoint::measurement> measurements = point.get_measurements();

    // }

    // for(std::vector<prov::Camera>::iterator it = _vector_camera.begin(); it != _vector_camera.end(); ++it)
    // {
    //     Struct_Line line = {scm::math::vec3f((*it).get_center())};
    //     vector_struct_line.push_back(line);
    //     // Struct_Line line1 = {scm::math::vec3f((*it).get_center())+scm::math::vec3f(10.0, 10.0, 10.0)};
    //     line = {scm::math::vec3f(0.0, 0.0, 0.0)};
    //     vector_struct_line.push_back(line);
    //     // vector_struct_line.push_back(line);
    //     // vector_struct_line.push_back(line);
    //     // vector_struct_line.push_back(line);
    //     // vector_struct_line.push_back(line);
    //     // std::cout << (*it).get_center() << std::endl;
    //     break;
    // }
    return vector_struct_line;
}

std::vector<Struct_Point> Scene::convert_points_to_struct_point()
{
    std::vector<Struct_Point> vector_struct_point;

    for(std::vector<prov::SparsePoint>::iterator it = _vector_point.begin(); it != _vector_point.end(); ++it)
    {
        scm::math::vec3f color_normalized = scm::math::vec3f((*it).get_color()) / 255.0;
        Struct_Point point = {scm::math::vec3f((*it).get_position()), color_normalized};

        vector_struct_point.push_back(point);
    }

    return vector_struct_point;
}

std::vector<Struct_Camera> Scene::convert_cameras_to_struct_camera()
{
    std::vector<Struct_Camera> vector_struct_camera;

    for(std::vector<prov::Camera>::iterator it = _vector_camera.begin(); it != _vector_camera.end(); ++it)
    {
        Struct_Camera camera = {scm::math::vec3f((*it).get_translation())};

        vector_struct_camera.push_back(camera);
    }

    return vector_struct_camera;
}

scm::gl::vertex_array_ptr Scene::get_vertex_array_object_points() { return _vertex_array_object_points; }
scm::gl::vertex_array_ptr Scene::get_vertex_array_object_cameras() { return _vertex_array_object_cameras; }
scm::gl::vertex_array_ptr Scene::get_vertex_array_object_lines() { return _vertex_array_object_lines; }
scm::shared_ptr<scm::gl::quad_geometry> Scene::get_quad() { return _quad; }

void Scene::update_scale_frustum(scm::shared_ptr<scm::gl::render_device> device, float offset)
{
    // for(std::vector<prov::Camera>::iterator it = _vector_camera.begin(); it != _vector_camera.end(); ++it)
    // {
    //     (*it).update_scale_frustum(device, offset);
    // }
}

void Scene::update_model_radius_scale(float offset)
{
    float new_scale = std::max(_model_radius_scale + offset, 0.0f);
    _model_radius_scale = new_scale;
}

void Scene::toggle_camera()
{
    is_default_camera = !is_default_camera;
    if(is_default_camera)
    {
        _camera_view.reset();
    }
    else
    {
        // prov::Camera camera = _vector_camera[index_current_image_camera];
        // std::cout << scm::math::vec3f(camera.get_center()) << " " << camera.get_orientation() << std::endl;
        // _camera_view.set_position(scm::math::vec3f(camera.get_center()));
        // _camera_view.set_rotation(camera.get_orientation());
    }
}

void Scene::previous_camera()
{
    if(!is_default_camera)
    {
        index_current_image_camera -= 1;
        if(index_current_image_camera == -1)
        {
            // index_current_image_camera = _vector_camera.size() - 1;
        }
        // prov::Camera camera = _vector_camera[index_current_image_camera];
        // _camera_view.set_position(scm::math::vec3f(camera.get_center()));
        // _camera_view.set_rotation(camera.get_orientation());
    }
}

void Scene::next_camera()
{
    if(!is_default_camera)
    {
        index_current_image_camera += 1;
        // if(index_current_image_camera == _vector_camera.size())
        // {
        //     index_current_image_camera = 0;
        // }
        // prov::Camera camera = _vector_camera[index_current_image_camera];
        // _camera_view.set_position(scm::math::vec3f(camera.get_center()));
        // _camera_view.set_rotation(camera.get_orientation());
    }
}
float &Scene::get_model_radius_scale() { return _model_radius_scale; }
void Scene::set_model_radius_scale(float scale) { _model_radius_scale = scale; }

Camera_View &Scene::get_camera_view() { return _camera_view; }

std::vector<prov::Camera> &Scene::get_vector_camera() { return _vector_camera; }
std::vector<prov::SparsePoint> &Scene::get_vector_point() { return _vector_point; }

int Scene::count_points() { return _vector_point.size(); }
int Scene::count_cameras() { return _vector_camera.size(); }
// int Scene::count_images() { return _vector_image.size(); }
