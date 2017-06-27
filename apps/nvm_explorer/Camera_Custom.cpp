#include "Camera_Custom.h"

Camera_Custom::Camera_Custom(prov::Camera camera) : prov::Camera(camera)
{
    update_transformation();

    float width_world = (1.0f / _fp_resolution_x) * _im_width;
    float height_world = (1.0f / _fp_resolution_y) * _im_height;
    _frustum = Frustum(width_world, height_world, _focal_length);
}

void Camera_Custom::update_transformation_image_plane()
{
    scm::math::mat4f matrix_translation = scm::math::make_translation(scm::math::vec3f(0.0, 0.0, -_focal_length));
    // // normalize quad to 1m*1m
    float scale = 0.5;
    // // get the width of quad in world dimension (should be sensor size)
    scale *= (1.0f / _fp_resolution_x) * _im_width;
    // // increase size for better visibility
    // scale *= _frustum.get_scale();

    float aspect_ratio = float(_im_height) / _im_width;
    scm::math::mat4f matrix_scale = scm::math::make_scale(scm::math::vec3f(scale, scale * aspect_ratio, scale));
    // _transformation_image_plane = _transformation * matrix_translation;
    _transformation_image_plane = _transformation * matrix_translation * matrix_scale;
}

// scm::gl::sampler_state_ptr Camera_Custom::get_state() { return _state; }

void Camera_Custom::load_texture(scm::shared_ptr<scm::gl::render_device> device)
{
    scm::gl::texture_loader tl;
    std::cout << "creating texture" << std::endl;
    // _texture.reset();
    _texture = tl.load_texture_2d(*device, _im_file_name, true, false);

    _state = device->create_sampler_state(scm::gl::FILTER_MIN_MAG_LINEAR, scm::gl::WRAP_CLAMP_TO_EDGE);
}
// scm::gl::texture_2d_ptr Camera_Custom::get_texture() { return _texture; }

void Camera_Custom::update_transformation()
{
    scm::math::mat4f matrix_translation = scm::math::make_translation(scm::math::vec3f(_translation));
    scm::math::mat4f matrix_rotation = scm::math::mat4f(_orientation.to_matrix());
    scm::math::mat4f matrix_scale = scm::math::make_scale(scm::math::vec3f(_frustum.get_scale(), _frustum.get_scale(), _frustum.get_scale()));

    _transformation = matrix_translation * matrix_rotation * matrix_scale;
    update_transformation_image_plane();
}

void Camera_Custom::update_scale_frustum(float offset)
{
    _frustum.update_scale_frustum(offset);
    update_transformation();
    // _frustum.init(device, calc_frustum_points());
}

scm::math::mat4f &Camera_Custom::get_transformation() { return _transformation; }
scm::math::mat4f &Camera_Custom::get_transformation_image_plane() { return _transformation_image_plane; }

// const Image &Camera_Custom::get_still_image() const { return _still_image; }

void Camera_Custom::init(scm::shared_ptr<scm::gl::render_device> device)
{
    load_texture(device);
    _frustum.init(device);
}

// void Camera_Custom::update_scale_frustum(scm::shared_ptr<scm::gl::render_device> device, float offset)
// {
//     float new_scale = std::max(_scale + offset, 0.0f);
//     _scale = new_scale;
//     update_transformation();
//     _frustum.init(device, calc_frustum_points());
// }
void Camera_Custom::bind_texture(scm::shared_ptr<scm::gl::render_context> context) { context->bind_texture(_texture, _state, 0); }

// void Camera_Custom::set_still_image(const Image &_still_image)
// {
//     Camera_Custom::_still_image = _still_image;
//     update_transformation();
// }

// void Camera_Custom::update_transformation()
// {
//     scm::math::mat4f matrix_translation = scm::math::make_translation(scm::math::vec3f(_center));
//     scm::math::mat4f matrix_rotation = scm::math::mat4f(_orientation.to_matrix());

//     // _transformation = matrix_translation;
//     // _transformation =  matrix_rotation;
//     // _transformation =  scm::math::inverse(matrix_rotation) * matrix_translation;
//     // _transformation = matrix_translation * matrix_rotation;
//     _transformation = matrix_translation * matrix_rotation;
//     _still_image.update_transformation(_transformation, _scale);
// }

// double Camera_Custom::get_focal_length() const { return _focal_length; }

// void Camera_Custom::set_focal_length(double _focal_length) { Camera_Custom::_focal_length = _focal_length; }

// const quat<double> &Camera_Custom::get_orientation() const { return _orientation; }

// void Camera_Custom::set_orientation(const quat<double> &orientation)
// {
//     Camera_Custom::_orientation = scm::math::normalize(orientation);
//     update_transformation();
// }

// const vec3d &Camera_Custom::get_center() const { return _center; }

// void Camera_Custom::set_center(const vec3d &_center)
// {
//     Camera_Custom::_center = _center;
//     update_transformation();
// }

// double Camera_Custom::get_radial_distortion() const { return _radial_distortion; }

// void Camera_Custom::set_radial_distortion(double _radial_distortion) { Camera_Custom::_radial_distortion = _radial_distortion; }

// scm::math::mat4f &Camera_Custom::get_transformation() { return _transformation; }

Frustum &Camera_Custom::get_frustum() { return _frustum; }

// Camera_Custom::Camera(const Image &_still_image, double _focal_length, const quat<double> &_orientation, const vec3d &_center, double _radial_distortion)
//     : _still_image(_still_image), _focal_length(_focal_length), _orientation(_orientation), _center(_center), _radial_distortion(_radial_distortion)
// {
// }

// Camera_Custom::Camera() : _still_image() {}
