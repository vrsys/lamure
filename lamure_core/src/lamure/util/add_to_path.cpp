
#include <lamure/util/add_to_path.h>

namespace lamure {
namespace util {

CORE_DLL boost::filesystem::path add_to_path(
  const boost::filesystem::path& path,
  const std::string& addition)
{
  return boost::filesystem::path(path) += addition;
}


} // namespace util
} // namespace lamure

