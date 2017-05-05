#ifndef REN_UTILS_H_
#define REN_UTILS_H_

#include "camera.h"
#include "point.h"
#include <iostream>

namespace utils {
    static bool read_environment(ifstream &in,
                                 vector<camera> &camera_vec,
                                 vector<point> &point_vec,
                                 vector<measurement> &measurements,
                                 vector<int> &ptidx,
                                 vector<int> &camidx,
                                 vector<string> &names,
                                 vector<int> &ptc);

    template<typename T>
    vec<T, 2> pair_to_vec2(T *arr);

    template<typename T>
    vec<T, 3> arr3_to_vec3(T arr[3]);

    template<typename T>
    mat<T, 3, 3> arr9_to_mat3(T arr[9]);
};

#endif
