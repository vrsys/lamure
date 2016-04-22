// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/exception.h>

namespace lamure {

exception_t::exception_t(
  const std::string& _error)
: std::exception(), error_(_error)
{

}

exception_t::~exception_t() throw()
{

}

const std::string&
exception_t::get_error() const
{
  return error_;
}

} // namespace lamure
