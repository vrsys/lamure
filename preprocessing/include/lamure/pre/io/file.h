// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_FILE_H_
#define PRE_FILE_H_

#include <lamure/pre/platform.h>
#include <lamure/pre/surfel.h>
#include <lamure/pre/prov.h>

#include <mutex>
#include <fstream>
#include <vector>
#include <string>
#include <memory>

namespace lamure {
namespace pre {

template<typename T>
class PREPROCESSING_DLL file
{
public:
    file() {}
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

    void append(const std::vector<T> *data,
                const size_t offset_in_mem,
                const size_t length);
    void append(const std::vector<T> *data);

    void write(const std::vector<T> *data,
               const size_t offset_in_mem,
               const size_t offset_in_file,
               const size_t length);
    void write(const T &surfel, const size_t pos_in_file);

    void read(std::vector<T> *data,
              const size_t offset_in_mem,
              const size_t offset_in_file,
              const size_t length) const;
    const T read(const size_t pos_in_file) const;

private:

    mutable std::mutex read_write_mutex_;
    mutable std::fstream stream_;
    std::string file_name_;

    void write_data(char *data, const size_t offset_in_file, const size_t length);
    void read_data(char *data, const size_t offset_in_file, const size_t length) const;

};

} // namespace pre
} // namespace lamure

#include <lamure/pre/io/file.inl>

namespace lamure {
namespace pre {

typedef file<surfel> surfel_file;
typedef std::shared_ptr<surfel_file> shared_surfel_file;

typedef file<prov_data> prov_file;
typedef std::shared_ptr<prov_file> shared_prov_file;

}
}

#endif // PRE_FILE_H_
