#include "measurement.h"

measurement::measurement(const image &_still_image, const point &_feature_point, const vec<float, 2> &_position)
        : _still_image(_still_image), _feature_point(_feature_point), _position(_position) {}

const image &measurement::get_still_image() const {
    return _still_image;
}

void measurement::set_still_image(const image &_still_image) {
    measurement::_still_image = _still_image;
}

const point &measurement::get_feature_point() const {
    return _feature_point;
}

void measurement::set_feature_point(const point &_feature_point) {
    measurement::_feature_point = _feature_point;
}

const vec<float, 2> &measurement::get_position() const {
    return _position;
}

void measurement::set_position(const vec<float, 2> &_position) {
    measurement::_position = _position;
}
