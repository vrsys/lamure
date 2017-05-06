#include "camera.h"

const image &camera::get_still_image() const {
    return _still_image;
}

void camera::set_still_image(const image &_still_image) {
    camera::_still_image = _still_image;
}

double camera::get_focal_length() const {
    return _focal_length;
}

void camera::set_focal_length(double _focal_length) {
    camera::_focal_length = _focal_length;
}

const quat<double> &camera::get_orientation() const {
    return _orientation;
}

void camera::set_orientation(const quat<double> &_orientation) {
    camera::_orientation = _orientation;
}

const vec<double, 3> &camera::get_center() const {
    return _center;
}

void camera::set_center(const vec<double, 3> &_center) {
    camera::_center = _center;
}

double camera::get_radial_distortion() const {
    return _radial_distortion;
}

void camera::set_radial_distortion(double _radial_distortion) {
    camera::_radial_distortion = _radial_distortion;
}

camera::camera(const image &_still_image, double _focal_length, const quat<double> &_orientation,
               const vec<double, 3> &_center, double _radial_distortion) : _still_image(_still_image),
                                                                           _focal_length(_focal_length),
                                                                           _orientation(_orientation), _center(_center),
                                                                           _radial_distortion(_radial_distortion) {}

camera::camera():_still_image() {}
