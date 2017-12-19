// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_PROV_H_
#define PRE_PROV_H_

#include <lamure/pre/platform.h>
#include <lamure/types.h>

#include <memory>
#include <vector>

namespace lamure
{
namespace pre
{
class PREPROCESSING_DLL prov
{
  public:
    prov() {};

    const float value() const { return value_; }
    float &value() { return value_; }

  private:
    float value_;
    

};

using prov_vector = std::vector<prov>;
using shared_prov = std::shared_ptr<prov>;
using shared_prov_vector = std::shared_ptr<prov_vector>;

}
} // namespace lamure

#endif // PRE_PROV_H_
