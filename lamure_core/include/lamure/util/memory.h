// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef LAMURE_UTIL_MEMORY_H_
#define LAMURE_UTIL_MEMORY_H_

#include <lamure/platform_core.h>
#include <cstddef>

namespace lamure {
namespace util {

CORE_DLL const size_t get_total_memory();
CORE_DLL const size_t get_available_memory(const bool use_buffers_cache = true);
CORE_DLL const size_t get_process_used_memory();

} // namespace util
} // namespace lamure

#endif // LAMURE_UTIL_MEMORY_H_

