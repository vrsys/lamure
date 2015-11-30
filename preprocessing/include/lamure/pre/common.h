// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_COMMON_H_
#define PRE_COMMON_H_

namespace lamure {
namespace pre
{

enum class rep_radius_algorithm {
    arithmetic_mean = 0,
    geometric_mean  = 1,
    harmonic_mean   = 2
};

enum class reduction_algorithm {
    NDC            = 0,
    constant       = 1,
    every_second    = 2
};

}}

#endif // PRE_COMMON_H_

