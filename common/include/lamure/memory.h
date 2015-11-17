// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef COMMON_MEMORY_H_
#define COMMON_MEMORY_H_

#include <lamure/platform.h>
#include <cstddef>

namespace lamure {

COMMON_DLL const size_t GetTotalMemory();
COMMON_DLL const size_t GetAvailableMemory(const bool use_buffers_cache = true);
COMMON_DLL const size_t GetProcessUsedMemory();

} // namespace lamure

#endif // COMMON_MEMORY_H_

