#include <lamure/utils.h>

namespace lamure {

boost::filesystem::path add_to_path(const boost::filesystem::path& path,
                                  const std::string& addition)
{
    return boost::filesystem::path(path) += addition;
}

} // namespace lamure

