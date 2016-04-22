
#ifndef LAMURE_UTIL_TO_STRING_H_
#define LAMURE_UTIL_TO_STRING_H_

#include <sstream>
#include <string>

namespace lamure {
namespace util {

template<typename T>
std::string
to_string(const T& value)
{
  std::ostringstream oss;
  oss << value;
  return oss.str();
}

} // namespace util
} // namespace lamure

#endif
