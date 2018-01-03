#ifndef LAMURE_STREAMABLE_H
#define LAMURE_STREAMABLE_H

#include <lamure/prov/data/interfaces/Readable.h>
#include <lamure/prov/common.h>
#include <lamure/prov/data/entities/Point.h>

namespace lamure {
namespace prov
{
template <class Entity>
class Streamable : public Readable
{
  public:
    Streamable(ifstream &is) : is(is) { read_header(is); }
    virtual ~Streamable() {}
    virtual const Entity &access_at_implicit(uint32_t index) = 0;
    virtual const vec<Entity> access_at_implicit_range(uint32_t index_start, uint32_t index_end) = 0;

  protected:
    ifstream &is;
};
}
}
#endif // LAMURE_STREAMABLE_H
