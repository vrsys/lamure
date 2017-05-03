//
// Created by anton on 03.05.17.
//

#ifndef LAMURE_POINT_H
#define LAMURE_POINT_H

#include <ext/hash_set>
#include "measurement.h"

class point {
private:
    double *center;
    int *color;
    hash_set <measurement> measurements;
};


#endif //LAMURE_POINT_H
