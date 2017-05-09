#include "Camera_View.h"

Camera_View::Camera_View()
{
    // _matrix_view = scm::math::mat4f::identity();
    // scm::math::translate(_matrix_view, scm::math::vec3f(0.0, 0.0, 0.0));

    _matrix_view = scm::math::make_look_at_matrix(_position, scm::math::vec3f(0.0, 0.0, 0.0), scm::math::vec3f(0.0, 1.0, 0.0));
    // std::cout << "matrix_view:" << std::endl << _matrix_view << std::endl;
    _matrix_perspective = scm::math::make_perspective_matrix(60.0f, 800.0f / 600.0f, 0.01f, 1000.0f);
}
void Camera_View::translate(scm::math::vec3f offset)
{

    _position += offset;
    // std::cout << _position << std::endl;
    _matrix_view = scm::math::make_look_at_matrix(_position, _position + _rotation, scm::math::vec3f(0.0, 1.0, 0.0));

}

scm::math::mat4f &Camera_View::get_matrix_view()
{
    return _matrix_view;
}
scm::math::mat4f &Camera_View::get_matrix_perspective()
{
    return _matrix_perspective;
}
void Camera_View::set_rotation(scm::math::vec3f rotation)
{
    _rotation = rotation;
}
