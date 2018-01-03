#ifndef LAMURE_META_CONTAINER_H
#define LAMURE_META_CONTAINER_H

#include <lamure/prov/common.h>

namespace lamure {
namespace prov
{
class MetaData
{
  public:
    const vec<char> &get_metadata() const { return _metadata; }
    MetaData() {}
    ~MetaData() {}
    virtual void read_metadata(ifstream &is, uint32_t meta_data_length)
    {
        _metadata = vec<char>(meta_data_length, 0);
        is.read(&_metadata[0], meta_data_length);
    }

  protected:
    vec<char> _metadata;
};
}
}

#endif // LAMURE_META_CONTAINER_H