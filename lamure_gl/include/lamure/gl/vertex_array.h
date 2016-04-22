// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef LAMURE_GL_VERTEX_ARRAY_H_
#define LAMURE_GL_VERTEX_ARRAY_H_

#include <lamure/types.h>
#include <lamure/platform_gl.h>

#include <GL/glew.h>
#include <GL/freeglut.h>

#include <vector>

namespace lamure {
namespace gl {

class vertex_array_t {
public:
  
  enum attribute_t {
    FLOAT = 0,
    VEC2F = 1,
    VEC3F = 2,
    VEC4F = 3,
    UBYTE = 4
  };

  vertex_array_t(uint32_t num_elements);
  ~vertex_array_t();

  void add_attribute(const attribute_t attribute, uint32_t layout_location, bool normalize = false);
  void declare_attributes(uint32_t location_offset = 0) const;
  void undeclare_attributes(uint32_t location_offset = 0) const;

protected:

  struct attribute_descr_t {
    attribute_t attribute_;
    uint32_t location_; //vertex layout location of this attribute
    uint32_t bytes_per_index_; //bytes of this attribute
    uint32_t offset_; //offset from the first element
    uint32_t num_components_; //e.g. 3 for float3
    uint32_t normalize_;
  };

  std::vector<attribute_descr_t> attributes_;

private:

  uint32_t element_size_;
  uint32_t element_components_;

};

} // namespace gl
} // namespace lamure

#endif
