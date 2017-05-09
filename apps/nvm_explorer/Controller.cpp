#include "Controller.h"

Controller::Controller(Scene scene, char** argv, int width_window, int height_window)
{
	_scene = scene;
	// initialize context
    scm::shared_ptr<scm::core> scm_core(new scm::core(1, argv));
    // initialize device
    _device.reset(new scm::gl::render_device());

	_renderer.init(argv, _device, width_window, height_window);
	_scene.init(_device);
}

bool Controller::update(int time_delta)
{
	handle_movements(time_delta);
	_renderer.render(_scene);

	return false;
}
void Controller::handle_movements(int time_delta)
{
	float speed_camera = 0.02f * time_delta;
	scm::math::vec3f offset = scm::math::vec3f(0.0, 0.0, 0.0);
	if(_keys[int('w')])
	{
		offset += scm::math::vec3f(0.0, 0.0, 1.0) * speed_camera;
	}
	if(_keys[int('s')])
	{
		offset += scm::math::vec3f(0.0, 0.0, -1.0) * speed_camera;
	}
	// Why do I have to invert this?
	if(_keys[int('a')])
	{
		offset += scm::math::vec3f(1.0, 0.0, 0.0) * speed_camera;
	}
	// Why do I have to invert this?
	if(_keys[int('d')])
	{
		offset += scm::math::vec3f(-1.0, 0.0, 0.0) * speed_camera;
	}
	if(_keys[int('q')])
	{
		offset += scm::math::vec3f(0.0, 1.0, 0.0) * speed_camera;
	}
	if(_keys[int('e')])
	{
		offset += scm::math::vec3f(0.0, -1.0, 0.0) * speed_camera;
	}
	_scene.get_camera_view().translate(offset);
}
void Controller::handle_mouse_movement(int x, int y)
{
	if(_is_first_mouse_movement)
	{
		_x_last = x;
		_y_last = y;
		_is_first_mouse_movement = false;
	}

	float xoffset = x - _x_last;
	float yoffset = _y_last - y;
	_x_last = x;
	_y_last = y;

	float sensitivity = 0.05;
	xoffset *= sensitivity * -1.0;
	yoffset *= sensitivity * -1.0;

	_yaw += xoffset;
	_pitch += yoffset;

	if (_pitch > 89.0f)
		_pitch = 89.0f;
	if (_pitch < -89.0f)
		_pitch = -89.0f;

	scm::math::vec3f front;
	front.x = scm::math::cos(scm::math::deg2rad(_yaw)) * scm::math::cos(scm::math::deg2rad(_pitch));
	front.y = scm::math::sin(scm::math::deg2rad(_pitch));
	front.z = scm::math::sin(scm::math::deg2rad(_yaw)) * scm::math::cos(scm::math::deg2rad(_pitch));

	_scene.get_camera_view().set_rotation(front);
	// render_manager.direction_camera = glm::normalize(front);
}
void Controller::handle_key_pressed(char key)
{
	_keys[int(key)] = true;
}
void Controller::handle_key_released(char key)
{
	// std::cout << key << " " << (char)int(key) << std::endl;
	_keys[int(key)] = false;
}
