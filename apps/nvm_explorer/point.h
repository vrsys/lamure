#ifndef LAMURE_POINT_H
#define LAMURE_POINT_H

#include "measurement.h"
#include <scm/core/math/vec3.h>
#include <unordered_set>

using namespace std;

class point {
private:

    vec<double, 3> _center;
    vec<int, 3> _color;
    unordered_set<measurement> _measurements;

public:

    point(const vec<double, 3> &_center, const vec<int, 3> &_color, const unordered_set<measurement> &_measurements);

    const vec<double, 3> &get_center() const;

    void set_center(vec<float, 3> _center);

    const vec<int, 3> &get_color() const;

    void set_color(const vec<int, 3> &_color);

    const unordered_set<measurement> &get_measurements() const;

    void set_measurements(const unordered_set<measurement> &_measurements);
};


#endif //LAMURE_POINT_H
