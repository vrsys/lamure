// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/bvh_node.h>

namespace lamure
{
namespace pre
{

bvh_node::
bvh_node(const node_id_type id,
        const uint32_t depth,
        const bounding_box& bounding_box,
        const surfel_mem_array& array)
: node_id_(id),
  depth_(depth),
  bounding_box_(bounding_box),
  reduction_error_(0.0),
  avg_surfel_radius_(0.0),
  centroid_(vec3r(0.0)),
  max_surfel_radius_deviation_(0.0)
{
    reset(array);
}

bvh_node::
bvh_node(const node_id_type id,
        const uint32_t depth,
        const bounding_box& bounding_box,
        const surfel_disk_array& array)
: node_id_(id),
  depth_(depth),
  bounding_box_(bounding_box),
  reduction_error_(0.0),
  avg_surfel_radius_(0.0),
  centroid_(vec3r(0.0)),
  max_surfel_radius_deviation_(0.0)

{
    reset(array);
}

bvh_node::
~bvh_node()
{
    reset();
}

void bvh_node::
calculate_statistics()
{
    node_stats_.calculate_statistics(mem_array_);
    node_stats_.set_dirty(false);
}

void bvh_node::
reset()
{
    mem_array_.reset();
    disk_array_.reset();
}

void bvh_node::
reset(const surfel_mem_array &array)
{
    reset();
    mem_array_ = array;
}

void bvh_node::
reset(const surfel_disk_array &array)
{
    reset();
    disk_array_ = array;
}

void bvh_node::
load_from_disk()
{
    assert(is_out_of_core());
    if (disk_array_.has_provenance()) {
      mem_array_.reset(disk_array_.read_all(), disk_array_.read_all_prov(), 0, disk_array_.length());
    }
    else {
      mem_array_.reset(disk_array_.read_all(), 0, disk_array_.length());
    }
}

void bvh_node::
flush_to_disk(const shared_surfel_file &file,
              const size_t offset_in_file,
              const bool dealloc_mem_array)
{
    assert(is_in_core());

    disk_array_.reset(file, offset_in_file, mem_array_.length());
    disk_array_.write_all(mem_array_.surfel_mem_data(), mem_array_.offset());

    if (dealloc_mem_array)
        mem_array_.reset();
}

void bvh_node::
flush_to_disk(const shared_surfel_file &surfel_file,
              const shared_prov_file &prov_file,
              const size_t offset_in_file,
              const bool dealloc_mem_array)
{
    assert(is_in_core());

    disk_array_.reset(surfel_file, prov_file, offset_in_file, mem_array_.length());
    disk_array_.write_all(mem_array_.surfel_mem_data(), mem_array_.prov_mem_data(), mem_array_.offset());

    if (dealloc_mem_array)
        mem_array_.reset();
}

void bvh_node::
flush_to_disk(const bool dealloc_mem_array)
{
    assert(is_in_core());
    assert(is_out_of_core());

    if (mem_array_.has_provenance()) {
      disk_array_.write_all(mem_array_.surfel_mem_data(), mem_array_.prov_mem_data(), mem_array_.offset());
    }
    else {
      disk_array_.write_all(mem_array_.surfel_mem_data(), mem_array_.offset());
    }

    if (dealloc_mem_array)
        mem_array_.reset();
}

}
} // namespace lamure

