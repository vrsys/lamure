#include "lamure/pro/data/DensePoint.h"

namespace prov
{
DensePoint::DensePoint(long _index, const vec3d &_center, const vec3d &_color, const vec<char> &_metadata, const vec3d &_normal) : Point(_index, _center, _color, _metadata), _normal(_normal) {}
DensePoint::~DensePoint() {}
const vec3d &DensePoint::get_normal() const { return _normal; }
}