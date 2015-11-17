// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_NODE_SERIALIZER_H_
#define PRE_NODE_SERIALIZER_H_

#include <lamure/pre/platform.h>
#include <lamure/pre/surfel.h>
#include <lamure/pre/bvh_node.h>
#include <lamure/pre/logger.h>

#include <fstream>
#include <string>
#include <deque>


namespace lamure {
namespace pre
{

/**
* Serializes nodes to a LOD file that can be used in rendering application.
*/
class PREPROCESSING_DLL NodeSerializer
{
public:
    explicit            NodeSerializer(const size_t surfels_per_node,
                                       const size_t buffer_size); // buffer_size - in bytes

                        NodeSerializer(const NodeSerializer&) = delete;
                        NodeSerializer& operator=(const NodeSerializer&) = delete;
    virtual             ~NodeSerializer();

    void                Open(const std::string& file_name, const bool read_write_mode = false);
    void                Close();
    const bool          IsOpen() const;

    void                SerializeNodes(const std::vector<BvhNode>& nodes);

    void                ReadNodeImmediate(SurfelVector& surfels, 
                                          const size_t offset);
    void                WriteNodeImmediate(const SurfelVector& surfels, 
                                           const size_t offset);

private:

    void                WriteNodeStreamed(const BvhNode& node);
    void                FlushBuffer();

    mutable std::fstream stream_;
    std::string         file_name_;
    size_t              surfels_per_node_;

    std::deque<SurfelVector*> 
                        buffer_;
    size_t              max_nodes_in_buffer_;
};

} } // namespace lamure

#endif // PRE_NODE_SERIALIZER_H_

