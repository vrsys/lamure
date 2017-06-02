#ifndef LAMURE_META_CONTAINER_H
#define LAMURE_META_CONTAINER_H

#include "lamure/pro/common.h"

namespace prov
{
class MetaContainer
{
  public:
    MetaContainer(const vec<char> &_metadata);
    virtual ~MetaContainer();
    const vec<char> &get_metadata() const;

  protected:
    vec<char> _metadata;
};
};

#endif // LAMURE_META_CONTAINER_H