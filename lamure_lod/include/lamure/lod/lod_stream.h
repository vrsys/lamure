// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef LAMURE_LOD_LOD_STREAM_H_
#define LAMURE_LOD_LOD_STREAM_H_

#include <lamure/platform_lod.h>
#include <lamure/types.h>
#include <lamure/lod/config.h>
#include <lamure/assert.h>

#include <fstream>
#include <vector>
#include <string>
#include <cstdio>

namespace lamure {
namespace lod {

class LOD_DLL lod_stream 
{
public:
                        lod_stream();
                        lod_stream(const lod_stream&) = delete;
                        lod_stream& operator=(const lod_stream&) = delete;
    virtual             ~lod_stream();


    void                open(const std::string& file_name);
    void                open_for_writing(const std::string& file_name);
    void                close();
    const bool          is_file_open() const { return is_file_open_; };
    const std::string&  file_name() const { return file_name_; };

    void                read(char* const data,
                            const size_t start_in_file,
                            const size_t length_in_bytes) const;
                            
    void                write(char* const data,
                            const size_t start_in_file,
                            const size_t length_in_bytes);

private:
    mutable std::fstream stream_;

    std::string         file_name_;
    bool                is_file_open_;
};

} } // namespace lamure

#endif // LAMURE_LOD_LOD_STREAM_H_

