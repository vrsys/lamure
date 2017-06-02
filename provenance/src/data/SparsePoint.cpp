#include <lamure/pro/data/SparsePoint.h>

namespace prov
{
SparsePoint::SparsePoint(long _index, const vec3d &_center, const vec3d &_color, const vec<char> &_metadata, const vec<SparsePoint::Measurement> &_measurements)
    : Point(_index, _center, _color, _metadata), _measurements(_measurements)
{
}
SparsePoint::~SparsePoint() {}
const vec<SparsePoint::Measurement> &SparsePoint::get_measurements() const { return _measurements; }
SparsePoint::Measurement::Measurement(const Camera &_camera, const vec2d &_occurence) : _camera(_camera), _occurence(_occurence) {}
SparsePoint::Measurement::~Measurement() {}
const Camera &SparsePoint::Measurement::get_camera() const { return _camera; }
const vec2d &SparsePoint::Measurement::get_occurence() const { return _occurence; };
}