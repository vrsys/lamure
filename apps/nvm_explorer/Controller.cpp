#include "Controller.h"

Controller::Controller(Scene const &scene, char **argv, int width_window, int height_window, std::string name_file_bvh) : _scene(scene)
{
    // initialize context
    scm::shared_ptr<scm::core> scm_core(new scm::core(1, argv));
    // initialize device
    _device.reset(new scm::gl::render_device());

    _renderer.init(argv, _device, width_window, height_window, name_file_bvh);
    _scene.init(_device);

    //    for(int i = 0; i < 1024; ++i)
    //    {
    //        _keys[i] = false;
    //        _keys_special[i] = false;
    //    };
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
    if(_keys_special[100])
    {
        xoffset += 1;
    }
    if(_keys_special[102])
    {
        xoffset += -1;
    }
    if(_keys_special[101])
    {
        yoffset += -1;
    }
    if(_keys_special[103])
    {
        yoffset += 1;
    }
    scm::math::quat<double> old_rotation = _renderer.get_camera_view().get_rotation();
    scm::math::quat<double> new_rotation_x = scm::math::quat<double>::from_axis(xoffset, scm::math::vec3d(0.0, 1.0, 0.0));
    scm::math::quat<double> new_rotation_y = scm::math::quat<double>::from_axis(yoffset, scm::math::vec3d(1.0, 0.0, 0.0));
    scm::math::quat<double> new_rotation = old_rotation * new_rotation_x;
    new_rotation = new_rotation * new_rotation_y;
    _renderer.get_camera_view().set_rotation(new_rotation);

    float speed = 0.04f * time_delta;
    scm::math::vec3f offset = scm::math::vec3f(0.0f, 0.0f, 0.0f);

    if(_keys['w'] || _keys['W'])
    {
        offset += scm::math::vec3f(0.0f, 0.0f, -1.0f) * speed;
    }
    if(_keys['s'] || _keys['S'])
    {
        offset += scm::math::vec3f(0.0f, 0.0f, 1.0f) * speed;
    }
    if(_keys['a'] || _keys['A'])
    {
        offset += scm::math::vec3f(-1.0f, 0.0f, 0.0f) * speed;
    }
    if(_keys['d'] || _keys['D'])
    {
        offset += scm::math::vec3f(1.0f, 0.0f, 0.0f) * speed;
    }
    if(_keys['q'] || _keys['Q'])
    {
        offset += scm::math::vec3f(0.0f, 1.0f, 0.0f) * speed;
    }
    if(_keys['e'] || _keys['E'])
    {
        offset += scm::math::vec3f(0.0f, -1.0f, 0.0f) * speed;
    }

    if(_keys['h']) // ctrl is pressed
    {
        offset *= 0.25;
        offset = scm::math::quat<float>(_renderer.get_camera_view().get_rotation()) * offset;
        _renderer.translate_sphere(offset);
    }
    else
    {
        if(_keys['g']) // Shift
        {
            offset *= 0.125;
        }
        // std::cout << offset << std::endl;
        offset = scm::math::quat<float>(_renderer.get_camera_view().get_rotation()) * offset;
        _renderer.get_camera_view().translate(offset);
    }

    if(_keys['k'])
    {
        _scene.update_scale_frustum(0.03f * time_delta);
    }
    if(_keys['j'])
    {
        _scene.update_scale_frustum(-0.03f * time_delta);
    }

    if(_keys['v'])
    {
        _renderer.update_size_point(-0.0004f * time_delta);
    }
    if(_keys['b'])
    {
        _renderer.update_size_point(0.0004f * time_delta);
    }

    float radius = 0.0f;
    if(_keys['x'] || _keys['X'])
    {
        std::cout << radius << std::endl;
        radius += -0.001f * time_delta;
    }
    if(_keys['c'] || _keys['C'])
    {
        radius += 0.001f * time_delta;
    }

    _renderer.update_radius_sphere(radius);
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
    //  _pitch = 89.0f;
    // if (_pitch < -89.0f)
    //  _pitch = -89.0f;

    // scm::math::vec3f front;
    // front.x = scm::math::cos(scm::math::deg2rad(_yaw)) * scm::math::cos(scm::math::deg2rad(_pitch));
    // front.y = scm::math::sin(scm::math::deg2rad(_pitch));
    // front.z = scm::math::sin(scm::math::deg2rad(_yaw)) * scm::math::cos(scm::math::deg2rad(_pitch));
    // std::cout << front << std::endl;
    // _renderer.get_camera_view().set_rotation(scm::math::quat<double>::from_axis(_pitch, _yaw, 0.0));
    scm::math::quat<double> old_rotation = _renderer.get_camera_view().get_rotation();
    scm::math::quat<double> new_rotation = scm::math::quat<double>::from_axis(xoffset, scm::math::vec3d(0.0, 1.0, 0.0));
    // new_rotation = new_rotation * scm::math::quat<double>::from_axis(yoffset, scm::math::vec3d(1.0, 0.0, 0.0));

    new_rotation = old_rotation * new_rotation;

    _renderer.get_camera_view().set_rotation(new_rotation);
    // _renderer.get_camera_view().set_rotation(scm::math::quat<double>::from_euler(_pitch, _yaw, 0.0));
    // render_manager.direction_camera = glm::normalize(front);
}

