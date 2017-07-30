// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/ren/config.h>
#include <lamure/ren/cut_database.h>
#include <lamure/ren/gpu_context.h>
#include <lamure/ren/model_database.h>
#include <lamure/ren/policy.h>
#include <scm/gl_core/render_device/opengl/gl_core.h>

namespace lamure
{
namespace ren
{
gpu_context::gpu_context(const context_t context_id)
    : context_id_(context_id), is_created_(false), temp_buffer_a_(nullptr), temp_buffer_b_(nullptr), primary_buffer_(nullptr), temporary_storages_(temporary_storages(nullptr, nullptr)),
      temporary_storages_provenance_(temporary_storages(nullptr, nullptr)), upload_budget_in_nodes_(LAMURE_DEFAULT_UPLOAD_BUDGET), render_budget_in_nodes_(LAMURE_DEFAULT_VIDEO_MEMORY_BUDGET)
{
}

gpu_context::~gpu_context()
{
    temporary_storages_ = temporary_storages(nullptr, nullptr);
    temporary_storages_provenance_ = temporary_storages(nullptr, nullptr);

    if(temp_buffer_a_)
    {
        delete temp_buffer_a_;
        temp_buffer_a_ = nullptr;
    }

    if(temp_buffer_b_)
    {
        delete temp_buffer_b_;
        temp_buffer_b_ = nullptr;
    }

    if(primary_buffer_)
    {
        delete primary_buffer_;
        primary_buffer_ = nullptr;
    }
}

void gpu_context::create(scm::gl::render_device_ptr device)
{
    assert(device);
    if(is_created_)
    {
        return;
    }
    is_created_ = true;

    test_video_memory(device);

    model_database *database = model_database::get_instance();

    temp_buffer_a_ = new gpu_access(device, upload_budget_in_nodes_, database->get_primitives_per_node(), false);
    temp_buffer_b_ = new gpu_access(device, upload_budget_in_nodes_, database->get_primitives_per_node(), false);
    primary_buffer_ = new gpu_access(device, render_budget_in_nodes_, database->get_primitives_per_node(), true);

    map_temporary_storage(cut_database_record::temporary_buffer::BUFFER_A, device);
    map_temporary_storage(cut_database_record::temporary_buffer::BUFFER_B, device);
}

void gpu_context::create(scm::gl::render_device_ptr device, Data_Provenance data_provenance)
{
    assert(device);
    if(is_created_)
    {
        return;
    }
    is_created_ = true;

    test_video_memory(device);

    model_database *database = model_database::get_instance();
    temp_buffer_a_ = new gpu_access(device, upload_budget_in_nodes_, database->get_primitives_per_node(), data_provenance, false);
    temp_buffer_b_ = new gpu_access(device, upload_budget_in_nodes_, database->get_primitives_per_node(), data_provenance, false);
    primary_buffer_ = new gpu_access(device, render_budget_in_nodes_, database->get_primitives_per_node(), data_provenance, true);

    map_temporary_storage(cut_database_record::temporary_buffer::BUFFER_A, device, data_provenance);
    map_temporary_storage(cut_database_record::temporary_buffer::BUFFER_B, device, data_provenance);

    int first_error = device->opengl_api().glGetError();
    if(first_error != 0)
    {
        std::cout << "------------------------------ DISPATCH ERROR CODE gpu_context::create: " << first_error << std::endl;
    }
    else
    {
        std::cout << "------------------------------ no error inside gpu_context::create" << std::endl;
    }
}

void gpu_context::test_video_memory(scm::gl::render_device_ptr device)
{
    model_database *database = model_database::get_instance();
    policy *policy = policy::get_instance();

    float safety = 0.75;
    size_t video_ram_free_in_mb = gpu_access::query_video_memory_in_mb(device) * safety;

    size_t render_budget_in_mb = policy->render_budget_in_mb();

    if(policy->out_of_core_budget_in_mb() == 0)
    {
        std::cout << "##### Total free video memory (" << video_ram_free_in_mb << " MB) will be used for the render budget #####" << std::endl;
        render_budget_in_mb = video_ram_free_in_mb;
    }
    else if(video_ram_free_in_mb < render_budget_in_mb)
    {
        std::cout << "##### The specified render budget is too large! " << video_ram_free_in_mb << " MB will be used for the render budget #####" << std::endl;
        render_budget_in_mb = video_ram_free_in_mb;
    }
    else
    {
        std::cout << "##### " << policy->render_budget_in_mb() << " MB will be used for the render budget #####" << std::endl;
    }
    long node_size_total = database->get_slot_size();
    render_budget_in_nodes_ = (render_budget_in_mb * 1024 * 1024) / node_size_total;

    // render_budget_in_mb = policy->render_budget_in_mb();

    // // render_budget_in_mb = render_budget_in_mb < LAMURE_MIN_VIDEO_MEMORY_BUDGET ? LAMURE_MIN_VIDEO_MEMORY_BUDGET : render_budget_in_mb;
    // render_budget_in_mb = render_budget_in_mb > video_ram_free_in_mb * 0.75 ? video_ram_free_in_mb * 0.75 : render_budget_in_mb;

    // render_budget_in_nodes_ = (render_budget_in_mb * 1024u * 1024u) / (database->get_primitives_per_node() * sizeof(data_provenance) + database->get_slot_size());
    // std::cout << "RENDER2: " << render_budget_in_nodes_ << std::endl;

    size_t max_upload_budget_in_mb = policy->max_upload_budget_in_mb();
    max_upload_budget_in_mb = max_upload_budget_in_mb < LAMURE_MIN_UPLOAD_BUDGET ? LAMURE_MIN_UPLOAD_BUDGET : max_upload_budget_in_mb;
    max_upload_budget_in_mb = max_upload_budget_in_mb > video_ram_free_in_mb * 0.125 ? video_ram_free_in_mb * 0.125 : max_upload_budget_in_mb;

    upload_budget_in_nodes_ = (max_upload_budget_in_mb * 1024u * 1024u) / node_size_total;

#if 1
// upload_budget_in_nodes_ = max_upload_budget_in_nodes/4;

#else
    gpu_access *test_temp = new gpu_access(device, 1, database->surfels_per_node(), false);
    gpu_access *test_main = new gpu_access(device, 1, database->surfels_per_node(), true);
    LodPointCloud::serializedsurfel *node_data = (LodPointCloud::serializedsurfel *)new char[size_of_node_in_bytes];
    memset((char *)node_data, 0, size_of_node_in_bytes);
    char *mapped_temp = test_temp->map(device);
    memcpy(mapped_temp, node_data, size_of_node_in_bytes);
    test_temp->unmap(device);

    auto frame_duration_in_ns = boost::timer::nanosecond_type(16.0 * 1000 * 1000);

    boost::timer::cpu_timer upload_timer;

    unsigned int iteration = 0;
    while(true)
    {
        upload_timer.start();

        for(unsigned int i = 0; i < upload_budget_in_nodes_; ++i)
        {
            size_t offset_in_temp_VBO = 0;
            size_t offset_in_render_VBO = 0;
            device->main_context()->copy_buffer_data(test_main->buffer(), test_temp->buffer(), offset_in_render_VBO, offset_in_temp_VBO, size_of_node_in_bytes);
        }

        upload_timer.stop();

        boost::timer::cpu_times const elapsed(upload_timer.elapsed());
        boost::timer::nanosecond_type const elapsed_ns(elapsed.system + elapsed.user);

        if(iteration++ > 1)
        {
            if(elapsed_ns < frame_duration_in_ns)
            {
                if(upload_budget_in_nodes_ < max_upload_budget_in_nodes)
                {
                    ++upload_budget_in_nodes_;
                }
                else
                {
                    break;
                }
            }
            else
            {
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
    std::cout << "lamure: context " << context_id_ << " render budget (MB): " << render_budget_in_mb << std::endl;
    std::cout << "lamure: context " << context_id_ << " upload budget (MB): " << max_upload_budget_in_mb << std::endl;
#endif
}

void gpu_context::test_video_memory(scm::gl::render_device_ptr device, Data_Provenance const &data_provenance)
{
    model_database *database = model_database::get_instance();
    policy *policy = policy::get_instance();

    float safety = 0.75;
    size_t video_ram_free_in_mb = gpu_access::query_video_memory_in_mb(device) * safety;

    size_t render_budget_in_mb = policy->render_budget_in_mb();

    if(policy->out_of_core_budget_in_mb() == 0)
    {
        std::cout << "##### Total free video memory (" << video_ram_free_in_mb << " MB) will be used for the render budget #####" << std::endl;
        render_budget_in_mb = video_ram_free_in_mb;
    }
    else if(video_ram_free_in_mb < render_budget_in_mb)
    {
        std::cout << "##### The specified render budget is too large! " << video_ram_free_in_mb << " MB will be used for the render budget #####" << std::endl;
        render_budget_in_mb = video_ram_free_in_mb;
    }
    else
    {
        std::cout << "##### " << policy->render_budget_in_mb() << " MB will be used for the render budget #####" << std::endl;
    }
    long node_size_total = database->get_primitives_per_node() * data_provenance.get_size_in_bytes() + database->get_slot_size();
    render_budget_in_nodes_ = (render_budget_in_mb * 1024 * 1024) / node_size_total;

    // render_budget_in_mb = policy->render_budget_in_mb();

    // // render_budget_in_mb = render_budget_in_mb < LAMURE_MIN_VIDEO_MEMORY_BUDGET ? LAMURE_MIN_VIDEO_MEMORY_BUDGET : render_budget_in_mb;
    // render_budget_in_mb = render_budget_in_mb > video_ram_free_in_mb * 0.75 ? video_ram_free_in_mb * 0.75 : render_budget_in_mb;

    // render_budget_in_nodes_ = (render_budget_in_mb * 1024u * 1024u) / (database->get_primitives_per_node() * sizeof(data_provenance) + database->get_slot_size());
    // std::cout << "RENDER2: " << render_budget_in_nodes_ << std::endl;

    size_t max_upload_budget_in_mb = policy->max_upload_budget_in_mb();
    max_upload_budget_in_mb = max_upload_budget_in_mb < LAMURE_MIN_UPLOAD_BUDGET ? LAMURE_MIN_UPLOAD_BUDGET : max_upload_budget_in_mb;
    max_upload_budget_in_mb = max_upload_budget_in_mb > video_ram_free_in_mb * 0.125 ? video_ram_free_in_mb * 0.125 : max_upload_budget_in_mb;

    upload_budget_in_nodes_ = (max_upload_budget_in_mb * 1024u * 1024u) / node_size_total;

#if 1
// upload_budget_in_nodes_ = max_upload_budget_in_nodes/4;

#else
    gpu_access *test_temp = new gpu_access(device, 1, database->surfels_per_node(), false);
    gpu_access *test_main = new gpu_access(device, 1, database->surfels_per_node(), true);
    LodPointCloud::serializedsurfel *node_data = (LodPointCloud::serializedsurfel *)new char[size_of_node_in_bytes];
    memset((char *)node_data, 0, size_of_node_in_bytes);
    char *mapped_temp = test_temp->map(device);
    memcpy(mapped_temp, node_data, size_of_node_in_bytes);
    test_temp->unmap(device);

    auto frame_duration_in_ns = boost::timer::nanosecond_type(16.0 * 1000 * 1000);

    boost::timer::cpu_timer upload_timer;

    unsigned int iteration = 0;
    while(true)
    {
        upload_timer.start();

        for(unsigned int i = 0; i < upload_budget_in_nodes_; ++i)
        {
            size_t offset_in_temp_VBO = 0;
            size_t offset_in_render_VBO = 0;
            device->main_context()->copy_buffer_data(test_main->buffer(), test_temp->buffer(), offset_in_render_VBO, offset_in_temp_VBO, size_of_node_in_bytes);
        }

        upload_timer.stop();

        boost::timer::cpu_times const elapsed(upload_timer.elapsed());
        boost::timer::nanosecond_type const elapsed_ns(elapsed.system + elapsed.user);

        if(iteration++ > 1)
        {
            if(elapsed_ns < frame_duration_in_ns)
            {
                if(upload_budget_in_nodes_ < max_upload_budget_in_nodes)
                {
                    ++upload_budget_in_nodes_;
                }
                else
                {
                    break;
                }
            }
            else
            {
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
    std::cout << "lamure: context " << context_id_ << " render budget (MB): " << render_budget_in_mb << std::endl;
    std::cout << "lamure: context " << context_id_ << " upload budget (MB): " << max_upload_budget_in_mb << std::endl;
#endif
}

scm::gl::buffer_ptr gpu_context::get_context_buffer(scm::gl::render_device_ptr device)
{
    if(!is_created_)
        create(device);

    assert(device);
    return primary_buffer_->get_buffer();
}

scm::gl::buffer_ptr gpu_context::get_context_buffer(scm::gl::render_device_ptr device, Data_Provenance const &data_provenance)
{
    if(!is_created_)
        create(device, data_provenance);

    assert(device);

    return primary_buffer_->get_buffer();
}

scm::gl::vertex_array_ptr gpu_context::get_context_memory(bvh::primitive_type type, scm::gl::render_device_ptr device)
{
    if(!is_created_)
        create(device);

    assert(device);

    return primary_buffer_->get_memory(type);
}

scm::gl::vertex_array_ptr gpu_context::get_context_memory(bvh::primitive_type type, scm::gl::render_device_ptr device, Data_Provenance const &data_provenance)
{
    if(!is_created_)
        create(device, data_provenance);

    assert(device);

    return primary_buffer_->get_memory(type);
}

void gpu_context::map_temporary_storage(const cut_database_record::temporary_buffer &buffer, scm::gl::render_device_ptr device)
{
    if(!is_created_)
        create(device);

    assert(device);

    switch(buffer)
    {
    case cut_database_record::temporary_buffer::BUFFER_A:
        if(!temp_buffer_a_->is_mapped())
        {
            temporary_storages_.storage_a_ = temp_buffer_a_->map(device);
            temporary_storages_provenance_.storage_a_ = temp_buffer_a_->map_provenance(device);
        }
        return;
        break;

    case cut_database_record::temporary_buffer::BUFFER_B:
        if(!temp_buffer_b_->is_mapped())
        {
            temporary_storages_.storage_b_ = temp_buffer_b_->map(device);
            temporary_storages_provenance_.storage_b_ = temp_buffer_b_->map_provenance(device);
        }
        return;
        break;

    default:
        break;
    }

    throw std::runtime_error("lamure: Failed to map temporary buffer on context: " + context_id_);
}

void gpu_context::map_temporary_storage(const cut_database_record::temporary_buffer &buffer, scm::gl::render_device_ptr device, Data_Provenance const &data_provenance)
{
    if(!is_created_)
        create(device, data_provenance);

    assert(device);

    int first_error = 5;

    switch(buffer)
    {
    case cut_database_record::temporary_buffer::BUFFER_A:
        if(!temp_buffer_a_->is_mapped())
        {
            temporary_storages_.storage_a_ = temp_buffer_a_->map(device);

            // temporary_storages_provenance_.storage_a_ = temp_buffer_a_->map_provenance(device);
            first_error = device->opengl_api().glGetError();
            if(first_error != 0)
            {
                std::cout << "------------------------------ DISPATCH ERROR CODE gpu_context::map_temporary_storage: " << first_error << std::endl;
            }
            else
            {
                std::cout << "------------------------------ no error inside gpu_context::map_temporary_storage" << std::endl;
            }
        }

        return;
        break;

    case cut_database_record::temporary_buffer::BUFFER_B:
        if(!temp_buffer_b_->is_mapped())
        {
            first_error = device->opengl_api().glGetError();
            if(first_error != 0)
            {
                std::cout << "------------------------------ DISPATCH ERROR CODE gpu_context::map_temporary_storage2: " << first_error << std::endl;
            }
            else
            {
                std::cout << "------------------------------ no error inside gpu_context::map_temporary_storage2" << std::endl;
            }
            temporary_storages_.storage_b_ = temp_buffer_b_->map(device);
            // temporary_storages_provenance_.storage_b_ = temp_buffer_b_->map_provenance(device);
        }

        return;
        break;

    default:
        break;
    }

    throw std::runtime_error("lamure: Failed to map temporary buffer on context: " + context_id_);
}

void gpu_context::unmap_temporary_storage(const cut_database_record::temporary_buffer &buffer, scm::gl::render_device_ptr device)
{
    if(!is_created_)
        create(device);

    assert(device);

    switch(buffer)
    {
    case cut_database_record::temporary_buffer::BUFFER_A:
        if(temp_buffer_a_->is_mapped())
        {
            temp_buffer_a_->unmap(device);
            temp_buffer_a_->unmap_provenance(device);
        }
        break;

    case cut_database_record::temporary_buffer::BUFFER_B:
        if(temp_buffer_b_->is_mapped())
        {
            temp_buffer_b_->unmap(device);
            temp_buffer_b_->unmap_provenance(device);
        }
        break;

    default:
        break;
    }
}

void gpu_context::unmap_temporary_storage(const cut_database_record::temporary_buffer &buffer, scm::gl::render_device_ptr device, Data_Provenance const &data_provenance)
{
    if(!is_created_)
        create(device, data_provenance);

    assert(device);

    switch(buffer)
    {
    case cut_database_record::temporary_buffer::BUFFER_A:
        if(temp_buffer_a_->is_mapped())
        {
            temp_buffer_a_->unmap(device);
            // temp_buffer_a_->unmap_provenance(device);
        }
        break;

    case cut_database_record::temporary_buffer::BUFFER_B:
        if(temp_buffer_b_->is_mapped())
        {
            temp_buffer_b_->unmap(device);
            // temp_buffer_b_->unmap_provenance(device);
        }
        break;

    default:
        break;
    }
}

// returns true if any node has been uploaded; false otherwise
bool gpu_context::update_primary_buffer(const cut_database_record::temporary_buffer &from_buffer, scm::gl::render_device_ptr device)
{
    if(!is_created_)
        create(device);

    assert(device);

    model_database *database = model_database::get_instance();

    cut_database *cuts = cut_database::get_instance();

    size_t uploaded_nodes = 0;

    switch(from_buffer)
    {
    case cut_database_record::temporary_buffer::BUFFER_A:
    {
        if(temp_buffer_a_->is_mapped())
        {
            throw std::runtime_error("lamure: gpu_context::Failed to transfer nodes into main memory on context: " + context_id_);
        }
        std::vector<cut_database_record::slot_update_desc> &transfer_descr_list = cuts->get_updated_set(context_id_);
        if(!transfer_descr_list.empty())
        {
            uploaded_nodes += transfer_descr_list.size();

            for(const auto &transfer_desc : transfer_descr_list)
            {
                size_t offset_in_temp_VBO = transfer_desc.src_ * database->get_slot_size();
                size_t offset_in_render_VBO = transfer_desc.dst_ * database->get_slot_size();
                device->main_context()->copy_buffer_data(primary_buffer_->get_buffer(), temp_buffer_a_->get_buffer(), offset_in_render_VBO, offset_in_temp_VBO, database->get_slot_size());
            }
        }
        break;
    }

    case cut_database_record::temporary_buffer::BUFFER_B:
    {
        if(temp_buffer_b_->is_mapped())
        {
            throw std::runtime_error("lamure: gpu_context::Failed to transfer nodes into main memory on context: " + context_id_);
        }
        std::vector<cut_database_record::slot_update_desc> &transfer_descr_list = cuts->get_updated_set(context_id_);
        if(!transfer_descr_list.empty())
        {
            uploaded_nodes += transfer_descr_list.size();

            for(const auto &transfer_desc : transfer_descr_list)
            {
                size_t offset_in_temp_VBO = transfer_desc.src_ * database->get_slot_size();
                size_t offset_in_render_VBO = transfer_desc.dst_ * database->get_slot_size();
                device->main_context()->copy_buffer_data(primary_buffer_->get_buffer(), temp_buffer_b_->get_buffer(), offset_in_render_VBO, offset_in_temp_VBO, database->get_slot_size());
            }
        }
        break;
    }
    default:
        break;
    }

    return uploaded_nodes != 0;
}
// returns true if any node has been uploaded; false otherwise
bool gpu_context::update_primary_buffer(const cut_database_record::temporary_buffer &from_buffer, scm::gl::render_device_ptr device, Data_Provenance const &data_provenance)
{
    if(!is_created_)
        create(device, data_provenance);

    assert(device);

    model_database *database = model_database::get_instance();

    cut_database *cuts = cut_database::get_instance();

    size_t uploaded_nodes = 0;

    switch(from_buffer)
    {
    case cut_database_record::temporary_buffer::BUFFER_A:
    {
        if(temp_buffer_a_->is_mapped())
        {
            throw std::runtime_error("lamure: gpu_context::Failed to transfer nodes into main memory on context: " + context_id_);
        }
        std::vector<cut_database_record::slot_update_desc> &transfer_descr_list = cuts->get_updated_set(context_id_);
        if(!transfer_descr_list.empty())
        {
            uploaded_nodes += transfer_descr_list.size();

            for(const auto &transfer_desc : transfer_descr_list)
            {
                size_t offset_in_temp_VBO = transfer_desc.src_ * database->get_slot_size();
                size_t offset_in_render_VBO = transfer_desc.dst_ * database->get_slot_size();
                device->main_context()->copy_buffer_data(primary_buffer_->get_buffer(), temp_buffer_a_->get_buffer(), offset_in_render_VBO, offset_in_temp_VBO, database->get_slot_size());

                int first_error = device->opengl_api().glGetError();
                if(first_error != 0)
                {
                    std::cout << "------------------------------ DISPATCH ERROR CODE gpu_context::update_primary_buffer: " << first_error << std::endl;
                }
                else
                {
                    std::cout << "------------------------------ no error inside gpu_context::update_primary_buffer" << std::endl;
                }
                size_t offset_in_temp_VBO_provenance = transfer_desc.src_ * database->get_primitives_per_node() * data_provenance.get_size_in_bytes();
                size_t offset_in_render_VBO_provenance = transfer_desc.dst_ * database->get_primitives_per_node() * data_provenance.get_size_in_bytes();
                device->main_context()->copy_buffer_data(primary_buffer_->get_buffer_provenance(), temp_buffer_a_->get_buffer_provenance(), offset_in_render_VBO_provenance,
                                                         offset_in_temp_VBO_provenance, database->get_primitives_per_node() * data_provenance.get_size_in_bytes());
                first_error = device->opengl_api().glGetError();
                if(first_error != 0)
                {
                    std::cout << "------------------------------ DISPATCH ERROR CODE gpu_context::update_primary_buffer_prov: " << first_error << std::endl;
                }
                else
                {
                    std::cout << "------------------------------ no error inside gpu_context::update_primary_buffer_prov" << std::endl;
                }
            }
        }
        break;
    }

    case cut_database_record::temporary_buffer::BUFFER_B:
    {
        if(temp_buffer_b_->is_mapped())
        {
            throw std::runtime_error("lamure: gpu_context::Failed to transfer nodes into main memory on context: " + context_id_);
        }
        std::vector<cut_database_record::slot_update_desc> &transfer_descr_list = cuts->get_updated_set(context_id_);
        if(!transfer_descr_list.empty())
        {
            uploaded_nodes += transfer_descr_list.size();

            for(const auto &transfer_desc : transfer_descr_list)
            {
                int first_error = device->opengl_api().glGetError();
                if(first_error != 0)
                {
                    std::cout << "------------------------------ DISPATCH ERROR CODE gpu_context::update_primary_buffer1: " << first_error << std::endl;
                }
                else
                {
                    std::cout << "------------------------------ no error inside gpu_context::update_primary_buffer1" << std::endl;
                }
                size_t offset_in_temp_VBO = transfer_desc.src_ * database->get_slot_size();
                size_t offset_in_render_VBO = transfer_desc.dst_ * database->get_slot_size();
                device->main_context()->copy_buffer_data(primary_buffer_->get_buffer(), temp_buffer_b_->get_buffer(), offset_in_render_VBO, offset_in_temp_VBO, database->get_slot_size());

                size_t offset_in_temp_VBO_provenance = transfer_desc.src_ * database->get_primitives_per_node() * data_provenance.get_size_in_bytes();
                size_t offset_in_render_VBO_provenance = transfer_desc.dst_ * database->get_primitives_per_node() * data_provenance.get_size_in_bytes();
                device->main_context()->copy_buffer_data(primary_buffer_->get_buffer_provenance(), temp_buffer_b_->get_buffer_provenance(), offset_in_render_VBO_provenance,
                                                         offset_in_temp_VBO_provenance, database->get_primitives_per_node() * data_provenance.get_size_in_bytes());
                first_error = device->opengl_api().glGetError();
                if(first_error != 0)
                {
                    std::cout << "------------------------------ DISPATCH ERROR CODE gpu_context::update_primary_buffer2: " << first_error << std::endl;
                }
                else
                {
                    std::cout << "------------------------------ no error inside gpu_context::update_primary_buffer2" << std::endl;
                }
            }
        }
        break;
    }
    default:
        break;
    }

    return uploaded_nodes != 0;
}
}
}
