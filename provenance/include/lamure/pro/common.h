#ifndef LAMURE_COMMON_H
#define LAMURE_COMMON_H

#include "3dparty/exif.h"
#include "3dparty/tinyply.h"
#include <assert.h>
#include <boost/crc.hpp>
#include <fstream>
#include <memory>
#include <stdio.h>
#include <string>

#include <scm/core.h>
#include <scm/core/math.h>

namespace prov
{
bool DEBUG = false;

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

template <typename T>
T swap(const T &arg, bool big_in_mem)
{
    if((BYTE_ORDER == BIG_ENDIAN) == big_in_mem)
    {
        return arg;
    }
    else
    {
        T ret;

        char *dst = reinterpret_cast<char *>(&ret);
        const char *src = reinterpret_cast<const char *>(&arg + 1);

        for(size_t i = 0; i < sizeof(T); i++)
            *dst++ = *--src;

        return ret;
    }
}

static inline std::string &ltrim(std::string &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

static inline std::string &rtrim(std::string &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

static inline std::string &trim(std::string &s) { return ltrim(rtrim(s)); }
}

#endif // LAMURE_COMMON_H