// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef LAMURE_REN_GPU_ACCESS_H
#define LAMURE_REN_GPU_ACCESS_H

#include <lamure/types.h>
#include <lamure/ren/config.h>

#include <boost/assign/list_of.hpp>
#include <memory>
#include <scm/core.h>
#include <scm/gl_core.h>

namespace lamure
{
namespace ren
{

class gpu_access
{
public:
    gpu_access(scm::gl::render_device_ptr device, const slot_t num_slots, const uint32_t num_surfels_per_node, bool create_layout = true);
    ~gpu_access();

    const slot_t num_slots() const { return num_slots_; };
    const size_t size_of_surfel() const { return size_of_surfel_; };
    const size_t size_of_slot() const { return size_of_slot_; };

    char* Map(scm::gl::render_device_ptr const& device);
    void Unmap(scm::gl::render_device_ptr const& device);
    const bool is_mapped() const { return is_mapped_; };
    const bool has_layout() const { return has_layout_; };

    scm::gl::buffer_ptr buffer() { return buffer_; };
    scm::gl::vertex_array_ptr memory() { return memory_; };

    static const size_t query_video_memory_in_mb(scm::gl::render_device_ptr const& device);


private:
    slot_t num_slots_;
    size_t size_of_slot_;
    size_t size_of_surfel_;

    bool is_mapped_;
    bool has_layout_;

    scm::gl::vertex_array_ptr memory_;
    scm::gl::buffer_ptr buffer_;
};


}
}

#endif
