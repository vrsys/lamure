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

enum class RepRadiusAlgorithm {
    ArithmeticMean = 0,
    GeometricMean  = 1,
    HarmonicMean   = 2
};

enum class NormalRadiusAlgorithm {
	PlaneFitting = 0
};

enum class ReductionAlgorithm {
    NDC            = 0,
    Constant       = 1,
    EverySecond    = 2,
    Random         = 3
};

}}

#endif // PRE_COMMON_H_

