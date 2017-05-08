#include "Scene.h"

Scene::Scene()
{
	convert_points_to_struct_point();
}
Scene::Scene(vector<Camera> vector_camera, vector<Point> vector_point, vector<Image> vector_image)
{
	_vector_camera = vector_camera;
	_vector_point = vector_point;
	_vector_image = vector_image;

}

void Scene::init(scm::shared_ptr<scm::gl::render_device> device)
{
	std::vector<Struct_Point> vector_struct_point = convert_points_to_struct_point();

    _vertex_buffer_object = device->create_buffer(scm::gl::BIND_VERTEX_BUFFER,
                                    scm::gl::USAGE_STATIC_DRAW,
                                    sizeof(float) * 3 * vector_struct_point.size(),
                                    &vector_struct_point[0]);

    _vertex_array_object = device->create_vertex_array(scm::gl::vertex_format
        (0, 0, scm::gl::TYPE_VEC3F, sizeof(float) * 3),
        boost::assign::list_of(_vertex_buffer_object));
    device->main_context()->apply();
}

std::vector<Struct_Point> Scene::convert_points_to_struct_point()
{
	std::vector<Struct_Point> vector_struct_point;

    for(std::vector<Point>::iterator it = _vector_point.begin(); it != _vector_point.end(); ++it) 
    {
    	// scm::math::vec3f tmp = (*it).get_center();
    	// tmp[2] = 0.0;
     //    Struct_Point point = {tmp};
        Struct_Point point = {(*it).get_center()};

        vector_struct_point.push_back(point);
    }

    return vector_struct_point;
}

scm::gl::vertex_array_ptr Scene::get_vertex_array_object()
{
	return _vertex_array_object;
}