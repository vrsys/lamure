// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef LAMURE_UTIL_ATOMIC_COUNTER_H_
#define LAMURE_UTIL_ATOMIC_COUNTER_H_

#include <lamure/platform_core.h>
#include <atomic>

namespace lamure {
namespace util {

template <typename integral_or_pointer_t>
struct atomic_counter_t {
  std::atomic<integral_or_pointer_t> head_;
  
  void initialize(integral_or_pointer_t const& init_value) {
    head_.store(init_value);
  }

  // returns the value contained before increment
  integral_or_pointer_t increment_head() {
    return head_++;
  }
};

}
} // namespace lamure

#endif // LAMURE_ATOMIC_COUNTER_H_
