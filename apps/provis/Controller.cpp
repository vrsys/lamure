#include "Controller.h"

Controller::Controller(Scene const &scene, char **argv, int width_window, int height_window, std::string name_file_lod, std::string name_file_dense, lamure::ren::Data_Provenance &data_provenance)
    : _scene(scene)
{
    // initialize context
    scm::shared_ptr<scm::core> scm_core(new scm::core(1, argv));
    // initialize device
    _device.reset(new scm::gl::render_device());

    _scene.init(_device);

    if(_scene.get_vector_point().size() == 0)
    {
        _renderer.dense_points_only = true;
    }

    _renderer.init(argv, _device, width_window, height_window, name_file_lod, name_file_dense, data_provenance);
}

bool Controller::update()
{
    double new_time_since_start = glfwGetTime();
    _time_delta = (new_time_since_start - _time_since_start) * 1000;
    _time_since_start = new_time_since_start;

    time_since_last_brush += _time_delta;
    handle_movements((float)_time_delta);
    _scene.update((float)_time_delta);

    return _renderer.render(_scene);
}

void Controller::handle_movements(float time_delta)
{
    float speed = _renderer._speed * time_delta;
    scm::math::vec3f offset = scm::math::vec3f(0.0f, 0.0f, 0.0f);

    if(_keys[GLFW_KEY_W])
    {
        // std::cout << "pressed w" << std::endl;
        offset += scm::math::vec3f(0.0f, 0.0f, -1.0f) * speed;
    }
    if(_keys[GLFW_KEY_S])
    {
        offset += scm::math::vec3f(0.0f, 0.0f, 1.0f) * speed;
    }
    if(_keys[GLFW_KEY_A])
    {
        offset += scm::math::vec3f(-1.0f, 0.0f, 0.0f) * speed;
    }
    if(_keys[GLFW_KEY_D])
    {
        offset += scm::math::vec3f(1.0f, 0.0f, 0.0f) * speed;
    }
    if(_keys[GLFW_KEY_Q])
    {
        offset += scm::math::vec3f(0.0f, 1.0f, 0.0f) * speed;
    }
    if(_keys[GLFW_KEY_E])
    {
        offset += scm::math::vec3f(0.0f, -1.0f, 0.0f) * speed;
    }

    if(scm::math::length(offset) > 0.0f)
    {
        _renderer._mode_depth_test_surfels_brush = true;
    }

    // if(_keys['h']) // ctrl is pressed
    // {
    //     offset *= 0.25;
    //     offset = scm::math::quat<float>(_renderer.get_camera_view().get_rotation()) * offset;
    //     _renderer.translate_sphere(offset);
    // }
    // else
    {
        if(_keys[GLFW_KEY_G]) // Shift
        {
            offset *= 0.125 * 0.3;
        }
        // std::cout << offset << std::endl;

        if(_renderer._mode_is_ego)
        {
            offset = scm::math::quat<float>(_renderer.get_camera_view().get_rotation()) * offset;
        }
        else
        {
            offset = scm::math::vec3f(0.0f, 0.0f, offset[2]);
            offset = scm::math::quat<float>(_renderer.get_camera_view().get_rotation()) * offset;
        }

        _renderer.get_camera_view().translate(offset);
    }

    if(_keys[GLFW_KEY_K])
    {
        _scene.update_scale_frustum(0.03f * time_delta);
    }
    if(_keys[GLFW_KEY_J])
    {
        _scene.update_scale_frustum(-0.03f * time_delta);
    }

    if(_keys[GLFW_KEY_V])
    {
        _renderer.update_size_point(-0.0004f * time_delta);
    }
    if(_keys[GLFW_KEY_B])
    {
        _renderer.update_size_point(0.0004f * time_delta);
    }

    if(_keys[GLFW_KEY_8])
    {
        _renderer.update_size_pixels_brush(-0.00004f * time_delta);
    }
    if(_keys[GLFW_KEY_9])
    {
        _renderer.update_size_pixels_brush(0.00004f * time_delta);
    }

    float radius_offset = 0.0f;
    if(_keys[GLFW_KEY_X])
    {
        std::cout << radius_offset << std::endl;
        radius_offset += -0.001f * time_delta;
    }
    else if(_keys[GLFW_KEY_C])
    {
        radius_offset += 0.001f * time_delta;
    }

    _renderer.update_radius_sphere(radius_offset);
}
void Controller::handle_mouse_movement(int x, int y)
{
    if(_mode_navigating)
    {
        float xoffset = x - _x_last;
        float yoffset = _y_last - y;

        _x_last = x;
        _y_last = y;

        if(_renderer._mode_is_ego)
        {
            _yaw -= xoffset * _renderer._speed_yaw / 100.0f * _time_delta;
            _pitch -= yoffset * -_renderer._speed_pitch / 100.0f * _time_delta;

            // if(_yaw > 360.0)
            // {
            //     _yaw -= 360.0;
            // }
            // else if(_yaw < 0.0)
            // {
            //     _yaw += 360.0;
            // }

            // _pitch = std::max(_pitch, -89.0f);
            // _pitch = std::min(_pitch, 89.0f);

            scm::math::quat<float> quat_x = scm::math::quat<float>::from_axis(scm::math::deg2rad(_pitch), scm::math::vec3f(1.0, 0.0, 0.0));
            scm::math::quat<float> quat_y = scm::math::quat<float>::from_axis(scm::math::deg2rad(_yaw), scm::math::vec3f(0.0, 1.0, 0.0));
            quat_x = scm::math::normalize(quat_y) * scm::math::normalize(quat_x);
            _renderer.get_camera_view().set_rotation(scm::math::normalize(quat_x));
        }
        else
        {
            scm::math::vec3f position = _renderer.get_camera_view().get_position();

            float angle = (float) scm::math::deg2rad(xoffset * _renderer._speed_yaw / 1000.0f * _time_delta);
            float rotatedX = scm::math::cos(angle) * (position[0] - _renderer._center_non_ego_mode[0]) - scm::math::sin(angle) * (position[2] - _renderer._center_non_ego_mode[2]) +
                             _renderer._center_non_ego_mode[0];
            float rotatedZ = scm::math::sin(angle) * (position[0] - _renderer._center_non_ego_mode[0]) + scm::math::cos(angle) * (position[2] - _renderer._center_non_ego_mode[2]) +
                             _renderer._center_non_ego_mode[2];

            scm::math::vec3f new_position = scm::math::vec3f(rotatedX, position[1], rotatedZ);
            _renderer.get_camera_view().set_position(new_position);

            // scm::math::vec3f position = _renderer.get_camera_view().get_position();

            // float angle = scm::math::deg2rad(yoffset * _renderer._speed_yaw / 1000.0f * _time_delta);
            // float rotatedY = scm::math::sin(angle) * (position[1] - _renderer._center_non_ego_mode[1]) + scm::math::cos(angle) * (position[2] - _renderer._center_non_ego_mode[2]) +
            //                  _renderer._center_non_ego_mode[1];
            // float rotatedZ = scm::math::cos(angle) * (position[1] - _renderer._center_non_ego_mode[1]) - scm::math::sin(angle) * (position[2] - _renderer._center_non_ego_mode[2]) +
            //                  _renderer._center_non_ego_mode[2];

            // scm::math::vec3f new_position = scm::math::vec3f(position[0], rotatedY, rotatedZ);
            // _renderer.get_camera_view().set_position(new_position);
        }
    }

    // std::cout << _mode_brushing << std::endl;
    if(_mode_brushing)
    {
        if(time_since_last_brush >= time_min_between_brush)
        {
            _renderer.start_brushing(x, y, _scene);
            time_since_last_brush = 0;
        }
    }
}

