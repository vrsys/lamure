// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/ren/gpu_context.h>
#include <lamure/ren/model_database.h>
#include <lamure/ren/policy.h>
#include <lamure/ren/raw_point_cloud.h>
#include <lamure/ren/cut_database.h>
#include <lamure/ren/config.h>

namespace lamure
{
namespace ren
{

GpuContext::
GpuContext(const context_t context_id)
: context_id_(context_id),
  is_created_(false),
  temp_buffer_a_(nullptr),
  temp_buffer_b_(nullptr),
  primary_buffer_(nullptr),
  temporary_storages_(TemporaryStorages(nullptr, nullptr)),
  upload_budget_in_nodes_(LAMURE_DEFAULT_UPLOAD_BUDGET),
  render_budget_in_nodes_(LAMURE_DEFAULT_VIDEO_MEMORY_BUDGET) {

}


GpuContext::
~GpuContext() {
    temporary_storages_ = TemporaryStorages(nullptr, nullptr);

    if (temp_buffer_a_) {
        delete temp_buffer_a_;
        temp_buffer_a_ = nullptr;
    }

    if (temp_buffer_b_) {
        delete temp_buffer_b_;
        temp_buffer_b_ = nullptr;
    }

    if (primary_buffer_) {
        delete primary_buffer_;
        primary_buffer_ = nullptr;
    }

}

void GpuContext::
Create(scm::gl::render_device_ptr device) {
    assert(device);
    if (is_created_) {
        return;
    }
    is_created_ = true;

    TestVideoMemory(device);

    Modeldatabase* database = Modeldatabase::get_instance();

    temp_buffer_a_ = new GpuAccess(device, upload_budget_in_nodes_, database->surfels_per_node(), false);
    temp_buffer_b_ = new GpuAccess(device, upload_budget_in_nodes_, database->surfels_per_node(), false);
    primary_buffer_ = new GpuAccess(device, render_budget_in_nodes_, database->surfels_per_node(), true);

    MapTempStorage(CutdatabaseRecord::Temporarybuffer::BUFFER_A, device);
    MapTempStorage(CutdatabaseRecord::Temporarybuffer::BUFFER_B, device);
}

void GpuContext::
TestVideoMemory(scm::gl::render_device_ptr device) {
    Modeldatabase* database = Modeldatabase::get_instance();
    Policy* policy = Policy::get_instance();

    size_t size_of_node_in_bytes = database->size_of_surfel() * database->surfels_per_node();
    size_t render_budget_in_mb = policy->render_budget_in_mb();

    size_t video_ram_in_mb = GpuAccess::QueryVideoRamInMb(device);
    render_budget_in_mb = render_budget_in_mb < LAMURE_MIN_VIDEO_MEMORY_BUDGET ? LAMURE_MIN_VIDEO_MEMORY_BUDGET : render_budget_in_mb;
    render_budget_in_mb = render_budget_in_mb > video_ram_in_mb * 0.75 ? video_ram_in_mb * 0.75 : render_budget_in_mb;
    render_budget_in_nodes_ = (render_budget_in_mb * 1024u * 1024u) / size_of_node_in_bytes;

    size_t max_upload_budget_in_mb = policy->max_upload_budget_in_mb();
    max_upload_budget_in_mb = max_upload_budget_in_mb < LAMURE_MIN_UPLOAD_BUDGET ? LAMURE_MIN_UPLOAD_BUDGET : max_upload_budget_in_mb;
    max_upload_budget_in_mb = max_upload_budget_in_mb > video_ram_in_mb * 0.125 ? video_ram_in_mb * 0.125 : max_upload_budget_in_mb;
    size_t max_upload_budget_in_nodes = (max_upload_budget_in_mb * 1024u * 1024u) / size_of_node_in_bytes;

    upload_budget_in_nodes_ = max_upload_budget_in_nodes;
    

#if 1
   // upload_budget_in_nodes_ = max_upload_budget_in_nodes/4;

#else
    GpuAccess* test_temp = new GpuAccess(device, 1, database->surfels_per_node(), false);
    GpuAccess* test_main = new GpuAccess(device, 1, database->surfels_per_node(), true);
    lod_point_cloud::serialized_surfel* node_data = (lod_point_cloud::serialized_surfel*)new char[size_of_node_in_bytes];
    memset((char*)node_data, 0, size_of_node_in_bytes);
    char* mapped_temp = test_temp->Map(device);
    memcpy(mapped_temp, node_data, size_of_node_in_bytes);
    test_temp->Unmap(device);

    auto frame_duration_in_ns = boost::timer::nanosecond_type(16.0 * 1000 * 1000);

    boost::timer::cpu_timer upload_timer;

    unsigned int iteration = 0;
    while (true) {

        upload_timer.start();

        for (unsigned int i = 0; i < upload_budget_in_nodes_; ++i) {
            size_t offset_in_temp_VBO = 0;
            size_t offset_in_render_VBO = 0;
            device->main_context()->copy_buffer_data(test_main->buffer(), test_temp->buffer(), offset_in_render_VBO, offset_in_temp_VBO, size_of_node_in_bytes);
        }

        upload_timer.stop();

        boost::timer::cpu_times const elapsed(upload_timer.elapsed());
        boost::timer::nanosecond_type const elapsed_ns(elapsed.system + elapsed.user);

        if (iteration++ > 1) {
            if (elapsed_ns < frame_duration_in_ns) {
                if (upload_budget_in_nodes_ < max_upload_budget_in_nodes) {
                    ++upload_budget_in_nodes_;
                }
                else {
                    break;
                }
            }
            else {
                break;
            }
        }


    }

    delete test_temp;
    delete test_main;
    delete[] node_data;

    device->main_context()->apply();
#endif

#ifdef LAMURE_ENABLE_INFO
    std::cout << "PLOD: context " << context_id_ << " render budget (MB): " << render_budget_in_mb << std::endl;
    std::cout << "PLOD: context " << context_id_ << " upload budget (MB): " << max_upload_budget_in_mb << std::endl;
#endif

}

scm::gl::buffer_ptr GpuContext::
GetContextbuffer(scm::gl::render_device_ptr device) {
    if (!is_created_)
        Create(device);

    assert(device);

    return primary_buffer_->buffer();
}

scm::gl::vertex_array_ptr GpuContext::
GetContextMemory(scm::gl::render_device_ptr device) {
    if (!is_created_)
        Create(device);

    assert(device);

    return primary_buffer_->memory();
}

void GpuContext::
MapTempStorage(const CutdatabaseRecord::Temporarybuffer& buffer, scm::gl::render_device_ptr device) {
    if (!is_created_)
        Create(device);

    assert(device);

    switch (buffer) {
        case CutdatabaseRecord::Temporarybuffer::BUFFER_A:
            if (!temp_buffer_a_->is_mapped()) {
                temporary_storages_.storage_a_ = temp_buffer_a_->Map(device);
            }
            return;
            break;

        case CutdatabaseRecord::Temporarybuffer::BUFFER_B:
            if (!temp_buffer_b_->is_mapped()) {
                temporary_storages_.storage_b_ = temp_buffer_b_->Map(device);
            }
            return;
            break;

        default: break;
    }

    throw std::runtime_error(
       "PLOD: Failed to map temporary buffer on context: " + context_id_);

}

void GpuContext::
UnmapTempStorage(const CutdatabaseRecord::Temporarybuffer& buffer, scm::gl::render_device_ptr device) {
    if (!is_created_)
        Create(device);

    assert(device);

    switch (buffer) {
        case CutdatabaseRecord::Temporarybuffer::BUFFER_A:
            if (temp_buffer_a_->is_mapped()) {
                temp_buffer_a_->Unmap(device);
            }
            break;

        case CutdatabaseRecord::Temporarybuffer::BUFFER_B:
            if (temp_buffer_b_->is_mapped()) {
                temp_buffer_b_->Unmap(device);
            }
            break;

        default: break;
    }
}

void GpuContext::
UpdatePrimarybuffer(const CutdatabaseRecord::Temporarybuffer& from_buffer, scm::gl::render_device_ptr device) {
    if (!is_created_)
        Create(device);

    assert(device);

    Modeldatabase* database = Modeldatabase::get_instance();

    size_t size_of_node_in_bytes = database->surfels_per_node() * database->size_of_surfel();

    Cutdatabase* cuts = Cutdatabase::get_instance();

    size_t uploaded_nodes = 0;

    switch (from_buffer) {
        case CutdatabaseRecord::Temporarybuffer::BUFFER_A:
        {
            if (temp_buffer_a_->is_mapped()) {
                throw std::runtime_error(
                   "PLOD: GpuContext::Failed to transfer nodes into main memory on context: " + context_id_);
            }
            std::vector<CutdatabaseRecord::SlotUpdateDescr>& transfer_descr_list = cuts->GetUpdatedSet(context_id_);
            if (!transfer_descr_list.empty()) {
                uploaded_nodes += transfer_descr_list.size();

                for (const auto& transfer_desc : transfer_descr_list)
                {
                    size_t offset_in_temp_VBO = transfer_desc.src_ * size_of_node_in_bytes;
                    size_t offset_in_render_VBO = transfer_desc.dst_ * size_of_node_in_bytes;
                    device->main_context()->copy_buffer_data(primary_buffer_->buffer(), temp_buffer_a_->buffer(), offset_in_render_VBO, offset_in_temp_VBO, size_of_node_in_bytes);
                }
            }
            break;
        }

        case CutdatabaseRecord::Temporarybuffer::BUFFER_B:
        {
            if (temp_buffer_b_->is_mapped()) {
                throw std::runtime_error(
                   "PLOD: GpuContext::Failed to transfer nodes into main memory on context: " + context_id_);
            }
            std::vector<CutdatabaseRecord::SlotUpdateDescr>& transfer_descr_list = cuts->GetUpdatedSet(context_id_);
            if (!transfer_descr_list.empty()) {
                uploaded_nodes += transfer_descr_list.size();

                for (const auto& transfer_desc : transfer_descr_list) {
                    size_t offset_in_temp_VBO = transfer_desc.src_ * size_of_node_in_bytes;
                    size_t offset_in_render_VBO = transfer_desc.dst_ * size_of_node_in_bytes;
                    device->main_context()->copy_buffer_data(primary_buffer_->buffer(), temp_buffer_b_->buffer(), offset_in_render_VBO, offset_in_temp_VBO, size_of_node_in_bytes);
                }
            }
            break;
        }
        default: break;

    }

}



}
}
