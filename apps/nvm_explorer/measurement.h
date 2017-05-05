#ifndef LAMURE_MEASUREMENT_H
#define LAMURE_MEASUREMENT_H

#include <scm/core/math/vec.h>
#include "image.h"
#include "point.h"

using namespace scm::math;

class measurement {
private:
public:
    const image &get_still_image() const;

    void set_still_image(const image &_still_image);

    const point &get_feature_point() const;

    void set_feature_point(const point &_feature_point);

    const vec<float, 2> &get_position() const;

    void set_position(const vec<float, 2> &_position);

    measurement(const image &_still_image, const point &_feature_point, const vec<float, 2> &_position);

private:

    image _still_image;
    point _feature_point;
    vec<float, 2> _position;
};


#endif //LAMURE_MEASUREMENT_H
