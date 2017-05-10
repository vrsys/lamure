#include "Point.h"

const vec3f &Point::get_center() const
{
    return _center;
}

void Point::set_center(const vec3f &_center)
{
    Point::_center = _center;
}

const vec3i &Point::get_color() const
{
    return _color;
}

void Point::set_color(const vec3i &_color)
{
    Point::_color = _color;
}

Point::Point(const vec3f &_center, const vec3i &_color, const vector<Point::measurement> &_measurements) : _center(
    _center),
                                                                                                           _color(_color),
                                                                                                           _measurements(
                                                                                                               _measurements)
{}

const vector<Point::measurement> &Point::get_measurements() const
{
    return _measurements;
}

void Point::set_measurements(const vector<Point::measurement> &_measurements)
{
    Point::_measurements = _measurements;
}

Point::Point()
{}

Point::measurement::measurement(const Image &_still_image, const vec2f &_position)
    : _still_image(_still_image), _position(_position)
{}

const Image &Point::measurement::get_still_image() const
{
    return _still_image;
}

void Point::measurement::set_still_image(const Image &_still_image)
{
    measurement::_still_image = _still_image;
}

const vec2f &Point::measurement::get_position() const
{
    return _position;
}

void Point::measurement::set_position(const vec2f &_position)
{
    measurement::_position = _position;
}
