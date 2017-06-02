#ifndef LAMURE_SPARSE_DATA_H
#define LAMURE_SPARSE_DATA_H

#include "Data.h"
#include "SparsePoint.h"
#include "lamure/pro/common.h"

namespace prov
{
class SparseData : public Data<SparsePoint>
{
  public:
    SparseData(){};
    ~SparseData(){};
    const vec<Camera> &get_cameras() const { return _cameras; }
    friend ifstream &operator>>(ifstream &is, SparseData &data)
    {
        // TODO: read cameras
        return is;
    }

  protected:
    vec<Camera> _cameras;
};
}

#endif // LAMURE_SPARSE_DATA_H