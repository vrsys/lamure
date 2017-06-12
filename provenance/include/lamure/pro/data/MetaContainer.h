#ifndef LAMURE_META_CONTAINER_H
#define LAMURE_META_CONTAINER_H

#include "lamure/pro/common.h"

namespace prov
{
class MetaContainer
{
  public:
    const vec<uint8_t> &get_metadata() const { return _metadata; }
    MetaContainer(const vec<uint8_t> &_metadata) : _metadata(_metadata) {}
    MetaContainer() { _metadata = vec<uint8_t>(); }
    ~MetaContainer() {}
    ifstream &read_metadata(ifstream &is)
    {
        uint32_t meta_data_length;
        is.read(reinterpret_cast<char *>(&meta_data_length), 4);
        meta_data_length = swap(meta_data_length, true);

        if(DEBUG)
            printf("\nMeta data length: %i ", meta_data_length);

        vec<uint8_t> byte_buffer(meta_data_length, 0);
        is.read(reinterpret_cast<char *>(&byte_buffer[0]), meta_data_length);
        _metadata = byte_buffer;
        return is;
    }

  protected:
    vec<uint8_t> _metadata;
};
};

#endif // LAMURE_META_CONTAINER_H