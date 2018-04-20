// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_IO_PLY_PLY_H_
#define PRE_IO_PLY_PLY_H_

#include <cstdint>

namespace lamure
{
namespace pre
{
namespace io
{
namespace ply
{

typedef int8_t int8;

typedef int16_t int16;

typedef int32_t int32;

typedef uint8_t uint8;

typedef uint16_t uint16;

typedef uint32_t uint32;

typedef float float32;

typedef double float64;

template<typename ScalarType>
struct type_traits;

#ifdef PLY_TYPE_TRAITS
#  error
#endif

#define PLY_TYPE_TRAITS(TYPE, NAME, OLD_NAME)\
template <>\
struct type_traits<TYPE>\
{\
  typedef TYPE type;\
  static const char* name() { return NAME; }\
  static const char* old_name() { return OLD_NAME; }\
};

PLY_TYPE_TRAITS(int8, "int8", "char")
PLY_TYPE_TRAITS(int16, "int16", "short")
PLY_TYPE_TRAITS(int32, "int32", "int")
PLY_TYPE_TRAITS(uint8, "uint8", "uchar")
PLY_TYPE_TRAITS(uint16, "uint16", "ushort")
PLY_TYPE_TRAITS(uint32, "uint32", "uint")
PLY_TYPE_TRAITS(float32, "float32", "float")
PLY_TYPE_TRAITS(float64, "float64", "double")

#undef PLY_TYPE_TRAITS

typedef int format_type;

enum format
{
    ascii_format, binary_little_endian_format, binary_big_endian_format
};

}
}
}
} // namespace lamure::pre::io::ply

#endif // PRE_IO_PLY_PLY_H_
