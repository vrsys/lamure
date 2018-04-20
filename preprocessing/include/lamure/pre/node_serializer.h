// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
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


namespace lamure
{
namespace pre
{

/**
* serializes nodes to a LOD file that can be used in rendering application.
*/
class PREPROCESSING_DLL node_serializer
{
public:
    explicit node_serializer(const size_t surfels_per_node,
                             const size_t buffer_size); // buffer_size - in bytes

    node_serializer(const node_serializer &) = delete;
    node_serializer &operator=(const node_serializer &) = delete;
    virtual             ~node_serializer();

    void open(const std::string &file_name, const bool read_write_mode = false);
    void close();
    const bool is_open() const;

    void serialize_nodes(const std::vector<bvh_node> &nodes);
    void serialize_prov(const std::vector<bvh_node> &nodes);

    void read_node_immediate(surfel_vector &surfels,
                             const size_t offset);
    void write_node_immediate(const surfel_vector &surfels,
                              const size_t offset);

private:

    void write_node_streamed(const bvh_node &node);
    void flush_surfel_buffer();

    mutable std::fstream stream_;
    std::string file_name_;
    size_t surfels_per_node_;

    std::deque<surfel_vector *> surfel_buffer_;
    size_t max_nodes_in_buffer_;
};

}
} // namespace lamure

#endif // PRE_NODE_SERIALIZER_H_

