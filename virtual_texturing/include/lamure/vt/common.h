//
// Created by sebastian on 13.11.17.
//

#ifndef VT_COMMON_H
#define VT_COMMON_H

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <utime.h>

#include <iostream>
#include <fstream>
#include <string>
#include <mutex>
#include <condition_variable>
#include <thread>

#include <cstdlib>

#include <lamure/config.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cstdint>
#include <cstring>
#include <functional>
#include <boost/assign/list_of.hpp>
#include <lamure/vt/ext/SimpleIni.h>

#include <scm/core.h>
#include <scm/log.h>
#include <scm/core/pointer_types.h>
#include <scm/core/io/tools.h>
#include <scm/core/time/accum_timer.h>
#include <scm/core/time/high_res_timer.h>

#include <scm/gl_core.h>

#include <scm/gl_util/data/imaging/texture_loader.h>
#include <scm/gl_util/manipulators/trackball_manipulator.h>
#include <scm/gl_util/primitives/box.h>
#include <scm/gl_util/primitives/quad.h>
#include <scm/gl_util/primitives/wavefront_obj.h>

namespace vt
{
typedef uint64_t id_type;
typedef uint32_t priority_type;
}

#endif // VT_COMMON_H
