
#ifndef LAMURE_CORE_TIMER_H_
#define LAMURE_CORE_TIMER_H_

#include <lamure/platform.h>
#include <lamure/types.h>

#include <boost/timer/timer.hpp>

namespace lamure {

typedef boost::timer::auto_cpu_timer auto_timer;
typedef boost::timer::cpu_timer cpu_timer;

} // namespace lamure

#endif // LAMURE_CORE_TIMER_H_
