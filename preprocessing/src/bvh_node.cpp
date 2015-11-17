// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/bvh_node.h>

namespace lamure {
namespace pre
{

BvhNode::
BvhNode(const NodeIdType id,
        const uint32_t depth,
        const BoundingBox& bounding_box,
        const SurfelMemArray& array)
: node_id_(id),
  depth_(depth),
  bounding_box_(bounding_box),
  reduction_error_(0.0),
  avg_surfel_radius_(0.0),
  centroid_(vec3r(0.0))
{
    Reset(array);
}

BvhNode::
BvhNode(const NodeIdType id,
        const uint32_t depth,
        const BoundingBox& bounding_box,
        const SurfelDiskArray& array)
: node_id_(id),
  depth_(depth),
  bounding_box_(bounding_box),
  reduction_error_(0.0),
  avg_surfel_radius_(0.0),
  centroid_(vec3r(0.0))
{
    Reset(array);
}

BvhNode::
~BvhNode()
{
    Reset();
}

void BvhNode::
Reset()
{
    mem_array_.Reset();
    disk_array_.Reset();
}

void BvhNode::
Reset(const SurfelMemArray& array)
{
    Reset();
    mem_array_ = array;
}

void BvhNode::
Reset(const SurfelDiskArray& array)
{
    Reset();
    disk_array_ = array;
}

void BvhNode::
LoadFromDisk()
{
    assert(IsOOC());
    mem_array_.Reset(disk_array_.ReadAll(), 0, disk_array_.length());
}

void BvhNode::
FlushToDisk(const SharedFile& file,
            const size_t offset_in_file,
            const bool dealloc_mem_array)
{
    assert(IsIC());

    disk_array_.Reset(file, offset_in_file, mem_array_.length());
    disk_array_.WriteAll(mem_array_.mem_data(), mem_array_.offset());

    if (dealloc_mem_array)
        mem_array_.Reset();
}

void BvhNode::
FlushToDisk(const bool dealloc_mem_array)
{
    assert(IsIC());
    assert(IsOOC());

    disk_array_.WriteAll(mem_array_.mem_data(), mem_array_.offset());

    if (dealloc_mem_array)
        mem_array_.Reset();
}


} } // namespace lamure

