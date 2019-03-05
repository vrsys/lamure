// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef LAMURE_CIELABINDEX_H
#define LAMURE_CIELABINDEX_H

#include <lamure/vt/pre/Index.h>

namespace vt
{
namespace pre
{
class CielabIndex : public Index<uint32_t>
{
  private:
    uint32_t _convert(float val);
    float _convert(uint32_t val);

  public:
    CielabIndex(size_t size);
    ~CielabIndex();

    float getCielabValue(uint64_t id);
    void set(uint64_t id, float cielabValue);
};
} // namespace pre
} // namespace vt

#endif // LAMURE_CIELABINDEX_H
