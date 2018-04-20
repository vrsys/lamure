//
// Created by towe2387 on 2/1/18.
//

#ifndef LAMURE_CIELABINDEX_H
#define LAMURE_CIELABINDEX_H

#include <lamure/vt/pre/Index.h>

namespace vt {
    namespace pre {
        class CielabIndex : public Index<uint32_t>{
        private:
            uint32_t _convert(float val);
            float _convert(uint32_t val);
        public:
            CielabIndex(size_t size);
            float getCielabValue(uint64_t id);
            void set(uint64_t id, float cielabValue);
        };
    }
}


#endif //LAMURE_CIELABINDEX_H
