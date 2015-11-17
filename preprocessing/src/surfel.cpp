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

bool Surfel::
CompareX(const Surfel &left, const Surfel &right)
{
    return left.pos().x < right.pos().x;
}

bool Surfel::
CompareY(const Surfel &left, const Surfel &right)
{
    return left.pos().y < right.pos().y;
}

bool Surfel::
CompareZ(const Surfel &left, const Surfel &right)
{
    return left.pos().z < right.pos().z;
}

Surfel::CompareFunction
Surfel::Compare(const uint8_t axis)
{
    assert(axis <= 2);
    CompareFunction comp;

    switch (axis) {
        case 0:
            comp = &Surfel::CompareX; break;
        case 1:
            comp = &Surfel::CompareY; break;
        case 2:
            comp = &Surfel::CompareZ; break;
    }
    return comp;
}

}} // namespace lamure

