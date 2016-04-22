// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef LAMURE_UTIL_VECTOR_TO_PTR_H_
#define LAMURE_UTIL_VECTOR_TO_PTR_H_

#include <algorithm>
#include <vector>

namespace lamure {
namespace util {

/**
 * converts the input vector to the vector of pointers.
 * The pointers refer to the base class of input's elements.
 */
template <class base_t, class derived_t>
std::vector<base_t*> vector_to_ptr(std::vector<derived_t>& input)
{
    std::vector<base_t*> result(input.size());
    std::transform(input.begin(), input.end(), result.begin(),
      [](derived_t& i){ return &i; });
    return result;
}

} // namespace util
} // namespace lamure

#endif
