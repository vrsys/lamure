// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/ren/gpu_access.h>

#include <boost/shared_ptr.hpp>

#include <scm/gl_core.h>
#include <scm/gl_core/render_device.h>
#include <scm/gl_core/render_device/opengl/gl_core.h>
#include <scm/gl_core/render_device/device.h>

namespace lamure
{
namespace ren
{

GpuAccess::GpuAccess(scm::gl::render_device_ptr device,
                     const slot_t num_slots,
                     const uint32_t num_surfels_per_node,
                     bool create_layout)
: num_slots_(num_slots),
  size_of_surfel_(8*sizeof(float)),
  is_mapped_(false),
  has_layout_(create_layout) {

    assert(device);
    assert(sizeof(float) == 4);

    num_slots_ = num_slots;
    size_of_slot_ = num_surfels_per_node * size_of_surfel_;

    buffer_ = device->create_buffer(scm::gl::BIND_VERTEX_BUFFER,
                                    scm::gl::USAGE_DYNAMIC_COPY,
                                    num_slots_ * size_of_slot_,
                                    0);

    if (has_layout_) {
        memory_ = device->create_vertex_array(scm::gl::vertex_format
            (0, 0, scm::gl::TYPE_VEC3F, size_of_surfel_)
            (0, 1, scm::gl::TYPE_UBYTE, size_of_surfel_, scm::gl::INT_FLOAT_NORMALIZE)
            (0, 2, scm::gl::TYPE_UBYTE, size_of_surfel_, scm::gl::INT_FLOAT_NORMALIZE)
            (0, 3, scm::gl::TYPE_UBYTE, size_of_surfel_, scm::gl::INT_FLOAT_NORMALIZE)
            (0, 4, scm::gl::TYPE_UBYTE, size_of_surfel_, scm::gl::INT_FLOAT_NORMALIZE)
            (0, 5, scm::gl::TYPE_FLOAT, size_of_surfel_)
            (0, 6, scm::gl::TYPE_VEC3F, size_of_surfel_),
            boost::assign::list_of(buffer_));
    }

    device->main_context()->apply();

#ifdef LAMURE_ENABLE_INFO
    std::cout << "PLOD: gpu-cache size (MB): " << buffer_->descriptor()._size / 1024 / 1024 << std::endl;
#endif

}

GpuAccess::
~GpuAccess() {
    buffer_.reset();
    if (has_layout_) {
        memory_.reset();
    }
}

char* GpuAccess::
Map(scm::gl::render_device_ptr const& device) {
    if (!is_mapped_) {
        assert(device);
        is_mapped_ = true;
        return (char*)device->main_context()->map_buffer(buffer_, scm::gl::ACCESS_READ_WRITE);
    }
    return nullptr;
}

void GpuAccess::
Unmap(scm::gl::render_device_ptr const& device) {
    if (is_mapped_) {
        assert(device);
        device->main_context()->unmap_buffer(buffer_);
        is_mapped_ = false;
    }
}

const size_t GpuAccess::
QueryVideoRamInMb(scm::gl::render_device_ptr const& device) {
  int size_in_kb;
  device->opengl_api().glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &size_in_kb);
  //glGetIntegerv(0x9048/*GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX*/, &size_in_kb);
  return size_t(size_in_kb) / 1024;
}

}
}

