#include "Controller.h"

Controller::Controller(Scene scene, char** argv)
{
	_scene = scene;
	// initialize context
    scm::shared_ptr<scm::core> scm_core(new scm::core(1, argv));
    // initialize device
    _device.reset(new scm::gl::render_device());

	_renderer.init(argv, _device);
	_scene.init(_device);
}

bool Controller::update()
{
	_renderer.render(_scene);

	return false;
}