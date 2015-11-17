// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/io/format_bin.h>

#include <stdexcept>

namespace lamure {
namespace pre {

void FormatBin::
Read(const std::string& filename, SurfelCallbackFuntion callback)
{
    throw std::runtime_error("Not implemented yet!");
}

void FormatBin::
Write(const std::string& filename, BufferCallbackFuntion callback)
{

    File file;
    SurfelVector buffer;
    size_t count = 0;

    file.Open(filename, true);
    while (true) {
        bool ret = callback(buffer);
        if (!ret)
            break;

        file.Append(&buffer);
        count += buffer.size();
    }
    file.Close();
    LOGGER_TRACE("Output surfels: " << count);
}

}
}

