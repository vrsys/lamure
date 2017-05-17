#ifndef REN_UTILS_H_
#define REN_UTILS_H_

#include "Camera.h"
#include "Point.h"
#include <iostream>
#include <scm/core/math.h>
#include <lamure/ren/camera.h>
#include <lamure/ren/cut.h>

#include <lamure/ren/controller.h>
#include <lamure/ren/model_database.h>
#include <lamure/ren/cut_database.h>
#include <lamure/ren/controller.h>
#include <lamure/ren/policy.h>

#include <boost/assign/list_of.hpp>
#include <memory>

#include <fstream>

#include <scm/core.h>
#include <scm/log.h>
#include <scm/core/time/accum_timer.h>
#include <scm/core/time/high_res_timer.h>
#include <scm/core/pointer_types.h>
#include <scm/core/io/tools.h>
#include <scm/core/time/accum_timer.h>
#include <scm/core/time/high_res_timer.h>

#include <scm/gl_util/data/imaging/texture_loader.h>
#include <scm/gl_util/viewer/camera.h>
#include <scm/gl_util/primitives/quad.h>
#include <scm/gl_util/primitives/box.h>

#include <scm/core/math.h>

#include <scm/gl_core/gl_core_fwd.h>
#include <scm/gl_util/primitives/primitives_fwd.h>
#include <scm/gl_util/primitives/geometry.h>

#include <scm/gl_util/font/font_face.h>
#include <scm/gl_util/font/text.h>
#include <scm/gl_util/font/text_renderer.h>

#include <scm/core/platform/platform.h>
#include <scm/core/utilities/platform_warning_disable.h>
#include <scm/gl_util/primitives/geometry.h>
#include "tinyply.h"

namespace utils
{
bool read_nvm (ifstream &in,
               vector<Camera> &camera_vec,
               vector<Point> &point_vec,
               vector<Image> &images);

bool read_ply(ifstream &in,
              vector<Point> &point_vec);

template<typename T>
vec<T, 2> pair_to_vec2 (T *arr);

template<typename T>
vec<T, 3> arr3_to_vec3 (T arr[3]);

template<typename T>
mat<T, 3, 3> arr9_to_mat3 (T arr[9]);
};
#endif

