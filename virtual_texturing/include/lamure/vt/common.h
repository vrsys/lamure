//
// Created by sebastian on 13.11.17.
//

#ifndef VT_COMMON_H
#define VT_COMMON_H

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <utime.h>

#include <atomic>
#include <condition_variable>
#include <fstream>
#include <iostream>
#include <mutex>
#include <queue>
#include <set>
#include <string>
#include <thread>

#include <cstdlib>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <lamure/vt/ext/imgui.h>
#include <lamure/vt/ext/imgui_impl_glfw_gl3.h>

#include <boost/assign/list_of.hpp>
#include <cstdint>
#include <cstring>
#include <functional>
#include <algorithm>
#include <lamure/config.h>
#include <lamure/vt/ext/SimpleIni.h>

#include <scm/core.h>
#include <scm/core/io/tools.h>
#include <scm/core/pointer_types.h>
#include <scm/core/time/accum_timer.h>
#include <scm/core/time/high_res_timer.h>
#include <scm/log.h>

#include <scm/gl_core.h>

#include <scm/gl_util/data/imaging/texture_loader.h>
#include <scm/gl_util/manipulators/trackball_manipulator.h>
#include <scm/gl_util/primitives/box.h>
#include <scm/gl_util/primitives/quad.h>
#include <scm/gl_util/primitives/wavefront_obj.h>

#include <cstddef>
#include <list>
#include <stdexcept>
#include <unordered_map>

namespace vt
{
typedef uint64_t id_type;
typedef uint32_t priority_type;
typedef std::set<id_type> cut_type;

struct mem_slot_type
{
    size_t position = SIZE_MAX;
    id_type tile_id = UINT64_MAX;
    uint8_t *pointer = nullptr;
    bool locked = false;
    bool updated = false;
};

typedef std::vector<mem_slot_type> mem_slots_type;
typedef std::map<id_type, size_t> mem_slots_index_type;
class Cut;
typedef std::map<uint64_t, Cut*> cut_map_type;
typedef std::pair<uint64_t, Cut*> cut_map_entry_type;
}

#endif // VT_COMMON_H
