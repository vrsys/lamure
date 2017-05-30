#include "Camera.h"

const Image &Camera::get_still_image() const { return _still_image; }

void Camera::init(scm::shared_ptr<scm::gl::render_device> device)
{
    _still_image.load_texture(device);

    _frustum.init(device, calc_frustum_points());
}
std::vector<scm::math::vec3f> Camera::calc_frustum_points()
{
    std::vector<scm::math::vec3f> result;

    // std::cout << _still_image.get_width_world() << std::endl;

    float width_world = _still_image.get_width_world();
    float width_world_half = width_world / 2;
    float height_world = _still_image.get_height_world();
    float height_world_half = height_world / 2;
    // std::cout << _still_image.get_width_world() << std::endl;

    width_world_half *= _scale;
    height_world_half *= _scale;

    result.push_back(scm::math::vec3f(0.0, 0.0, 0.0));
    result.push_back(scm::math::vec3f(0.0, 0.0, 0.0));
    result.push_back(scm::math::vec3f(0.0, 0.0, 0.0));
    result.push_back(scm::math::vec3f(0.0, 0.0, 0.0));
    result.push_back(scm::math::vec3f(-width_world_half, height_world_half, -_still_image.get_focal_length() * _scale));
    result.push_back(scm::math::vec3f(width_world_half, height_world_half, -_still_image.get_focal_length() * _scale));
    result.push_back(scm::math::vec3f(-width_world_half, -height_world_half, -_still_image.get_focal_length() * _scale));
    result.push_back(scm::math::vec3f(width_world_half, -height_world_half, -_still_image.get_focal_length() * _scale));
    // scm::math::vec3d _direction_of_camera = scm::math::vec3d(0.0, 0.0, -1.0);
    // scm::math::vec3d center_image_plane = _center + _focal_length * scm::math::normalize(_direction_of_camera);
    // width_image_plane = (1.0f/_still_image.get_fp_resolution_x()) * _still_image.get_width();
    return result;
}
void Camera::update_scale_frustum(scm::shared_ptr<scm::gl::render_device> device, float offset)
{
    float new_scale = std::max(_scale + offset, 0.0f);
    _scale = new_scale;
    update_transformation();
    _frustum.init(device, calc_frustum_points());
}
void Camera::bind_texture(scm::shared_ptr<scm::gl::render_context> context) { context->bind_texture(_still_image.get_texture(), _still_image.get_state(), 0); }

void Camera::set_still_image(const Image &_still_image)
{
    Camera::_still_image = _still_image;
    update_transformation();
}

void Camera::update_transformation()
{
    scm::math::mat4f matrix_translation = scm::math::make_translation(scm::math::vec3f(_center));
    scm::math::mat4f matrix_rotation = scm::math::mat4f(_orientation.to_matrix());

    // _transformation = matrix_translation;
    // _transformation =  matrix_rotation;
    // _transformation =  scm::math::inverse(matrix_rotation) * matrix_translation;
    // _transformation = matrix_translation * matrix_rotation;
    _transformation = matrix_translation * matrix_rotation;
    _still_image.update_transformation(_transformation, _scale);
}

double Camera::get_focal_length() const { return _focal_length; }

void Camera::set_focal_length(double _focal_length) { Camera::_focal_length = _focal_length; }

const quat<double> &Camera::get_orientation() const { return _orientation; }

void Camera::set_orientation(const quat<double> &orientation)
{
    Camera::_orientation = scm::math::normalize(orientation);
    update_transformation();
}

const vec3d &Camera::get_center() const { return _center; }

void Camera::set_center(const vec3d &_center)
{
    Camera::_center = _center;
    update_transformation();
}

double Camera::get_radial_distortion() const { return _radial_distortion; }

void Camera::set_radial_distortion(double _radial_distortion) { Camera::_radial_distortion = _radial_distortion; }

scm::math::mat4f &Camera::get_transformation() { return _transformation; }

Frustum &Camera::get_frustum() { return _frustum; }

Camera::Camera(const Image &_still_image, double _focal_length, const quat<double> &_orientation, const vec3d &_center, double _radial_distortion)
    : _still_image(_still_image), _focal_length(_focal_length), _orientation(_orientation), _center(_center), _radial_distortion(_radial_distortion)
{
}

Camera::Camera() : _still_image() {}
