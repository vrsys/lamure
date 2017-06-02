#ifndef LAMURE_COMMON_H
#define LAMURE_COMMON_H

#include "3dparty/exif.h"
#include "3dparty/tinyply.h"
#include <memory>
#include <string>

#include <scm/core.h>
#include <scm/core/math.h>

namespace prov
{
typedef std::ifstream ifstream;
typedef std::string string;

template <typename T, std::size_t SIZE>
using arr = std::array<T, SIZE>;

template <typename T>
using vec = std::vector<T>;

template <typename T>
using u_ptr = std::unique_ptr<T>;

typedef scm::math::vec2d vec2d;
typedef scm::math::vec3d vec3d;
typedef scm::math::mat4d mat4d;
typedef scm::math::quatd quatd;
}

#endif // LAMURE_COMMON_H