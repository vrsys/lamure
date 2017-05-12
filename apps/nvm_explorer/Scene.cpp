#include "Scene.h"

Scene::Scene()
{
}
Scene::Scene(vector<Camera> vector_camera, vector<Point> vector_point, vector<Image> vector_image)
{
	_vector_camera = vector_camera;
	_vector_point = vector_point;
	_vector_image = vector_image;

}

void Scene::init(scm::shared_ptr<scm::gl::render_device> device)
{
    // create buffer for the points
	std::vector<Struct_Point> vector_struct_point = convert_points_to_struct_point();

    scm::gl::buffer_ptr vertex_buffer_object_points = device->create_buffer(scm::gl::BIND_VERTEX_BUFFER,
                                    scm::gl::USAGE_STATIC_DRAW,
                                    (sizeof(float) * 6) * vector_struct_point.size(),
                                    &vector_struct_point[0]);

    _vertex_array_object_points = device->create_vertex_array(scm::gl::vertex_format
        (0, 0, scm::gl::TYPE_VEC3F, sizeof(float) * 6)
        (0, 1, scm::gl::TYPE_VEC3F, sizeof(float) * 6),
        boost::assign::list_of(vertex_buffer_object_points));

    // create buffer for the cameras
	std::vector<Struct_Camera> vector_struct_camera = convert_cameras_to_struct_camera();

    scm::gl::buffer_ptr vertex_buffer_object_cameras = device->create_buffer(scm::gl::BIND_VERTEX_BUFFER,
                                    scm::gl::USAGE_STATIC_DRAW,
                                    (sizeof(double) * 3) * vector_struct_camera.size(),
                                    &vector_struct_camera[0]);

    _vertex_array_object_cameras = device->create_vertex_array(scm::gl::vertex_format
        (0, 0, scm::gl::TYPE_VEC3D, sizeof(double) * 3),
        boost::assign::list_of(vertex_buffer_object_cameras));
    device->main_context()->apply();

	_quad.reset(new scm::gl::quad_geometry(device, vec2f(-1.0f, -1.0f), vec2f(1.0f, 1.0f)));
}

std::vector<Struct_Point> Scene::convert_points_to_struct_point()
{
	std::vector<Struct_Point> vector_struct_point;

    for(std::vector<Point>::iterator it = _vector_point.begin(); it != _vector_point.end(); ++it) 
    {
    	scm::math::vec3f color_normalized = scm::math::vec3f((*it).get_color()) / 255.0;
        Struct_Point point = {(*it).get_center(), color_normalized};

        vector_struct_point.push_back(point);
    }

    return vector_struct_point;
}

std::vector<Struct_Camera> Scene::convert_cameras_to_struct_camera()
{
	std::vector<Struct_Camera> vector_struct_camera;

    for(std::vector<Camera>::iterator it = _vector_camera.begin(); it != _vector_camera.end(); ++it) 
    {
        Struct_Camera camera = {(*it).get_center()};

        vector_struct_camera.push_back(camera);
    }

    return vector_struct_camera;
}

scm::gl::vertex_array_ptr Scene::get_vertex_array_object_points()
{
	return _vertex_array_object_points;
}
scm::gl::vertex_array_ptr Scene::get_vertex_array_object_cameras()
{
	return _vertex_array_object_cameras;
}
scm::shared_ptr<scm::gl::quad_geometry> Scene::get_quad()
{
	return _quad;
}

void Scene::toggle_camera()
{
	is_default_camera = !is_default_camera;
	if(is_default_camera)
	{
		_camera_view.reset();
	} else {
		Camera camera = _vector_camera[index_current_image_camera];
		_camera_view.set_position(scm::math::vec3f(camera.get_center()));
		_camera_view.set_rotation(camera.get_orientation());
	}
}

void Scene::previous_camera()
{
	if(!is_default_camera)
	{
		index_current_image_camera -= 1;
		if(index_current_image_camera == -1)
		{
			index_current_image_camera = _vector_camera.size() - 1;
		}
		Camera camera = _vector_camera[index_current_image_camera];
		_camera_view.set_position(scm::math::vec3f(camera.get_center()));
		_camera_view.set_rotation(camera.get_orientation());
	}
}

void Scene::next_camera()
{
	if(!is_default_camera)
	{
		index_current_image_camera += 1;
		if(index_current_image_camera == _vector_camera.size())
		{
			index_current_image_camera = 0;
		}
		Camera camera = _vector_camera[index_current_image_camera];
		_camera_view.set_position(scm::math::vec3f(camera.get_center()));
		_camera_view.set_rotation(camera.get_orientation());
	}
}

 Camera_View &Scene::get_camera_view()
 {
 	return _camera_view;
 }
 // std::vector<Image> &Scene::get_vector_image()
 // {
 // 	return _vector_image;
 // }
 std::vector<Camera> &Scene::get_vector_camera()
 {
 	return _vector_camera;
 }

 int Scene::count_points()
 {
 	return _vector_point.size();
 }
 int Scene::count_cameras()
 {
 	return _vector_camera.size();
 }
 int Scene::count_images()
 {
 	return _vector_image.size();
 }