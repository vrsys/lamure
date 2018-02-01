//
// Created by towe2387 on 2/1/18.
//

#ifndef LAMURE_OFFSETINDEX_H
#define LAMURE_OFFSETINDEX_H

#include "Index.h"
#include "AtlasFile.h"

namespace vt {
    namespace pre {
        class OffsetIndex : public Index<uint64_t>{
        private:
            const uint64_t EXISTS_BIT = 0x8000000000000000;

            AtlasFile::LAYOUT _layout;

            size_t _idToIdx(uint64_t id){
                size_t idx = 0;

                switch(_layout){
                    case AtlasFile::LAYOUT::RAW:
                        idx = id;
                        break;
                    case AtlasFile::LAYOUT::PACKED:
                        idx = id + 1;
                        break;
                    default:
                        throw std::runtime_error("Unknown file layout.");
                }

                return idx;
            }

        public:
            explicit OffsetIndex(size_t size, AtlasFile::LAYOUT layout) : Index(size + 1){
                _layout = layout;
            }

            bool exists(uint64_t id){
                return (_data[_idToIdx(id)] & EXISTS_BIT) != 0;
            }

            size_t getOffset(uint64_t id){
                return _data[_idToIdx(id)] & ~EXISTS_BIT;
            }

            size_t getByteSize(uint64_t id){
                size_t idx = _idToIdx(id);
                size_t nextIdx = 0;

                switch(_layout){
                    case AtlasFile::LAYOUT::RAW:
                        nextIdx = idx + 1;
                        break;
                    case AtlasFile::LAYOUT::PACKED:
                        nextIdx = idx - 1;
                        break;
                    default:
                        throw std::runtime_error("Unknown file layout.");
                }

                uint64_t offset = _data[idx] & ~EXISTS_BIT;
                uint64_t nextOffset = _data[nextIdx] & ~EXISTS_BIT;

                return nextOffset - offset;
            }

            void set(uint64_t id, uint64_t offset, size_t byteSize){
                size_t idx = _idToIdx(id);
                size_t nextIdx = 0;

                switch(_layout){
                    case AtlasFile::LAYOUT::RAW:
                        nextIdx = idx + 1;
                        break;
                    case AtlasFile::LAYOUT::PACKED:
                        nextIdx = idx - 1;
                        break;
                    default:
                        throw std::runtime_error("Unknown file layout.");
                }

                _data[idx] = offset | EXISTS_BIT;
                _data[nextIdx] = (offset + byteSize) & ~EXISTS_BIT;
            }
        };
    }
}


#endif //LAMURE_OFFSETINDEX_H
