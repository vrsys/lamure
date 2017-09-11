// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/logger.h>

namespace lamure
{
namespace pre
{

std::mutex logger::mutex_;

logger logger::single_;

logger::logger()
{

}

logger::
~logger()
{
}

logger &logger::
get_instance()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return single_;
}

}
} //namespace lamure


