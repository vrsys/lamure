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

namespace lamure {
namespace pre
{

File::
~File()
{
    try {
        Close();
    }
    catch (...) {}
}

void File::
Open(const std::string& file_name, const bool truncate)
{
    if (IsOpen()) {
        LOGGER_ERROR("Attempt to open file when the instance of "
                                "pre::File is already in open state. "
                                "File: \"" << file_name << "\" "
                                "Opened file: \"" << file_name_ << "\"");
        exit(1);
    }

    file_name_ = file_name;
    std::ios::openmode mode = std::ios::in |
                              std::ios::out |
                              std::ios::binary;
    if (truncate)
        mode |= std::ios::trunc;

    
    stream_.open(file_name_, mode);

    if (!IsOpen()) {
        LOGGER_ERROR("Failed to open file: \"" << file_name_ << 
                                "\". " << strerror(errno));
    }
    stream_.exceptions(std::ifstream::failbit | std::ifstream::badbit);
}

void File::
Close(const bool remove)
{
    if (IsOpen()) {
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

const bool File::
IsOpen() const
{
    return stream_.is_open();
}

const size_t File::
GetSize() const
{
    std::lock_guard<std::mutex> lock(read_write_mutex_);

    assert(IsOpen());
    stream_.seekg(0, stream_.end);
    size_t len = stream_.tellg();
    stream_.seekg(0, stream_.beg);

    if (stream_.fail() || stream_.bad()) {
        LOGGER_ERROR("GetSize failed. File: \"" << file_name_ << 
                                "\". " << strerror(errno));
    }
    return len / sizeof(Surfel);
}

void File::
Append(const SurfelVector* data,
       const size_t offset_in_mem,
       const size_t length)
{
    std::lock_guard<std::mutex> lock(read_write_mutex_);

    assert(IsOpen());
    assert(length > 0);
    assert(offset_in_mem + length <= data->size());

    stream_.seekp(0, stream_.end);
    stream_.write(reinterpret_cast<char*>(
                  const_cast<Surfel*>(&(*data)[offset_in_mem])),
                  length * sizeof(Surfel));

    if (stream_.fail() || stream_.bad()) {
        LOGGER_ERROR("Append failed. File: \"" << file_name_ << 
                                "\". (mem offset: " << offset_in_mem << 
                                ", len: " << length << "). " << strerror(errno));
    }
    stream_.exceptions(std::ifstream::failbit | std::ifstream::badbit);
}

void File::
Append(const SurfelVector* data)
{
    Append(data, 0, data->size());
}

void File::
Write(const SurfelVector* data,
      const size_t offset_in_mem,
      const size_t offset_in_file,
      const size_t length)
{
    assert(length > 0);
    assert(offset_in_mem + length <= data->size());

    WriteData(reinterpret_cast<char*>(
              const_cast<Surfel*>(&(*data)[offset_in_mem])),
              offset_in_file, length);
}

void File::
Write(const Surfel& surfel, const size_t pos_in_file)
{
    WriteData(reinterpret_cast<char*>(const_cast<Surfel*>(&surfel)),
              pos_in_file, 1);
}

void File::
Read(SurfelVector* data,
     const size_t offset_in_mem,
     const size_t offset_in_file,
     const size_t length) const
{
    assert(length > 0);
    assert(offset_in_mem + length <= data->size());

    ReadData(reinterpret_cast<char*>(&(*data)[offset_in_mem]),
             offset_in_file, length);
}

const Surfel File::
Read(const size_t pos_in_file) const
{
    Surfel s;
    ReadData(reinterpret_cast<char*>(&s), pos_in_file, 1);
    return s;
}


void File::
WriteData(char *data, const size_t offset_in_file, const size_t length)
{
    assert(IsOpen());

    std::lock_guard<std::mutex> lock(read_write_mutex_);
    stream_.seekp(offset_in_file * sizeof(Surfel));
    stream_.write(data, length * sizeof(Surfel));

    if (stream_.fail() || stream_.bad()) {
        LOGGER_ERROR("Write failed. File: \"" << file_name_ << 
                                "\". (offset: " << offset_in_file << 
                                ", len: " << length << "). " << strerror(errno));
    }
    stream_.exceptions(std::ifstream::failbit | std::ifstream::badbit);
}

void File::
ReadData(char *data, const size_t offset_in_file, const size_t length) const
{
    assert(IsOpen());

    std::lock_guard<std::mutex> lock(read_write_mutex_);
    stream_.seekg(offset_in_file * sizeof(Surfel));
    stream_.read(data, length * sizeof(Surfel));

    if (stream_.fail() || stream_.bad()) {
        LOGGER_ERROR("Read failed. File: \"" << file_name_ << 
                                "\". (offset: " << offset_in_file << 
                                ", len: " << length << "). " << strerror(errno));
    }
    stream_.exceptions(std::ifstream::failbit | std::ifstream::badbit);
}

} } // namespace lamure

