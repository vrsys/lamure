#include "point.h"

point::point(const vec<double, 3> &_center, const vec<int, 3> &_color, const unordered_set<measurement> &_measurements)
        : _center(_center), _color(_color), _measurements(_measurements) {}

const vec<double, 3> &point::get_center() const {
    return _center;
}

void point::set_center(vec<float, 3> _center) {
    point::_center = _center;
}

const vec<int, 3> &point::get_color() const {
    return _color;
}

void point::set_color(const vec<int, 3> &_color) {
    point::_color = _color;
}

const unordered_set<measurement> &point::get_measurements() const {
    return _measurements;
}

void point::set_measurements(const unordered_set<measurement> &_measurements) {
    point::_measurements = _measurements;
}
