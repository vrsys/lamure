//
// Created by anton on 03.05.17.
//

#ifndef LAMURE_CAMERA_H
#define LAMURE_CAMERA_H

#include <boost/math/quaternion.hpp>
#include "image.h"

class camera {
private:
    image captured_image;
    double focal_length;
    boost::math::quaternion<long double> quaternion;
    double *camera_center;
    double radial_distortion;
};


#endif //LAMURE_CAMERA_H
