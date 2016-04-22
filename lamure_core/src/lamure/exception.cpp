
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
