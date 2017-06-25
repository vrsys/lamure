#ifndef LAMURE_LODSTREAM_H
#define LAMURE_LODSTREAM_H

#include "lamure/pro/data/entities/DensePoint.h"
#include "lamure/pro/data/entities/LoDMetaData.h"
#include "lamure/pro/data/interfaces/Streamable.h"

namespace prov
{
class LoDMetaStream : public Streamable<LoDMetaData>
{
  public:
    LoDMetaStream(ifstream &is) : Streamable(is) {}
    const LoDMetaData &access_at_implicit(uint32_t index) override
    {
        uint32_t sought_pos = Readable::HEADER_LENGTH + LoDMetaData::ENTITY_LENGTH * index;

        if(is.tellg() < sought_pos)
        {
            is.ignore(sought_pos - is.tellg());
        }
        else
        {
            is.clear();
            is.seekg(sought_pos);
        }

        LoDMetaData data;
        data.read_metadata(is, LoDMetaData::ENTITY_LENGTH);
        return data;
    }

    const vec<LoDMetaData> access_at_implicit_range(uint32_t index_start, uint32_t index_end) override
    {
        uint32_t sought_pos = Readable::HEADER_LENGTH + LoDMetaData::ENTITY_LENGTH * index_start;

        if(is.tellg() < sought_pos)
        {
            is.ignore(sought_pos - is.tellg());
        }
        else
        {
            is.clear();
            is.seekg(sought_pos);
        }

        vec<LoDMetaData> range;

        for(int i = 0; i < index_end - index_start; i++)
        {
            LoDMetaData data;
            data.read_metadata(is, LoDMetaData::ENTITY_LENGTH);
            range.push_back(data);
        }

        return range;
    }
};
}

#endif // LAMURE_LODSTREAM_H
