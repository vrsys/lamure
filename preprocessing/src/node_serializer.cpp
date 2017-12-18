// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/node_serializer.h>

#include <lamure/pre/serialized_surfel.h>
#include <cstring>

namespace lamure
{
namespace pre
{

node_serializer::
node_serializer(const size_t surfels_per_node,
                const size_t buffer_size)
    : surfels_per_node_(surfels_per_node)
{
    max_nodes_in_buffer_ = buffer_size / sizeof(surfel) / surfels_per_node;
}

node_serializer::
~node_serializer()
{
    try {
        close();
    }
    catch (...) {}
}

void node_serializer::
open(const std::string &file_name, const bool read_write_mode)
{
    file_name_ = file_name;
    buffer_.clear();

    if (read_write_mode)
        stream_.open(file_name, std::ios::in | std::ios::out | std::ios::binary);
    else
        stream_.open(file_name, std::ios::out | std::ios::binary | std::ios::trunc);

    if (!stream_.is_open()) {
        LOGGER_ERROR("Failed to create/open file: \"" << file_name_ <<
                                                      "\". " << strerror(errno));
    }

    stream_.exceptions(std::ifstream::failbit | std::ifstream::badbit);
}

void node_serializer::
close()
{
    if (is_open()) {
        flush_buffer();
        buffer_.clear();
        stream_.close();
        if (stream_.fail()) {
            LOGGER_ERROR("Failed to close file: \"" << file_name_ <<
                                                    "\". " << strerror(errno));
        }
        stream_.exceptions(std::ifstream::failbit);
        file_name_ = "";
    }
}

const bool node_serializer::
is_open() const
{
    return stream_.is_open();
}

void node_serializer::
read_node_immediate(surfel_vector &surfels,
                    const size_t offset)
{
    surfels.clear();
    const size_t buffer_size = serialized_surfel::get_size() * surfels_per_node_;
    char *buffer = new char[buffer_size];

    stream_.seekg(buffer_size * offset);
    stream_.read(buffer, buffer_size);
    if (stream_.fail() || stream_.bad()) {
        LOGGER_ERROR("read failed. file: \"" << file_name_ <<
                                             "\". " << strerror(errno));
    }
    stream_.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    for (size_t i = 0; i < surfels_per_node_; ++i) {
        size_t pos = i * serialized_surfel::get_size();
        surfels.push_back(serialized_surfel().Deserialize(buffer + pos).get_surfel());
    }

    delete[] buffer;
}

void node_serializer::
write_node_immediate(const surfel_vector &surfels,
                     const size_t offset)
{
    const size_t buffer_size = serialized_surfel::get_size() * surfels_per_node_;
    char *buffer = new char[buffer_size];

    for (size_t i = 0; i < surfels_per_node_; ++i) {
        size_t pos = i * serialized_surfel::get_size();
        serialized_surfel(surfels[i]).serialize(buffer + pos);
    }

    stream_.seekp(buffer_size * offset);
    stream_.write(buffer, buffer_size);
    if (stream_.fail() || stream_.bad()) {
        LOGGER_ERROR("write failed. file: \"" << file_name_ <<
                                              "\". " << strerror(errno));
    }
    stream_.exceptions(std::ifstream::failbit | std::ifstream::badbit);

}

void node_serializer::
serialize_nodes(const std::vector<bvh_node> &nodes)
{
    for (const auto &n: nodes)
        write_node_streamed(n);
}

void node_serializer::
write_node_streamed(const bvh_node &node)
{
    assert(max_nodes_in_buffer_ != 0);
    assert(is_open());
    assert(node.is_out_of_core());

    const size_t read_length = (node.disk_array().length() > surfels_per_node_) ?
                               surfels_per_node_ :
                               node.disk_array().length();

    surfel_vector *surfel_buffer = new surfel_vector(read_length);
    node.disk_array().get_file()->read(surfel_buffer, 0,
                                   node.disk_array().offset(),
                                   read_length);
    buffer_.push_back(surfel_buffer);

    if (buffer_.size() >= max_nodes_in_buffer_)
        flush_buffer();
}

void node_serializer::
flush_buffer()
{
    if (buffer_.size()) {
        const size_t output_buffer_size = serialized_surfel::get_size() * surfels_per_node_ * buffer_.size();
        char *output_buffer = new char[output_buffer_size];

        LOGGER_ERROR("Flush buffer to disk. buffer size: " <<
                                                           buffer_.size() << " nodes (" <<
                                                           output_buffer_size / 1024 / 1024 << " MiB)");

#pragma omp parallel for
        for (size_t k = 0; k < buffer_.size(); ++k) {
            for (size_t i = 0; i < surfels_per_node_; ++i) {
                char *buf = output_buffer + k * serialized_surfel::get_size() * surfels_per_node_ +
                    i * serialized_surfel::get_size();
                if (i < buffer_[k]->size())
                    serialized_surfel(buffer_[k]->at(i)).serialize(buf);
                else
                    serialized_surfel().serialize(buf);
            }
            delete buffer_[k];
        }

        stream_.seekp(0, stream_.end);
        stream_.write(output_buffer, output_buffer_size);
        if (stream_.fail() || stream_.bad()) {
            LOGGER_ERROR("write failed. file: \"" << file_name_ <<
                                                  "\". " << strerror(errno));
        }
        buffer_.clear();
        delete[] output_buffer;
        stream_.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    }
}

}
} // namespace lamure

