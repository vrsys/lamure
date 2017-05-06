#include "point.h"


const vec3f &point::get_center() const {
    return _center;
}

void point::set_center(const vec3f &_center) {
    point::_center = _center;
}

const vec3i &point::get_color() const {
    return _color;
}

void point::set_color(const vec3i &_color) {
    point::_color = _color;
}

point::point(const vec3f &_center, const vec3i &_color, const vector<point::measurement> &_measurements) : _center(_center),
                                                                                                    _color(_color),
                                                                                                    _measurements(
                                                                                                            _measurements) {}

const vector<point::measurement> &point::get_measurements() const {
    return _measurements;
}

void point::set_measurements(const vector<point::measurement> &_measurements) {
    point::_measurements = _measurements;
}

point::point() {}

point::measurement::measurement(const image &_still_image, const vec2f &_position)
        : _still_image(_still_image), _position(_position) {}

const image &point::measurement::get_still_image() const {
    return _still_image;
}

void point::measurement::set_still_image(const image &_still_image) {
    measurement::_still_image = _still_image;
}

const vec2f &point::measurement::get_position() const {
    return _position;
}

void point::measurement::set_position(const vec2f &_position) {
    measurement::_position = _position;
}
