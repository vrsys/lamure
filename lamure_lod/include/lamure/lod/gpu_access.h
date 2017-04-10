// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef LAMURE_LOD_GPU_ACCESS_H
#define LAMURE_LOD_GPU_ACCESS_H

#include <lamure/types.h>
#include <lamure/lod/config.h>
#include <lamure/lod/bvh.h>
#include <lamure/gl/vertex_array.h>
#include <lamure/gl/array_buffer.h>
#include <lamure/assert.h>
#include <lamure/platform_lod.h>

#include <memory>

namespace lamure {
namespace lod {

class LOD_DLL gpu_access
{
public:
    gpu_access(const slot_t num_slots, const uint32_t num_surfels_per_node, bool create_layout = true);
    ~gpu_access();

    const slot_t num_slots() const { return num_slots_; };
    const size_t size_of_surfel() const { return size_of_surfel_; };
    const size_t size_of_slot() const { return size_of_slot_; };

    char* map();
    void unmap();
    const bool is_mapped() const { return is_mapped_; };
    const bool has_layout() const { return has_layout_; };

    lamure::gl::array_buffer_t* get_buffer();
    lamure::gl::vertex_array_t* get_memory(bvh::primitive_type type);

    static const size_t query_video_memory_in_mb();


private:
    slot_t num_slots_;
    size_t size_of_slot_;
    size_t size_of_surfel_;

    bool is_mapped_;
    bool has_layout_;

    lamure::gl::vertex_array_t* pcl_memory_;
    lamure::gl::vertex_array_t* tri_memory_;
    lamure::gl::array_buffer_t* buffer_;
};


}
}

#endif
