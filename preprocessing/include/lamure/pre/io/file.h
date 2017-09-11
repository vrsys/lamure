// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_FILE_H_
#define PRE_FILE_H_

#include <lamure/pre/platform.h>
#include <lamure/pre/surfel.h>

#include <mutex>
#include <fstream>
#include <vector>
#include <string>
#include <memory>

namespace lamure
{
namespace pre
{

class PREPROCESSING_DLL file
{
public:
    file()
    {}
    file(const file &) = delete;
    file &operator=(const file &) = delete;
    virtual             ~file();

    void open(const std::string &file_name,
              const bool truncate = false);
    void close(const bool remove = false);
    const bool is_open() const;
    const size_t get_size() const;
    const std::string &file_name() const
    { return file_name_; }

    void append(const surfel_vector *data,
                const size_t offset_in_mem,
                const size_t length);
    void append(const surfel_vector *data);

    void write(const surfel_vector *data,
               const size_t offset_in_mem,
               const size_t offset_in_file,
               const size_t length);
    void write(const surfel &surfel, const size_t pos_in_file);

    void read(surfel_vector *data,
              const size_t offset_in_mem,
              const size_t offset_in_file,
              const size_t length) const;
    const surfel read(const size_t pos_in_file) const;

private:

    mutable std::mutex read_write_mutex_;
    mutable std::fstream stream_;
    std::string file_name_;

    void write_data(char *data, const size_t offset_in_file, const size_t length);
    void read_data(char *data, const size_t offset_in_file, const size_t length) const;

};

typedef std::shared_ptr<file> shared_file;

} // namespace pre
} // namespace lamure

#endif // PRE_FILE_H_
