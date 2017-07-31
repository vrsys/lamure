#ifndef LAMURE_META_CONTAINER_H
#define LAMURE_META_CONTAINER_H

#include <lamure/pro/common.h>

namespace prov
{
class MetaData
{
  public:
    const vec<uint8_t> &get_metadata() const { return _metadata; }
    MetaData() {}

    virtual void read_metadata(ifstream &is, uint64_t meta_data_length)
    {
        vec<uint8_t> byte_buffer(meta_data_length, 0);
        is.read(reinterpret_cast<char *>(&byte_buffer[0]), meta_data_length);
    }

  protected:
    vec<uint8_t> _metadata;
};
};

#endif // LAMURE_META_CONTAINER_H