void Controller::handle_mouse_click(int button, int action, int mods, int xpos, int ypos)
{
    // std::cout << button << " " << state << " " << x << " " << y << " " << std::endl;
    if(button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        if(action == GLFW_PRESS && !ImGui::GetIO().WantCaptureMouse)
        {
            _mode_navigating = true;

            _x_last = xpos;
            _y_last = ypos;
        }
        else if(action == GLFW_RELEASE)
        {
            _mode_navigating = false;
        }
    }

    if(button == GLFW_MOUSE_BUTTON_LEFT && !ImGui::GetIO().WantCaptureMouse)
    {
        if(action == GLFW_PRESS)
        {
            _mode_brushing = true;
            if(time_since_last_brush >= time_min_between_brush)
            {
                _renderer.start_brushing(xpos, ypos, _scene);
                time_since_last_brush = 0;
            }
        }
        else if(action == GLFW_RELEASE)
        {
            _mode_brushing = false;
        }
    }
}

void Controller::handle_key_pressed(int key)
{
    // std::cout << key << std::endl;
    _keys[key] = true;
}
void Controller::handle_key_special_pressed(int key) { _keys_special[key] = true; }
void Controller::handle_key_special_released(int key) { _keys_special[key] = false; }
void Controller::handle_key_released(int key)
{
    // std::cout << key << " " << (char)int(key) << std::endl;
    _keys[key] = false;

    if(key == GLFW_KEY_I)
    {
        _renderer.toggle_is_camera_active();
    }
    else if(key == GLFW_KEY_U)
    {
        _renderer.previous_camera(_scene);
    }
    else if(key == GLFW_KEY_O)
    {
        _renderer.next_camera(_scene);
    }
    else if(key == GLFW_KEY_R)
    {
        _renderer.toggle_camera(_scene);
    }
    else if(key == GLFW_KEY_T)
    {
        _renderer.update_state_lense();
    }
    else if(key == GLFW_KEY_1)
    {
        _renderer.mode_prov_data = 0;
    }
    else if(key == GLFW_KEY_2)
    {
        _renderer.mode_prov_data = 1;
    }
    else if(key == GLFW_KEY_3)
    {
        _renderer.mode_prov_data = 2;
    }
    else if(key == GLFW_KEY_0)
    {
        _renderer.mode_prov_data = 3;
    }
    else if(key == GLFW_KEY_P)
    {
        _renderer.dispatch = !_renderer.dispatch;
    }
    else if(key == GLFW_KEY_6)
    {
        _renderer._depth_octree = std::max(0, _renderer._depth_octree - 1);
        _renderer.update_vector_nodes();
        std::cout << "Current Depth: " << _renderer._depth_octree << std::endl;
    }

    else if(key == GLFW_KEY_7)
    {
        _renderer._depth_octree += 1;
        _renderer.update_vector_nodes();
        std::cout << "Current Depth: " << _renderer._depth_octree << std::endl;
    }

    // if(key == GLFW_KEY_W)
    // {
    //     _keys[GLFW_KEY_W] = false;
    // }
    // if(key == GLFW_KEY_S)
    // {
    //     _keys[GLFW_KEY_S] = false;
    // }
    // if(key == GLFW_KEY_A)
    // {
    //     _keys[GLFW_KEY_A] = false;
    // }
    // if(key == 'd' || key == 'D')
    // {
    //     _keys['d'] = false;
    // }
    // if(key == 'q' || key == 'Q')
    // {
    //     _keys['q'] = false;
    // }
    // if(key == 'e' || key == 'E')
    // {
    //     _keys['e'] = false;
    // }
    // if(key == 'x' || key == 'X')
    // {
    //     _keys['x'] = false;
    // }
    // if(key == 'c' || key == 'C')
    // {
    //     _keys['c'] = false;
    // }
}
