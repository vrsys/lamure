// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/io/file.h>

#include <stdexcept>
#include <cstdio>
#include <cstring>

#include <lamure/pre/logger.h>

namespace lamure
{
namespace pre
{

file::
~file()
{
    try {
        close();
    }
    catch (...) {}
}

void file::
open(const std::string &file_name, const bool truncate)
{
    if (is_open()) {
        LOGGER_ERROR("Attempt to open file when the instance of "
                         "pre::file is already in open state. "
                         "file: \"" << file_name << "\" "
            "opened file: \"" << file_name_ << "\"");
        exit(1);
    }

    file_name_ = file_name;
    std::ios::openmode mode = std::ios::in |
        std::ios::out |
        std::ios::binary;
    if (truncate)
        mode |= std::ios::trunc;


    stream_.open(file_name_, mode);

    if (!is_open()) {
        LOGGER_ERROR("Failed to open file: \"" << file_name_ <<
                                               "\". " << strerror(errno));
    }
    stream_.exceptions(std::ifstream::failbit | std::ifstream::badbit);
}

void file::
close(const bool remove)
{
    if (is_open()) {
        stream_.flush();
        stream_.close();
        if (stream_.fail()) {
            LOGGER_ERROR("Failed to close file: \"" << file_name_ <<
                                                    "\". " << strerror(errno));
        }
        stream_.exceptions(std::ifstream::failbit);

        if (remove)
            if (std::remove(file_name_.c_str())) {
                LOGGER_WARN("Unable to delete file: \"" << file_name_ <<
                                                        "\". " << strerror(errno));
            }
        file_name_ = "";
    }
}

const bool file::
is_open() const
{
    return stream_.is_open();
}

const size_t file::
get_size() const
{
    std::lock_guard<std::mutex> lock(read_write_mutex_);

    assert(is_open());
    stream_.seekg(0, stream_.end);
    size_t len = stream_.tellg();
    stream_.seekg(0, stream_.beg);

    if (stream_.fail() || stream_.bad()) {
        LOGGER_ERROR("get_size failed. file: \"" << file_name_ <<
                                                 "\". " << strerror(errno));
    }
    return len / sizeof(surfel);
}

void file::
append(const surfel_vector *data,
       const size_t offset_in_mem,
       const size_t length)
{
    std::lock_guard<std::mutex> lock(read_write_mutex_);

    assert(is_open());
    assert(length > 0);
    assert(offset_in_mem + length <= data->size());

    stream_.seekp(0, stream_.end);
    stream_.write(reinterpret_cast<char *>(
                      const_cast<surfel *>(&(*data)[offset_in_mem])),
                  length * sizeof(surfel));

    if (stream_.fail() || stream_.bad()) {
        LOGGER_ERROR("append failed. file: \"" << file_name_ <<
                                               "\". (mem offset: " << offset_in_mem <<
                                               ", len: " << length << "). " << strerror(errno));
    }
    stream_.exceptions(std::ifstream::failbit | std::ifstream::badbit);
}

void file::
append(const surfel_vector *data)
{
    append(data, 0, data->size());
}

void file::
write(const surfel_vector *data,
      const size_t offset_in_mem,
      const size_t offset_in_file,
      const size_t length)
{
    assert(length > 0);
    assert(offset_in_mem + length <= data->size());

    write_data(reinterpret_cast<char *>(
                   const_cast<surfel *>(&(*data)[offset_in_mem])),
               offset_in_file, length);
}

void file::
write(const surfel &srfl, const size_t pos_in_file)
{
    write_data(reinterpret_cast<char *>(const_cast<surfel *>(&srfl)),
               pos_in_file, 1);
}

void file::
read(surfel_vector *data,
     const size_t offset_in_mem,
     const size_t offset_in_file,
     const size_t length) const
{
    assert(length > 0);
    assert(offset_in_mem + length <= data->size());

    read_data(reinterpret_cast<char *>(&(*data)[offset_in_mem]),
              offset_in_file, length);
}

const surfel file::
read(const size_t pos_in_file) const
{
    surfel s;
    read_data(reinterpret_cast<char *>(&s), pos_in_file, 1);
    return s;
}

void file::
write_data(char *data, const size_t offset_in_file, const size_t length)
{
    assert(is_open());

    std::lock_guard<std::mutex> lock(read_write_mutex_);
    stream_.seekp(offset_in_file * sizeof(surfel));
    stream_.write(data, length * sizeof(surfel));

    if (stream_.fail() || stream_.bad()) {
        LOGGER_ERROR("write failed. file: \"" << file_name_ <<
                                              "\". (offset: " << offset_in_file <<
                                              ", len: " << length << "). " << strerror(errno));
    }
    stream_.exceptions(std::ifstream::failbit | std::ifstream::badbit);
}

void file::
read_data(char *data, const size_t offset_in_file, const size_t length) const
{
    assert(is_open());

    std::lock_guard<std::mutex> lock(read_write_mutex_);
    stream_.seekg(offset_in_file * sizeof(surfel));
    stream_.read(data, length * sizeof(surfel));

    if (stream_.fail() || stream_.bad()) {
        LOGGER_ERROR("read failed. file: \"" << file_name_ <<
                                             "\". (offset: " << offset_in_file <<
                                             ", len: " << length << "). " << strerror(errno));
    }
    stream_.exceptions(std::ifstream::failbit | std::ifstream::badbit);
}

}
} // namespace lamure

