#include "lamure/pro/data/Point.h"

namespace prov
{
Point::Point(long _index, const vec3d &_center, const vec3d &_color, const vec<char> &_metadata) : MetaContainer(_metadata), _index(_index), _center(_center), _color(_color) {}
Point::~Point() {}
long Point::get_index() const { return _index; }
const vec3d &Point::get_center() const { return _center; }
const vec3d &Point::get_color() const { return _color; };
}