//
// Created by moqe3167 on 30/05/18.
//

#ifndef LAMURE_XYZ_H
#define LAMURE_XYZ_H

#include <scm/core/math.h>

struct xyz {
    scm::math::vec3f pos_;
    uint8_t r_;
    uint8_t g_;
    uint8_t b_;
    uint8_t a_;
    float rad_;
    scm::math::vec3f nml_;
};

#endif //LAMURE_XYZ_H
