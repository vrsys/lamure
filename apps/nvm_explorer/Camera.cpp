#include "Camera.h"

const Image &Camera::get_still_image() const {
    return _still_image;
}

void Camera::set_still_image(const Image &_still_image) {
    Camera::_still_image = _still_image;
}

double Camera::get_focal_length() const {
    return _focal_length;
}

void Camera::set_focal_length(double _focal_length) {
    Camera::_focal_length = _focal_length;
}

const quat<double> &Camera::get_orientation() const {
    return _orientation;
}

void Camera::set_orientation(const quat<double> &_orientation) {
    Camera::_orientation = _orientation;
}

const vec3d &Camera::get_center() const {
    return _center;
}

void Camera::set_center(const vec3d &_center) {
    Camera::_center = _center;
}

double Camera::get_radial_distortion() const {
    return _radial_distortion;
}

void Camera::set_radial_distortion(double _radial_distortion) {
    Camera::_radial_distortion = _radial_distortion;
}

Camera::Camera(const Image &_still_image, double _focal_length, const quat<double> &_orientation,
               const vec3d &_center, double _radial_distortion) : _still_image(_still_image),
                                                                           _focal_length(_focal_length),
                                                                           _orientation(_orientation), _center(_center),
                                                                           _radial_distortion(_radial_distortion) {}

Camera::Camera():_still_image() {}
