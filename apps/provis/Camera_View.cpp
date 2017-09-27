#include "Camera_View.h"

Camera_View::Camera_View()
{
    _matrix_perspective = scm::math::make_perspective_matrix(_fov, float(_width_window) / float(_height_window), _plane_near, _plane_far);
    std::cout << "aspect ratio: " << float(_width_window) / float(_height_window) << std::endl;
    // _matrix_view = scm::math::make_look_at_matrix(_position, _position + scm::math::vec3f(0.0, 0.0, -1.0), scm::math::vec3f(0.0, -1.0, 0.0));

    update_matrices();
}
void Camera_View::translate(scm::math::vec3f offset)
{
    _position += offset;
    update_matrices();
}
void Camera_View::update_window_size(float width_window, float height_window)
{
    _width_window = width_window;
    _height_window = height_window;

    _matrix_perspective = scm::math::make_perspective_matrix(_fov, _width_window / _height_window, _plane_near, _plane_far);

    lamure::ren::policy *policy = lamure::ren::policy::get_instance();
    policy->set_window_width((const int32_t) _width_window);
    policy->set_window_height((const int32_t) _height_window);
}
void Camera_View::update_matrices()
{
    scm::math::mat4f matrix_view_new = scm::math::mat4f::identity();

    scm::math::mat4f matrix_translation = scm::math::make_translation(scm::math::vec3f(_position));

    // scm::math::mat4f matrix_rotation = scm::math::mat4f(_rotation.to_matrix());
    // std::cout << _matrix_view[2]<<" " << _matrix_view[6]<< " " << _matrix_view[10] << std::endl;

    // float _yaw = 180.0f;
    // float _pitch = 0.0f;

    // scm::math::quat<double>  quat_pitch = scm::math::quat<double>::from_axis((_pitch), scm::math::vec3d(1.0, 0.0, 0.0));
    // scm::math::quat<double>  quat_yaw = scm::math::quat<double>::from_axis((_yaw), scm::math::vec3d(0.0, 1.0, 0.0));

    // scm::math::quat<double> quat_final = quat_pitch * quat_yaw;
    // scm::math::quat<double> quat_normalized = scm::math::normalize(quat_final);

    scm::math::mat4f matrix_rotation = scm::math::mat4f(_rotation.to_matrix());
    // std::cout << matrix_rotation << std::endl;

    // std::cout << "pitch " << quat_pitch << std::endl;
    // std::cout << "yaw " << quat_yaw << std::endl;

    // scm::math::vec3f front;
    // front.x = scm::math::rad2deg(scm::math::cos(scm::math::deg2rad(_pitch)) * scm::math::sin(scm::math::deg2rad(_yaw)));
    // front.y = scm::math::rad2deg(scm::math::sin(scm::math::deg2rad(_pitch)));
    // front.z = scm::math::rad2deg(scm::math::cos(scm::math::deg2rad(_yaw)) * scm::math::cos(scm::math::deg2rad(_pitch)));
    // std::cout << front << std::endl;

    // glm::vec3 direction(
    // cos(verticalAngle) * sin(horizontalAngle),
    // sin(verticalAngle),
    // cos(verticalAngle) * cos(horizontalAngle)
    // );

    // _matrix_view = scm::math::make_look_at_matrix(_position, _position + scm::math::vec3f(matrix_rotation[2], matrix_rotation[6], matrix_rotation[10]), scm::math::vec3f(0.0, 1.0, 0.0));
    // scm::math::mat4f matrix_rotation = scm::math::mat4f(
    // 1.0f, 0.0f, 0.0f, 0.0f,
    // 0.0f, 1.0f, 0.0f, 0.0f,
    // 0.0f, 0.0f, -1.0f, 0.0f,
    // 0.0f, 0.0f, 0.0f, 1.0f);

    // matrix_view_new = matrix_translation;
    matrix_view_new = matrix_translation * matrix_rotation;

    matrix_view_new = scm::math::inverse(matrix_view_new);

    // scm::math::mat4d matrix_rotation = _rotation.to_matrix();
    // std::cout << matrix_rotation[2]<<" " << matrix_rotation[6]<< " " << matrix_rotation[10] << "      " <<matrix_view_new[2]<<" " << matrix_view_new[6]<< " " << matrix_view_new[10] << std::endl;
    // std::cout << -matrix_view_new[2]<<" " << -matrix_view_new[6]<< " " << -matrix_view_new[10] << std::endl;
    // std::cout << _matrix_view[2]<<" " << _matrix_view[6]<< " " << _matrix_view[10] << std::endl;

    // std::cout << rot_mat << std::endl;

    _matrix_view = matrix_view_new;
}

scm::math::mat4f &Camera_View::get_matrix_view()
{
    // std::cout << _rotation << std::endl;
    // std::cout << _position << std::endl;
    if(_mode_is_ego)
    {
        update_matrices();
    }
    else
    {
        _matrix_view = scm::math::make_look_at_matrix(_position, scm::math::vec3f(0.0, 0.0, 0.0), scm::math::vec3f(0.0, 1.0, 0.0));
    }
    return _matrix_view;
}
float &Camera_View::get_width_window() { return _width_window; }
float &Camera_View::get_height_window() { return _height_window; }
scm::math::mat4f &Camera_View::get_matrix_perspective() { return _matrix_perspective; }
void Camera_View::set_position(scm::math::vec3f position)
{
    _position = position;
    update_matrices();
}
scm::math::vec3f &Camera_View::get_position() { return _position; }

scm::math::quat<float> &Camera_View::get_rotation() { return _rotation; }
void Camera_View::set_rotation(scm::math::quat<float> rotation)
{
    _rotation = scm::math::normalize(rotation);
    update_matrices();
}
void Camera_View::reset()
{
    _position = _position_initial;
    _rotation = _rotation_initial;
    update_matrices();
}
