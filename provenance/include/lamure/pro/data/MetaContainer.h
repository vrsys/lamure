#ifndef LAMURE_META_CONTAINER_H
#define LAMURE_META_CONTAINER_H

#include "lamure/pro/common.h"

namespace prov
{
class MetaContainer
{
  public:
    const vec<char> &get_metadata() const { return _metadata; }
    MetaContainer(const vec<char> &_metadata) : _metadata(_metadata) {}
    MetaContainer() { _metadata = vec<char>(); }
    ~MetaContainer() {}
    ifstream &read_metadata(ifstream &is)
    {
        uint32_t meta_data_length;
        is.read(reinterpret_cast<char *>(&meta_data_length), 4);
        meta_data_length = swap(meta_data_length, true);

        if(DEBUG)
            printf("\nMeta data length: %i ", meta_data_length);

        char metadata_buffer[meta_data_length];
        is.read(reinterpret_cast<char *>(&metadata_buffer), meta_data_length);
        _metadata = vec<char>(metadata_buffer, metadata_buffer + meta_data_length);
        return is;
    }

  protected:
    vec<char> _metadata;
};
};

#endif // LAMURE_META_CONTAINER_H