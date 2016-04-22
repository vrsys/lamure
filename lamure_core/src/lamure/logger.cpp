
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
