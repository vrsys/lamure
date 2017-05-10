#include "Camera_View.h"

Camera_View::Camera_View()
{
    // _matrix_view = scm::math::mat4f::identity();
    // scm::math::translate(_matrix_view, scm::math::vec3f(0.0, 0.0, 0.0));

    // _matrix_view = scm::math::make_look_at_matrix(_position, scm::math::vec3f(0.0, 0.0, 0.0), scm::math::vec3f(0.0, 1.0, 0.0));
    // std::cout << "matrix_view:" << std::endl << _matrix_view << std::endl;
    _matrix_perspective = scm::math::make_perspective_matrix(60.0f, 1920.0f / 1080.0f, 0.01f, 1000.0f);
}
void Camera_View::translate(scm::math::vec3f offset)
{
    _position += offset;
    update_matrices();
}
void Camera_View::update_matrices()
{
    scm::math::mat4f view_matrix_new = scm::math::mat4f::identity();

    scm::math::mat4f matrix_translation = scm::math::make_translation(scm::math::vec3f(_position));
    scm::math::mat4f matrix_rotation = scm::math::mat4f(_rotation.to_matrix());

    view_matrix_new = matrix_translation * matrix_rotation;

    _matrix_view = view_matrix_new;
}

scm::math::mat4f &Camera_View::get_matrix_view()
{
    return _matrix_view;
}
scm::math::mat4f &Camera_View::get_matrix_perspective()
{
    return _matrix_perspective;
}
void Camera_View::set_position(scm::math::vec3f position)
{
    _position = position;
    update_matrices();
}
void Camera_View::set_rotation(scm::math::quat<double> rotation)
{
    _rotation = rotation;
    update_matrices();
}
void Camera_View::reset()
{
    _position = _position_initial;
    _rotation = _rotation_initial;
    update_matrices();
}
