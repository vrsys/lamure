// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/gl/vertex_array.h>

namespace lamure {
namespace gl {


vertex_array_t::vertex_array_t(uint32_t num_elements) 
: element_size_(0),
  element_components_(0) {

}


vertex_array_t::~vertex_array_t() {

}

void
vertex_array_t::add_attribute(
  const attribute_t attribute,
  uint32_t layout_location,
  bool normalize)
{
  uint32_t attribute_bytes_per_index_table[5] = {
    1*sizeof(float32_t), //FLOAT
    2*sizeof(float32_t), //VEC2F
    3*sizeof(float32_t), //VEC3F
    4*sizeof(float32_t), //VEC4F
    sizeof(uint8_t)      //UBYTE
  };

  uint32_t attribute_components_per_index_table[5] = {
    1,
    2,
    3,
    4,
    1
  };

  attribute_descr_t a;
  a.attribute_ = attribute;
  a.location_ = layout_location;
  a.bytes_per_index_ = attribute_bytes_per_index_table[attribute];
  a.num_components_ = attribute_components_per_index_table[attribute];
  a.normalize_ = (uint32_t)normalize;

  a.offset_ = 0;
  for (const auto& attrib : attributes_) {
    a.offset_ += attrib.bytes_per_index_;
  }

  attributes_.push_back(a);
  element_size_ += a.bytes_per_index_;
  element_components_ += a.num_components_;
}

void
vertex_array_t::declare_attributes(
  uint32_t location_offset) const
{

  GLenum gl_enum_per_index_table[5] = {
    GL_FLOAT,
    GL_FLOAT,
    GL_FLOAT,
    GL_FLOAT,
    GL_UNSIGNED_BYTE
  };

  GLboolean gl_boolean_normalize_table[2] = {
    GL_FALSE,
    GL_TRUE
  };

  for (const auto& a : attributes_) {
    glEnableVertexAttribArray(location_offset + a.location_);
    glVertexAttribPointer(
      location_offset + a.location_, 
      a.num_components_, 
      gl_enum_per_index_table[a.attribute_], 
      gl_boolean_normalize_table[a.normalize_], 
      element_size_, 
      (GLvoid*)&a.offset_);
  }
}

void
vertex_array_t::undeclare_attributes(
  uint32_t location_offset) const
{
  for (const auto& a : attributes_) {
    glDisableVertexAttribArray(location_offset + a.location_);
  }
}


} // namespace gl
} // namespace lamure
