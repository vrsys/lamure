// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr


#ifndef NORMAL_RADII_STRATEGY_H_
#define NORMAL_RADII_STRATEGY_H_

#include <vector>

#include <lamure/pre/surfel.h>

namespace lamure{
namespace pre {

class normal_radii_strategy {
public:

  virtual ~normal_radii_strategy() = default;

  virtual vec3f compute_normal(size_t node, size_t surfel_of_interest, std::vector<std::pair<surfel, real>> const& neighbours) const = 0;
  virtual real compute_radius(size_t node, size_t surfel_of_interest, std::vector<std::pair<surfel, real>> const& neighbours) const = 0;
};

} // namespace pre
} // namespace lamure

#endif //NORMAL_RADII_STRATEGY_H_