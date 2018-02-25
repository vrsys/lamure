#include <lamure/vt/pre/CielabIndex.h>

namespace vt {
    namespace pre {
            uint32_t CielabIndex::_convert(float val){
                auto *ptr = (uint32_t*)&val;
                return *ptr;
            }

            float CielabIndex::_convert(uint32_t val){
                auto *ptr = (float*)&val;
                return *ptr;
            }

            CielabIndex::CielabIndex(size_t size) : Index<uint32_t>(size) {

            }

            float CielabIndex::getCielabValue(uint64_t id){
                return _convert(this->_data[id]);
            }

            void CielabIndex::set(uint64_t id, float cielabValue){
                this->_data[id] = _convert(cielabValue);
            }
    }
}