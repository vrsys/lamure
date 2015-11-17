#include <lamure/utils.h>

namespace lamure {

boost::filesystem::path AddToPath(const boost::filesystem::path& path,
                                  const std::string& addition)
{
    return boost::filesystem::path(path) += addition;
}

} // namespace lamure

