// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/ren/gpu_access.h>
#include <lamure/ren/Data_Provenance.h>

#include <boost/shared_ptr.hpp>

#include <scm/gl_core.h>
#include <scm/gl_core/render_device.h>
#include <scm/gl_core/render_device/opengl/gl_core.h>
#include <scm/gl_core/render_device/device.h>

namespace lamure
{
namespace ren
{

gpu_access::gpu_access(scm::gl::render_device_ptr device,
                     const slot_t num_slots,
                     const uint32_t num_surfels_per_node,
                     bool create_layout)
: num_slots_(num_slots),
  // size_of_surfel_(9*sizeof(float)),
  size_of_surfel_(8*sizeof(float)),
  is_mapped_(false),
  is_mapped_provenance_(false),
  has_layout_(create_layout) {

    assert(device);
    assert(sizeof(float) == 4);


    Data_Provenance data_provenance;
    lamure::ren::Item_Provenance item_float(
        // lamure::ren::Item_Provenance::type_item::TYPE_VEC3F,
        lamure::ren::Item_Provenance::type_item::TYPE_FLOAT,
        lamure::ren::Item_Provenance::visualization_item::VISUALIZATION_COLOR
    );
    data_provenance.add_item(item_float);


    num_slots_ = num_slots;
    std::cout << "slots: " << num_slots << std::endl;
    size_of_slot_ = num_surfels_per_node * size_of_surfel_;
    std::cout << "size of surfel: " << size_of_surfel_ << std::endl;
    std::cout << "size of slot: " << size_of_slot_ << std::endl;
    std::cout << "size of prov: " << data_provenance.get_size_in_bytes() << std::endl;

    buffer_ = device->create_buffer(scm::gl::BIND_VERTEX_BUFFER,
                                    scm::gl::USAGE_DYNAMIC_COPY,
                                    num_slots_ * size_of_slot_,
                                    0);

    size_of_slot_provenance_ = num_surfels_per_node * data_provenance.get_size_in_bytes();
    buffer_provenance_ = device->create_buffer(scm::gl::BIND_VERTEX_BUFFER,
                                    scm::gl::USAGE_DYNAMIC_COPY,
                                    num_slots_ * size_of_slot_provenance_,
                                    0);
    if (has_layout_) {
        std::vector<scm::gl::vertex_format::element> vertex_format;
        vertex_format.push_back(scm::gl::vertex_format::element(0, 0, scm::gl::TYPE_VEC3F, size_of_surfel_));
        vertex_format.push_back(scm::gl::vertex_format::element(0, 1, scm::gl::TYPE_UBYTE, size_of_surfel_, scm::gl::INT_FLOAT_NORMALIZE));
        vertex_format.push_back(scm::gl::vertex_format::element(0, 2, scm::gl::TYPE_UBYTE, size_of_surfel_, scm::gl::INT_FLOAT_NORMALIZE));
        vertex_format.push_back(scm::gl::vertex_format::element(0, 3, scm::gl::TYPE_UBYTE, size_of_surfel_, scm::gl::INT_FLOAT_NORMALIZE));
        vertex_format.push_back(scm::gl::vertex_format::element(0, 4, scm::gl::TYPE_UBYTE, size_of_surfel_, scm::gl::INT_FLOAT_NORMALIZE));
        vertex_format.push_back(scm::gl::vertex_format::element(0, 5, scm::gl::TYPE_FLOAT, size_of_surfel_));
        vertex_format.push_back(scm::gl::vertex_format::element(0, 6, scm::gl::TYPE_VEC3F, size_of_surfel_));
        // vertex_format.push_back(scm::gl::vertex_format::element(1, 7, scm::gl::TYPE_DOUBLE, 8));

        int counter = 7;
        for(Item_Provenance item:  data_provenance.get_items()) 
        // for(std::vector<Item_Provenance>::iterator it = data_provenance.get_items().begin(); it != data_provenance.get_items().end(); ++it)
        {
            // Item_Provenance &item = (*it);
            if(item.get_visualization() == Item_Provenance::visualization_item::VISUALIZATION_COLOR)
            {
                std::cout << item.get_type() << std::endl;
                switch(item.get_type()) 
                {
                    case Item_Provenance::type_item::TYPE_INT: vertex_format.push_back(scm::gl::vertex_format::element(1, counter, scm::gl::TYPE_INT, data_provenance.get_size_in_bytes()));std::cout << item.get_type() << std::endl; break;
                    // case Item_Provenance::type_item::TYPE_FLOAT: vertex_format.push_back(scm::gl::vertex_format::element(1, counter, scm::gl::TYPE_DOUBLE, data_provenance.get_size_in_bytes())); break;
                    case Item_Provenance::type_item::TYPE_FLOAT: vertex_format.push_back(scm::gl::vertex_format::element(1, counter, scm::gl::TYPE_FLOAT, data_provenance.get_size_in_bytes()));std::cout << item.get_type() << std::endl; break;
                    case Item_Provenance::type_item::TYPE_VEC3I: vertex_format.push_back(scm::gl::vertex_format::element(1, counter, scm::gl::TYPE_VEC3I, data_provenance.get_size_in_bytes()));std::cout << item.get_type() << std::endl; break;
                    case Item_Provenance::type_item::TYPE_VEC3F: vertex_format.push_back(scm::gl::vertex_format::element(1, counter, scm::gl::TYPE_VEC3F, data_provenance.get_size_in_bytes()));std::cout << item.get_type() << std::endl; break;
                }
                counter++;
            }
        }

        pcl_memory_ = device->create_vertex_array(vertex_format, boost::assign::list_of(buffer_)(buffer_provenance_));
        
        tri_memory_ = device->create_vertex_array(scm::gl::vertex_format
            (0, 0, scm::gl::TYPE_VEC3F, size_of_surfel_)
            (0, 1, scm::gl::TYPE_VEC3F, size_of_surfel_)
            (0, 2, scm::gl::TYPE_VEC2F, size_of_surfel_),
            boost::assign::list_of(buffer_));
    }

    device->main_context()->apply();

#ifdef LAMURE_ENABLE_INFO
    std::cout << "lamure: gpu-cache size (MB): " << buffer_->descriptor()._size / 1024 / 1024 << std::endl;
#endif

}

gpu_access::
~gpu_access() {
    buffer_.reset();
    buffer_provenance_.reset();
    if (has_layout_) {
        pcl_memory_.reset();
        tri_memory_.reset();
    }
}

char* gpu_access::
map(scm::gl::render_device_ptr const& device) {
    if (!is_mapped_) {
        assert(device);
        is_mapped_ = true;
        return (char*)device->main_context()->map_buffer(buffer_, scm::gl::ACCESS_READ_WRITE);
    }
    return nullptr;
}

void gpu_access::
unmap(scm::gl::render_device_ptr const& device) {
    if (is_mapped_) {
        assert(device);
        device->main_context()->unmap_buffer(buffer_);
        is_mapped_ = false;
    }
}

char* gpu_access::
map_provenance(scm::gl::render_device_ptr const& device) {
    if (!is_mapped_provenance_) {
        assert(device);
        is_mapped_provenance_ = true;
        return (char*)device->main_context()->map_buffer(buffer_provenance_, scm::gl::ACCESS_READ_WRITE);
    }
    return nullptr;
}

void gpu_access::
unmap_provenance(scm::gl::render_device_ptr const& device) {
    if (is_mapped_provenance_) {
        assert(device);
        device->main_context()->unmap_buffer(buffer_provenance_);
        is_mapped_provenance_ = false;
    }
}

scm::gl::vertex_array_ptr gpu_access::
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
  return scm::gl::vertex_array_ptr(nullptr);

};

const size_t gpu_access::
query_video_memory_in_mb(scm::gl::render_device_ptr const& device) {
  int size_in_kb;
  device->opengl_api().glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &size_in_kb);
  //glGetIntegerv(0x9048/*GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX*/, &size_in_kb);
  return size_t(size_in_kb) / 1024;
}

}
}

