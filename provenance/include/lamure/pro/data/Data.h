#ifndef LAMURE_DATA_H
#define LAMURE_DATA_H

#include "Point.h"
#include "lamure/pro/common.h"
#include <ostream>

namespace prov
{
template <class TPoint>
class Data
{
  public:
    Data() { static_assert(std::is_base_of<Point, TPoint>::value, "The used point type is not a derivative of Point"); }
    ~Data() {};
    const vec<TPoint> &get_points() const { return _points; }
    friend ifstream &operator>>(ifstream &is, Data<TPoint> &data)
    {
        // TODO: read header and vector of points
        return is;
    }

  protected:
    vec<TPoint> _points;
};
}

#endif // LAMURE_DATA_H