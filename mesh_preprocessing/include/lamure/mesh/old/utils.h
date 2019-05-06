#ifndef LAMURE_MP_UTILS_H
#define LAMURE_MP_UTILS_H

struct BoundingBoxLimits
{
    scm::math::vec3f min;
    scm::math::vec3f max;
};

namespace utils
{
    static Vector normalise(Vector v) { return v / std::sqrt(v.squared_length()); }
    static Point midpoint(Point p1, Point p2) { return Point(((p1.x() + p2.x()) / 2.0), ((p1.y() + p2.y()) / 2.0), ((p1.z() + p2.z()) / 2.0)); }
}

#endif // LAMURE_MP_UTILS_H