void Controller::handle_key_pressed(char key) { _keys[key] = true; }
void Controller::handle_key_special_pressed(int key) { _keys_special[key] = true; }
void Controller::handle_key_special_released(int key) { _keys_special[key] = false; }
void Controller::handle_key_released(char key)
{
    // std::cout << key << " " << (char)int(key) << std::endl;
    _keys[key] = false;

    if(key == 'i')
    {
        _renderer.toggle_is_camera_active();
    }
    else if(key == 'u')
    {
        _renderer.previous_camera(_scene);
    }
    else if(key == 'o')
    {
        _renderer.next_camera(_scene);
    }
    else if(key == 'r')
    {
        _renderer.toggle_camera(_scene);
    }
    else if(key == 'n')
    {
        _renderer.mode_draw_points_dense = !_renderer.mode_draw_points_dense;
    }
    else if(key == 'm')
    {
        _renderer.mode_draw_images = !_renderer.mode_draw_images;
    }
    else if(key == 'l')
    {
        _renderer.mode_draw_lines = !_renderer.mode_draw_lines;
    }
    else if(key == 't')
    {
        _renderer.update_state_lense();
    }
    else if(key == '1')
    {
        _renderer.mode_prov_data = 0;
    }
    else if(key == '2')
    {
        _renderer.mode_prov_data = 1;
    }
    else if(key == '3')
    {
        _renderer.mode_prov_data = 2;
    }
    else if(key == '0')
    {
        _renderer.mode_prov_data = 3;
    }
    else if(key == 'p')
    {
        _renderer.dispatch = !_renderer.dispatch;
    }

    if(key == 'w' || key == 'W')
    {
        _keys['w'] = false;
        _keys['W'] = false;
    }
    if(key == 's' || key == 'S')
    {
        _keys['s'] = false;
        _keys['S'] = false;
    }
    if(key == 'a' || key == 'A')
    {
        _keys['a'] = false;
        _keys['A'] = false;
    }
    if(key == 'd' || key == 'D')
    {
        _keys['d'] = false;
        _keys['D'] = false;
    }
    if(key == 'q' || key == 'Q')
    {
        _keys['q'] = false;
        _keys['Q'] = false;
    }
    if(key == 'e' || key == 'E')
    {
        _keys['e'] = false;
        _keys['E'] = false;
    }
    if(key == 'x' || key == 'X')
    {
        _keys['x'] = false;
        _keys['X'] = false;
    }
    if(key == 'c' || key == 'C')
    {
        _keys['c'] = false;
        _keys['C'] = false;
    }
}
