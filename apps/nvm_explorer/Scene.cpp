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