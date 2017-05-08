#include "Controller.h"

Controller::Controller(Scene scene, char** argv)
{
	_scene = scene;
	_renderer.init(argv);
}

bool Controller::update()
{
	_renderer.render(_scene);

	return false;
}