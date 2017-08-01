#ifndef LAMURE_META_CONTAINER_H
#define LAMURE_META_CONTAINER_H

#include <lamure/pro/common.h>

namespace prov
{
class MetaData
{
  public:
    const vec<char> &get_metadata() const { return _metadata; }
    MetaData() {}

    virtual void read_metadata(ifstream &is, uint32_t meta_data_length)
    {
        _metadata = vec<char>(meta_data_length, 0);
        is.read(&_metadata[0], meta_data_length);

        std::stringstream sstr;
        sstr << "0x";

        for(uint32_t i = 0; i < meta_data_length; i++)
        {
            sstr << std::uppercase << std::hex << _metadata[i];
        }

        printf("\nMeta data read: %s", sstr.str().c_str());
    }

  protected:
    vec<char> _metadata;
};
};

#endif // LAMURE_META_CONTAINER_H