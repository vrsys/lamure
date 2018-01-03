#ifndef LAMURE_DENSE_DATA_H
#define LAMURE_DENSE_DATA_H

#include <lamure/prov/common.h>
#include <lamure/prov/dense_point.h>
#include <lamure/prov/dense_meta_data.h>
#include <lamure/prov/cacheable.h>
#include <lamure/prov/streamable.h>

namespace lamure {
namespace prov
{
class DenseCache : public Cacheable<DensePoint, DenseMetaData>
{
  public:
    DenseCache(ifstream &is_prov, ifstream &is_meta) : Cacheable(is_prov, is_meta){};
    ~DenseCache(){};
};
}
}

#endif // LAMURE_DENSE_DATA_H