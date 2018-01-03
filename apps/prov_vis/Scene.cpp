#include "Scene.h"

Scene::Scene() {}

Scene::Scene(std::vector<lamure::prov::SparsePoint> &vector_point, std::vector<lamure::prov::Camera> &vector_camera) : _vector_point(vector_point)
{
    int counter = 0;
    for(lamure::prov::Camera const &camera : vector_camera)
    {
        if(counter++ == 10)
        {
            // break;
        }

        Camera_Custom camera_new = Camera_Custom(camera);
        _vector_camera.push_back(camera_new);
    }
}

bool Scene::update(float time_delta)
{

}

void Scene::init(scm::shared_ptr<scm::gl::render_device> device, std::string const& image_directory)
{
    // create buffer for the points

    std::vector<Struct_Point> vector_struct_point = convert_points_to_struct_point();
    _vertex_buffer_object_points = device->create_buffer(scm::gl::BIND_VERTEX_BUFFER, scm::gl::USAGE_STATIC_DRAW, (sizeof(float) * 6) * vector_struct_point.size(), &vector_struct_point[0]);
    _vertex_array_object_points = device->create_vertex_array(scm::gl::vertex_format(0, 0, scm::gl::TYPE_VEC3F, sizeof(float) * 6)(0, 1, scm::gl::TYPE_VEC3F, sizeof(float) * 6),
                                                              boost::assign::list_of(_vertex_buffer_object_points));

    // create buffer for the cameras
    std::vector<Struct_Camera> vector_struct_camera = convert_cameras_to_struct_camera();
    _vertex_buffer_object_cameras = device->create_buffer(scm::gl::BIND_VERTEX_BUFFER, scm::gl::USAGE_STATIC_DRAW, (sizeof(float) * 3) * vector_struct_camera.size(), &vector_struct_camera[0]);
    _vertex_array_object_cameras = device->create_vertex_array(scm::gl::vertex_format(0, 0, scm::gl::TYPE_VEC3F, sizeof(float) * 3), boost::assign::list_of(_vertex_buffer_object_cameras));

    _quad.reset(new scm::gl::quad_geometry(device, scm::math::vec2f(-1.0f, -1.0f), scm::math::vec2f(1.0f, 1.0f)));

    int counter = 0;

    for(Camera_Custom &camera : _vector_camera)
    {
        camera.init(device, _vector_point, image_directory);
    }

    device->main_context()->apply();
}

std::vector<Struct_Point> Scene::convert_points_to_struct_point()
{
    std::vector<Struct_Point> vector_struct_point;

    for(std::vector<lamure::prov::SparsePoint>::iterator it = _vector_point.begin(); it != _vector_point.end(); ++it)
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

    for(std::vector<Camera_Custom>::iterator it = _vector_camera.begin(); it != _vector_camera.end(); ++it)
    {
        Struct_Camera camera = {scm::math::vec3f((*it).get_translation())};

        vector_struct_camera.push_back(camera);
    }

    return vector_struct_camera;
}

scm::gl::vertex_array_ptr Scene::get_vertex_array_object_points() { return _vertex_array_object_points; }
scm::gl::vertex_array_ptr Scene::get_vertex_array_object_cameras() { return _vertex_array_object_cameras; }
scm::shared_ptr<scm::gl::quad_geometry> Scene::get_quad() { return _quad; }

void Scene::update_scale_frustum(float offset)
{
    for(std::vector<Camera_Custom>::iterator it = _vector_camera.begin(); it != _vector_camera.end(); ++it)
    {
        (*it).update_scale_frustum(offset);
    }
}

std::vector<Camera_Custom> &Scene::get_vector_camera() { return _vector_camera; }
std::vector<lamure::prov::SparsePoint> &Scene::get_vector_point() { return _vector_point; }

int Scene::count_points() { return _vector_point.size(); }
int Scene::count_cameras() { return _vector_camera.size(); }
// int Scene::count_images() { return _vector_image.size(); }
