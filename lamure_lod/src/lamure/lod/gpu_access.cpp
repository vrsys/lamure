// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/lod/gpu_access.h>

namespace lamure {
namespace lod {

gpu_access::gpu_access(const slot_t num_slots,
                       const uint32_t num_surfels_per_node,
                       bool create_layout)
: num_slots_(num_slots),
  size_of_surfel_(8*sizeof(float)),
  is_mapped_(false),
  has_layout_(create_layout) {

    ASSERT(sizeof(float) == 4);

    num_slots_ = num_slots;
    size_of_slot_ = num_surfels_per_node * size_of_surfel_;

    buffer_ = new lamure::gl::array_buffer_t(num_surfels_per_node * num_slots_, size_of_surfel_);

    if (has_layout_) {
        tri_memory_ = new lamure::gl::vertex_array_t(num_surfels_per_node * num_slots_);
        tri_memory_->add_attribute(lamure::gl::vertex_array_t::VEC3F, 0);
        tri_memory_->add_attribute(lamure::gl::vertex_array_t::VEC3F, 1);
        tri_memory_->add_attribute(lamure::gl::vertex_array_t::VEC2F, 2);

        pcl_memory_ = new lamure::gl::vertex_array_t(num_surfels_per_node * num_slots_);
        pcl_memory_->add_attribute(lamure::gl::vertex_array_t::VEC3F, 0);
        pcl_memory_->add_attribute(lamure::gl::vertex_array_t::UBYTE, 1, true);
        pcl_memory_->add_attribute(lamure::gl::vertex_array_t::UBYTE, 2, true);
        pcl_memory_->add_attribute(lamure::gl::vertex_array_t::UBYTE, 3, true);
        pcl_memory_->add_attribute(lamure::gl::vertex_array_t::UBYTE, 4, true);
        pcl_memory_->add_attribute(lamure::gl::vertex_array_t::FLOAT, 5);
        pcl_memory_->add_attribute(lamure::gl::vertex_array_t::VEC3F, 3);

    }

#ifdef LAMURE_ENABLE_INFO
    std::cout << "lamure: gpu-cache size (MB): " << (num_surfels_per_node * num_slots_ * size_of_surfel_) / 1024 / 1024 << std::endl;
#endif

}

gpu_access::
~gpu_access() {
    if (buffer_ != nullptr) {
      delete buffer_;
    }
    if (has_layout_) {
        if (pcl_memory_ != nullptr) {
          delete pcl_memory_;
        }
        if (tri_memory_ != nullptr) {
          delete tri_memory_;
        }
    }
}

char* gpu_access::
map() {
    if (!is_mapped_) {
        is_mapped_ = true;
        return (char*)buffer_->map(GL_READ_WRITE);
    }
    return nullptr;
}

void gpu_access::
unmap() {
    if (is_mapped_) {
        buffer_->unmap();
        is_mapped_ = false;
    }
}

const lamure::gl::vertex_array_t* gpu_access::
get_memory(bvh::primitive_type type) {
  switch (type) {
    case bvh::primitive_type::POINTCLOUD:
      return pcl_memory_;
    case bvh::primitive_type::TRIMESH:
      return tri_memory_;
    default: break;
  }
  throw std::runtime_error(
    "lamure: gpu_access::Invalid primitive type");
  return nullptr;

};

const size_t gpu_access::
query_video_memory_in_mb() {
  int size_in_kb;
  glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &size_in_kb);
  //glGetIntegerv(0x9048/*GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX*/, &size_in_kb);
  return size_t(size_in_kb) / 1024;
}

}
}

