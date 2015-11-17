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

class PREPROCESSING_DLL File
{
public:
                        File() {}
                        File(const File&) = delete;
                        File& operator=(const File&) = delete;
    virtual             ~File();


    void                Open(const std::string& file_name,
                             const bool truncate = false);
    void                Close(const bool remove = false);
    const bool          IsOpen() const;
    const size_t        GetSize() const;
    const std::string&  file_name() const { return file_name_; }

    void                Append(const SurfelVector* data,
                               const size_t offset_in_mem,
                               const size_t length);
    void                Append(const SurfelVector* data);

    void                Write(const SurfelVector* data,
                              const size_t offset_in_mem,
                              const size_t offset_in_file,
                              const size_t length);
    void                Write(const Surfel& surfel, const size_t pos_in_file);

    void                Read(SurfelVector* data,
                              const size_t offset_in_mem,
                              const size_t offset_in_file,
                              const size_t length) const;
    const Surfel        Read(const size_t pos_in_file) const;

private:

    mutable std::mutex  read_write_mutex_;
    mutable std::fstream stream_;
    std::string         file_name_;

    void WriteData(char *data, const size_t offset_in_file, const size_t length);
    void ReadData(char *data, const size_t offset_in_file, const size_t length) const;

};

typedef std::shared_ptr<File> SharedFile;

} // namespace pre
} // namespace lamure

#endif // PRE_FILE_H_
