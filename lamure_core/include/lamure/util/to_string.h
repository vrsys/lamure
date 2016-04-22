// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef LAMURE_UTIL_TO_STRING_H_
#define LAMURE_UTIL_TO_STRING_H_

#include <sstream>
#include <string>

namespace lamure {
namespace util {

template<typename T>
std::string
to_string(const T& value)
{
  std::ostringstream oss;
  oss << value;
  return oss.str();
}

} // namespace util
} // namespace lamure

#endif
