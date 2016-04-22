// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef LAMURE_UTIL_ADD_TO_PATH_H_
#define LAMURE_UTIL_ADD_TO_PATH_H_

#include <lamure/platform_core.h>
#include <lamure/types.h>

#include <boost/filesystem.hpp>
#include <string>

namespace lamure {
namespace util {

CORE_DLL boost::filesystem::path add_to_path(
  const boost::filesystem::path& path,
  const std::string& addition);

}
} //namespace lamure

#endif
