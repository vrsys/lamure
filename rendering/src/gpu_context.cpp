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

gpu_context::
gpu_context(const context_t context_id)
: context_id_(context_id),
  is_created_(false),
  temp_buffer_a_(nullptr),
  temp_buffer_b_(nullptr),
  primary_buffer_(nullptr),
  temporary_storages_(temporary_storages(nullptr, nullptr)),
  upload_budget_in_nodes_(LAMURE_DEFAULT_UPLOAD_BUDGET),
  render_budget_in_nodes_(LAMURE_DEFAULT_VIDEO_MEMORY_BUDGET) {

}


gpu_context::
~gpu_context() {
    temporary_storages_ = temporary_storages(nullptr, nullptr);

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

void gpu_context::
create(scm::gl::render_device_ptr device) {
    assert(device);
    if (is_created_) {
        return;
    }
    is_created_ = true;

    test_video_memory(device);

    model_database* database = model_database::get_instance();

    temp_buffer_a_ = new gpu_access(device, upload_budget_in_nodes_, database->surfels_per_node(), false);
    temp_buffer_b_ = new gpu_access(device, upload_budget_in_nodes_, database->surfels_per_node(), false);
    primary_buffer_ = new gpu_access(device, render_budget_in_nodes_, database->surfels_per_node(), true);

    map_temporary_storage(cut_database_record::temporary_buffer::BUFFER_A, device);
    map_temporary_storage(cut_database_record::temporary_buffer::BUFFER_B, device);
}

void gpu_context::
test_video_memory(scm::gl::render_device_ptr device) {
    model_database* database = model_database::get_instance();
    policy* policy = policy::get_instance();

    size_t size_of_node_in_bytes = database->size_of_surfel() * database->surfels_per_node();
    size_t render_budget_in_mb = policy->render_budget_in_mb();

    size_t video_ram_in_mb = gpu_access::query_video_memory_in_mb(device);
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
    gpu_access* test_temp = new gpu_access(device, 1, database->surfels_per_node(), false);
    gpu_access* test_main = new gpu_access(device, 1, database->surfels_per_node(), true);
    LodPointCloud::serializedsurfel* node_data = (LodPointCloud::serializedsurfel*)new char[size_of_node_in_bytes];
    memset((char*)node_data, 0, size_of_node_in_bytes);
    char* mapped_temp = test_temp->map(device);
    memcpy(mapped_temp, node_data, size_of_node_in_bytes);
    test_temp->unmap(device);

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

scm::gl::buffer_ptr gpu_context::
get_context_buffer(scm::gl::render_device_ptr device) {
    if (!is_created_)
        create(device);

    assert(device);

    return primary_buffer_->buffer();
}

scm::gl::vertex_array_ptr gpu_context::
get_context_memory(scm::gl::render_device_ptr device) {
    if (!is_created_)
        create(device);

    assert(device);

    return primary_buffer_->memory();
}

void gpu_context::
map_temporary_storage(const cut_database_record::temporary_buffer& buffer, scm::gl::render_device_ptr device) {
    if (!is_created_)
        create(device);

    assert(device);

    switch (buffer) {
        case cut_database_record::temporary_buffer::BUFFER_A:
            if (!temp_buffer_a_->is_mapped()) {
                temporary_storages_.storage_a_ = temp_buffer_a_->map(device);
            }
            return;
            break;

        case cut_database_record::temporary_buffer::BUFFER_B:
            if (!temp_buffer_b_->is_mapped()) {
                temporary_storages_.storage_b_ = temp_buffer_b_->map(device);
            }
            return;
            break;

        default: break;
    }

    throw std::runtime_error(
       "PLOD: Failed to map temporary buffer on context: " + context_id_);

}

void gpu_context::
unmap_temporary_storage(const cut_database_record::temporary_buffer& buffer, scm::gl::render_device_ptr device) {
    if (!is_created_)
        create(device);

    assert(device);

    switch (buffer) {
        case cut_database_record::temporary_buffer::BUFFER_A:
            if (temp_buffer_a_->is_mapped()) {
                temp_buffer_a_->unmap(device);
            }
            break;

        case cut_database_record::temporary_buffer::BUFFER_B:
            if (temp_buffer_b_->is_mapped()) {
                temp_buffer_b_->unmap(device);
            }
            break;

        default: break;
    }
}

//returns true if any node has been uploaded; false otherwise
bool gpu_context::
update_primary_buffer(const cut_database_record::temporary_buffer& from_buffer, scm::gl::render_device_ptr device) {
    if (!is_created_)
        create(device);

    assert(device);

    model_database* database = model_database::get_instance();

    size_t size_of_node_in_bytes = database->surfels_per_node() * database->size_of_surfel();

    cut_database* cuts = cut_database::get_instance();

    size_t uploaded_nodes = 0;

    switch (from_buffer) {
        case cut_database_record::temporary_buffer::BUFFER_A:
        {
            if (temp_buffer_a_->is_mapped()) {
                throw std::runtime_error(
                   "PLOD: gpu_context::Failed to transfer nodes into main memory on context: " + context_id_);
            }
            std::vector<cut_database_record::slot_update_desc>& transfer_descr_list = cuts->get_updated_set(context_id_);
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

        case cut_database_record::temporary_buffer::BUFFER_B:
        {
            if (temp_buffer_b_->is_mapped()) {
                throw std::runtime_error(
                   "PLOD: gpu_context::Failed to transfer nodes into main memory on context: " + context_id_);
            }
            std::vector<cut_database_record::slot_update_desc>& transfer_descr_list = cuts->get_updated_set(context_id_);
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

    return uploaded_nodes != 0;
}



}
}
