// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef LAMURE_REN_GPU_CONTEXT_H
#define LAMURE_REN_GPU_CONTEXT_H

#include <lamure/types.h>
#include <lamure/ren/gpu_access.h>
#include <lamure/ren/cut_database_record.h>

namespace lamure
{
namespace ren
{

class GpuContext
{

public:
    GpuContext(const context_t context_id);
    ~GpuContext();


    struct TemporaryStorages
    {
        TemporaryStorages(char* storage_a, char* storage_b)
        : storage_a_(storage_a), storage_b_(storage_b) {};

        char* storage_a_;
        char* storage_b_;
    };

    const context_t context_id() const { return context_id_; };
    const bool is_created() const { return is_created_; };

    TemporaryStorages temporary_storages() { return temporary_storages_; };

    scm::gl::buffer_ptr GetContextBuffer(scm::gl::render_device_ptr device);
    scm::gl::vertex_array_ptr GetContextMemory(scm::gl::render_device_ptr device);

    const node_t upload_budget_in_nodes() const { return upload_budget_in_nodes_; };
    const node_t render_budget_in_nodes() const { return render_budget_in_nodes_; };

    void MapTempStorage(const CutDatabaseRecord::TemporaryBuffer& buffer, scm::gl::render_device_ptr device);
    void UnmapTempStorage(const CutDatabaseRecord::TemporaryBuffer& buffer, scm::gl::render_device_ptr device);
    void UpdatePrimaryBuffer(const CutDatabaseRecord::TemporaryBuffer& from_buffer, scm::gl::render_device_ptr device);

    void Create(scm::gl::render_device_ptr device);

private:
    void TestVideoMemory(scm::gl::render_device_ptr device);

    context_t context_id_;

    bool is_created_;
    
    GpuAccess* temp_buffer_a_;
    GpuAccess* temp_buffer_b_;
    GpuAccess* primary_buffer_;

    TemporaryStorages temporary_storages_;
    node_t upload_budget_in_nodes_;
    node_t render_budget_in_nodes_;

};


}
}


#endif
