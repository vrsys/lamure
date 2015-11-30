// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/surfel.h>

namespace lamure {
namespace pre
{

bool surfel::
compare_x(const surfel &left, const surfel &right)
{
    return left.pos().x < right.pos().x;
}

bool surfel::
compare_y(const surfel &left, const surfel &right)
{
    return left.pos().y < right.pos().y;
}

bool surfel::
compare_z(const surfel &left, const surfel &right)
{
    return left.pos().z < right.pos().z;
}

surfel::compare_function
surfel::compare(const uint8_t axis)
{
    assert(axis <= 2);
    compare_function comp;

    switch (axis) {
        case 0:
            comp = &surfel::compare_x; break;
        case 1:
            comp = &surfel::compare_y; break;
        case 2:
            comp = &surfel::compare_z; break;
    }
    return comp;
}

}} // namespace lamure

