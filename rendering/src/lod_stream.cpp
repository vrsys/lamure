// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/ren/lod_stream.h>

#include <stdexcept>
#include <cstdio>

namespace lamure {
namespace ren {

LodStream::
LodStream()
: is_file_open_(false) {

}

LodStream::
~LodStream() {
    try {
        close();
    }
    catch (...) {}
}

void LodStream::
open(const std::string& file_name) {
    file_name_ = file_name;
    std::ios::openmode mode = std::ios::in |
                              std::ios::binary;

    stream_.open(file_name_, mode);
    try {
        stream_.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    }
    catch (const std::ios_base::failure& e) {
        std::cout << "caught ios::failure \n " << 
                     "expl: " << e.what() << std::endl;
                     //"error code: " << e.code() << "\n" << std::endl;
    }

    if (!stream_.is_open()) {
        throw std::runtime_error(
            "LodStream::Unable to open file: " + file_name_);
    }

    is_file_open_ = true;
}


void LodStream::
openForWriting(const std::string& file_name) {
    file_name_ = file_name;
    std::ios::openmode mode = std::ios::out |
                              std::ios::binary;

    stream_.open(file_name_, mode);
    stream_.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    if (!stream_.is_open()) {
        throw std::runtime_error(
            "LodStream::Unable to open file for writing: " + file_name_);
    }

    is_file_open_ = true;
}

void LodStream::
close() {
    if (is_file_open_) {
        stream_.close();
        stream_.exceptions(std::ifstream::failbit);

        file_name_ = "";
        is_file_open_ = false;
    }
}

void LodStream::
read(char* const data,
     const size_t offset_in_bytes,
     const size_t length_in_bytes) const {
    assert(length_in_bytes > 0);
    assert(is_file_open_);
    assert(data != nullptr);

    stream_.seekg(offset_in_bytes);
    stream_.read(data, length_in_bytes);

}

                            
void LodStream::
write(char* const data,
      const size_t start_in_file,
      const size_t length_in_bytes) {
    assert(length_in_bytes > 0);
    assert(is_file_open_);
    assert(data != nullptr);
    
    stream_.seekp(start_in_file);
    stream_.write(data, length_in_bytes);
    
}

} } // namespace lamure
