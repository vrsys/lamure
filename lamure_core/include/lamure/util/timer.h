
#ifndef LAMURE_UTIL_TIMER_H_
#define LAMURE_UTIL_TIMER_H_

#include <lamure/platform_core.h>
#include <lamure/types.h>

#include <boost/timer/timer.hpp>

namespace lamure {
namespace util {

typedef boost::timer::auto_cpu_timer auto_timer_t;
typedef boost::timer::cpu_timer cpu_timer_t;

}
} // namespace lamure

#endif // LAMURE_CORE_TIMER_H_
