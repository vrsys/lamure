#include <lamure/vt/ooc/TileCache.h>

namespace vt {
    namespace ooc {
        typedef TileCacheSlot slot_type;

        TileCacheSlot::TileCacheSlot(){
            _state = STATE::FREE;
            _buffer = nullptr;
            _cache = nullptr;
            _size = 0;
            _tileId = 0;
        }

        TileCacheSlot::~TileCacheSlot(){
            //std::cout << "del slot " << this << std::endl;
        }

        bool TileCacheSlot::hasState(STATE state){
            return _state == state;
        }

        void TileCacheSlot::setTileId(uint64_t tileId){
            _tileId = tileId;
        }

        uint64_t TileCacheSlot::getTileId(){
            return _tileId;
        }

        void TileCacheSlot::setCache(TileCache *cache){
            _cache = cache;
        }

        void TileCacheSlot::setState(STATE state){
            _state = state;
        }

        void TileCacheSlot::setId(size_t id){
            _id = id;
        }

        size_t TileCacheSlot::getId(){
            return _id;
        }

        void TileCacheSlot::setBuffer(uint8_t *buffer){
            _buffer = buffer;
        }

        uint8_t *TileCacheSlot::getBuffer(){
            return _buffer;
        }

        void TileCacheSlot::setSize(size_t size){
            _size = size;
        }

        size_t TileCacheSlot::getSize(){
            return _size;
        }

        void TileCacheSlot::updateLastUsed(){
            uint64_t micro = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now()).time_since_epoch().count();
            this->setPriority(UINT64_MAX - micro);
        }

        uint64_t TileCacheSlot::getLastUsed(){
            return UINT64_MAX - this->getPriority();
        }

        /*void TileCacheSlot::setAssocData(assoc_data_type assocData){
            _assocData = assocData;
        }

        assoc_data_type TileCacheSlot::getAssocData(){
            return _assocData;
        }*/

        void TileCacheSlot::removeFromLRU(){
            PriorityHeapContent<uint64_t>::remove();
        }

        void TileCacheSlot::setResource(AtlasFile* res){
            _resource = res;
        }

        AtlasFile *TileCacheSlot::getResource(){
            return _resource;
        }

        void TileCacheSlot::removeFromIDS(){
            std::lock_guard<std::mutex> lock(this->_lock);

            if(_cache != nullptr){
                _cache->unregisterId(_resource, _tileId);
            }
        }

        TileCache::TileCache(size_t tileByteSize, size_t slotCount) : _leastRecentlyUsed(0, nullptr) {
            _tileByteSize = tileByteSize;
            _slotCount = slotCount;
            _buffer = new uint8_t[tileByteSize * slotCount];
            _slots = new slot_type[slotCount];
            _locks = new std::mutex[slotCount];

            //_counter = 0;

            for(size_t i = 0; i < slotCount; ++i){
                _slots[i].setId(i);
                _slots[i].setBuffer(&_buffer[tileByteSize * i]);
                _slots[i].setCache(this);
                _slots[i].updateLastUsed();

                _leastRecentlyUsed.push(_slots[i].getPriority(), &_slots[i]);
            }
        }

        slot_type *TileCache::readSlotById(AtlasFile *resource, uint64_t id){
            std::lock_guard<std::mutex> lock(_idsLock);

            auto iter = _ids.find(std::make_pair(resource, id));

            if(iter == _ids.end()){
                return nullptr;
            }

            auto slot = iter->second;

            slot->removeFromLRU();
            slot->setState(slot_type::STATE::READING);

            return slot;
        }

        slot_type *TileCache::writeSlot(std::chrono::milliseconds maxTime){
            PriorityHeapContent<uint64_t> *content;

            if(!_leastRecentlyUsed.pop(content, maxTime)){
                return nullptr;
            }

            auto slot = (slot_type*)content;

            if(!slot->hasState(slot_type::STATE::FREE)) {
                slot->removeFromIDS();
            }

            slot->setState(slot_type::STATE::WRITING);

            return slot;
        }

        void TileCache::setSlotReady(slot_type *slot){
            std::lock_guard<std::mutex> lockSlot(_locks[slot->getId()]);

            if(slot->hasState(slot_type::STATE::WRITING)){
                std::lock_guard<std::mutex> lock(_idsLock);
                _ids.insert(std::make_pair(std::make_pair(slot->getResource(), slot->getTileId()), slot));
            }

            slot->setState(slot_type::STATE::OCCUPIED);
            slot->updateLastUsed();

            _leastRecentlyUsed.push(slot->getPriority(), slot);
        }

        void TileCache::unregisterId(AtlasFile *resource, uint64_t id){
            _ids.erase(std::make_pair(resource, id));
        }

        TileCache::~TileCache(){
            delete[] _buffer;
            delete[] _slots;
            delete[] _locks;
        }

        void TileCache::print(){
            std::cout << "Slots: " << std::endl;

            for(size_t i = 0; i < _slotCount; ++i){
                auto slot = &_slots[i];

                if(slot->hasState(slot_type::STATE::FREE)) {
                    std::cout << "\tx ";
                }else{
                    std::cout << "\t  ";
                }

                std::cout << slot->getTileId() << std::endl;
            }

            std::cout << std::endl << "IDs:" << std::endl;

            for(auto pair : _ids){
                std::cout << "\t" << pair.first.first << " " << pair.first.second << " --> " << pair.second->getResource() << " " << pair.second->getTileId() << std::endl;
            }

            std::cout << std::endl;
        }
    }
}