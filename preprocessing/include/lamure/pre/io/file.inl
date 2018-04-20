// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <stdexcept>
#include <cstdio>
#include <cstring>

#include <lamure/pre/logger.h>

namespace lamure {
namespace pre {

template<typename T>
file<T>::~file()
{
    try {
        close();
    }
    catch (...) {}
}

template<typename T>
void file<T>::
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

template<typename T>
void file<T>::
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

template<typename T>
const bool file<T>::
is_open() const
{
    return stream_.is_open();
}

template<typename T>
const size_t file<T>::
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
    return len / sizeof(T);
}

template<typename T>
void file<T>::
append(const std::vector<T> *data,
       const size_t offset_in_mem,
       const size_t length)
{
    std::lock_guard<std::mutex> lock(read_write_mutex_);

    assert(is_open());
    assert(length > 0);
    assert(offset_in_mem + length <= data->size());

    stream_.seekp(0, stream_.end);
    stream_.write(reinterpret_cast<char *>(
                      const_cast<T *>(&(*data)[offset_in_mem])),
                  length * sizeof(T));

    if (stream_.fail() || stream_.bad()) {
        LOGGER_ERROR("append failed. file: \"" << file_name_ <<
                                               "\". (mem offset: " << offset_in_mem <<
                                               ", len: " << length << "). " << strerror(errno));
    }
    stream_.exceptions(std::ifstream::failbit | std::ifstream::badbit);
}

template<typename T>
void file<T>::
append(const std::vector<T> *data)
{
    append(data, 0, data->size());
}

template<typename T>
void file<T>::
write(const std::vector<T> *data,
      const size_t offset_in_mem,
      const size_t offset_in_file,
      const size_t length)
{
    assert(length > 0);
    assert(offset_in_mem + length <= data->size());

    write_data(reinterpret_cast<char *>(
                   const_cast<T *>(&(*data)[offset_in_mem])),
               offset_in_file, length);
}

template<typename T>
void file<T>::
write(const T &srfl, const size_t pos_in_file)
{
    write_data(reinterpret_cast<char *>(const_cast<T *>(&srfl)),
               pos_in_file, 1);
}

template<typename T>
void file<T>::
read(std::vector<T> *data,
     const size_t offset_in_mem,
     const size_t offset_in_file,
     const size_t length) const
{
    assert(length > 0);
    assert(offset_in_mem + length <= data->size());

    read_data(reinterpret_cast<char *>(&(*data)[offset_in_mem]),
              offset_in_file, length);
}

template<typename T>
const T file<T>::
read(const size_t pos_in_file) const
{
    T s;
    read_data(reinterpret_cast<char *>(&s), pos_in_file, 1);
    return s;
}

template<typename T>
void file<T>::
write_data(char *data, const size_t offset_in_file, const size_t length)
{
    assert(is_open());

    std::lock_guard<std::mutex> lock(read_write_mutex_);
    stream_.seekp(offset_in_file * sizeof(T));
    stream_.write(data, length * sizeof(T));

    if (stream_.fail() || stream_.bad()) {
        LOGGER_ERROR("write failed. file: \"" << file_name_ <<
                                              "\". (offset: " << offset_in_file <<
                                              ", len: " << length << "). " << strerror(errno));
    }
    stream_.exceptions(std::ifstream::failbit | std::ifstream::badbit);
}

template<typename T>
void file<T>::
read_data(char *data, const size_t offset_in_file, const size_t length) const
{
    assert(is_open());

    std::lock_guard<std::mutex> lock(read_write_mutex_);
    stream_.seekg(offset_in_file * sizeof(T));
    stream_.read(data, length * sizeof(T));

    if (stream_.fail() || stream_.bad()) {
        LOGGER_ERROR("read failed. file: \"" << file_name_ <<
                                             "\". (offset: " << offset_in_file <<
                                             ", len: " << length << "). " << strerror(errno));
    }
    stream_.exceptions(std::ifstream::failbit | std::ifstream::badbit);
}

}
} // namespace lamure

