#include "Controller.h"

Controller::Controller(Scene scene)
{
	_scene = scene;
	_renderer.init();
}

bool Controller::update()
{
	_renderer.render();

	return false;
}