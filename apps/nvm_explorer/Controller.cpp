#include "Controller.h"

Controller::Controller(Scene scene, char **argv, int width_window, int height_window)
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
    _scene.update(time_delta);
    _renderer.render(_scene);

    return false;
}
void Controller::handle_movements(int time_delta)
{
    float xoffset = 0.0f;
    float yoffset = 0.0f;
    if(_keys[int('4')])
    {
        xoffset += 1;
    }
    if(_keys[int('6')])
    {
        xoffset += -1;
    }
    if(_keys[int('8')])
    {
        yoffset += -1;
    }
    if(_keys[int('5')])
    {
        yoffset += 1;
    }
    scm::math::quat<double> old_rotation = _scene.get_camera_view().get_rotation();
    scm::math::quat<double> new_rotation_x = scm::math::quat<double>::from_axis(xoffset, scm::math::vec3d(0.0, 1.0, 0.0));
    scm::math::quat<double> new_rotation_y = scm::math::quat<double>::from_axis(yoffset, scm::math::vec3d(1.0, 0.0, 0.0));
    scm::math::quat<double> new_rotation = old_rotation * new_rotation_x;
    new_rotation = new_rotation * new_rotation_y;
    _scene.get_camera_view().set_rotation(new_rotation);

    float speed_camera = 0.002f * time_delta;
    scm::math::vec3d offset = scm::math::vec3d(0.0, 0.0, 0.0);
    if(_keys[int('w')])
    {
        offset += scm::math::vec3d(0.0, 0.0, -1.0) * speed_camera;
    }
    if(_keys[int('s')])
    {
        offset += scm::math::vec3d(0.0, 0.0, 1.0) * speed_camera;
    }
    if(_keys[int('a')])
    {
        offset += scm::math::vec3d(-1.0, 0.0, 0.0) * speed_camera;
    }
    if(_keys[int('d')])
    {
        offset += scm::math::vec3d(1.0, 0.0, 0.0) * speed_camera;
    }
    if(_keys[int('q')])
    {
        offset += scm::math::vec3d(0.0, 1.0, 0.0) * speed_camera;
    }
    if(_keys[int('e')])
    {
        offset += scm::math::vec3d(0.0, -1.0, 0.0) * speed_camera;
    }
    offset = _scene.get_camera_view().get_rotation() * offset;
    _scene.get_camera_view().translate(scm::math::vec3f(offset));

    if(_keys[int('k')])
    {
        _scene.update_scale_frustum(_device, 0.5f);
    }
    if(_keys[int('j')])
    {
        _scene.update_scale_frustum(_device, -0.5f);
    }
}
void Controller::handle_mouse_movement(int x, int y)
{
    if(_is_first_mouse_movement)
    {
        _x_last = x;
        _y_last = y;
        _is_first_mouse_movement = false;
    }

    _x_last = 1920 / 2;
    _y_last = 1080 / 2;
    std::cout << x << std::endl;

    float xoffset = x - _x_last;
    float yoffset = _y_last - y;
    _x_last = x;
    _y_last = y;

    float sensitivity = 0.05;
    xoffset *= sensitivity * -1.0;
    yoffset *= sensitivity * -1.0;

    // _yaw += xoffset;
    // _pitch += yoffset;

    // if (_pitch > 89.0f)
    // 	_pitch = 89.0f;
    // if (_pitch < -89.0f)
    // 	_pitch = -89.0f;

    // scm::math::vec3f front;
    // front.x = scm::math::cos(scm::math::deg2rad(_yaw)) * scm::math::cos(scm::math::deg2rad(_pitch));
    // front.y = scm::math::sin(scm::math::deg2rad(_pitch));
    // front.z = scm::math::sin(scm::math::deg2rad(_yaw)) * scm::math::cos(scm::math::deg2rad(_pitch));
    // std::cout << front << std::endl;
    // _scene.get_camera_view().set_rotation(scm::math::quat<double>::from_axis(_pitch, _yaw, 0.0));
    scm::math::quat<double> old_rotation = _scene.get_camera_view().get_rotation();
    scm::math::quat<double> new_rotation = scm::math::quat<double>::from_axis(xoffset, scm::math::vec3d(0.0, 1.0, 0.0));
    // new_rotation = new_rotation * scm::math::quat<double>::from_axis(yoffset, scm::math::vec3d(1.0, 0.0, 0.0));

    new_rotation = old_rotation * new_rotation;

    _scene.get_camera_view().set_rotation(new_rotation);
    // _scene.get_camera_view().set_rotation(scm::math::quat<double>::from_euler(_pitch, _yaw, 0.0));
    // render_manager.direction_camera = glm::normalize(front);
}
void Controller::handle_key_pressed(char key) { _keys[int(key)] = true; }
void Controller::handle_key_released(char key)
{
    // std::cout << key << " " << (char)int(key) << std::endl;
    _keys[int(key)] = false;

    if(key == 'i')
    {
        _scene.toggle_camera();
    }
    else if(key == 'u')
    {
        _scene.previous_camera();
    }
    else if(key == 'o')
    {
        _scene.next_camera();
    }
}
