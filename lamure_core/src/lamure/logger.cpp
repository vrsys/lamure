// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/logger.h>

namespace lamure {

logger_t logger_t::single_;

logger_t::logger_t()
: level_(LOGGER_LEVEL_INFO)
{

}

logger_t::~logger_t()
{

}

logger_t& logger_t::get()
{
  return single_;
}

} // namespace lamure
