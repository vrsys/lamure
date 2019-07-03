// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/platform.h>

namespace lamure
{
namespace pre
{
#ifdef _WIN32
#if defined(LAMURE_PREPROCESSING_LIBRARY)
template<typename T>
array_abstract<T>::
~array_abstract()
{
    try {
        reset();
    }
    catch (...) {}
}
#endif
#else
template<typename T>
array_abstract<T>::
~array_abstract()
{
    try {
        reset();
    }
    catch (...) {}
}
#endif
}
} // namespace lamure

