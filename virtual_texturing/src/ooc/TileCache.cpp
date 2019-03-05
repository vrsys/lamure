// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/vt/ooc/TileCache.h>
#include <lamure/vt/VTConfig.h>

namespace vt
{
namespace ooc
{
typedef TileCacheSlot slot_type;

TileCacheSlot::TileCacheSlot()
{
    _state = STATE::FREE;
    _buffer = nullptr;
    _cache = nullptr;
    _size = 0;
    _tileId = 0;
    _context_reference = 0;
}

TileCacheSlot::~TileCacheSlot()
{
    // std::cout << "del slot " << this << std::endl;
}

bool TileCacheSlot::compareState(STATE state) { return _state == state; }

void TileCacheSlot::setTileId(uint64_t tileId) { _tileId = tileId; }

uint64_t TileCacheSlot::getTileId() { return _tileId; }

void TileCacheSlot::setCache(TileCache* cache) { _cache = cache; }

void TileCacheSlot::setState(STATE state) { _state = state; }

void TileCacheSlot::setId(size_t id) { _id = id; }

size_t TileCacheSlot::getId() { return _id; }

void TileCacheSlot::setBuffer(uint8_t* buffer) { _buffer = buffer; }

uint8_t* TileCacheSlot::getBuffer() { return _buffer; }

void TileCacheSlot::setSize(size_t size) { _size = size; }

size_t TileCacheSlot::getSize() { return _size; }

void TileCacheSlot::setResource(pre::AtlasFile* res) { _resource = res; }

pre::AtlasFile* TileCacheSlot::getResource() { return _resource; }

void TileCacheSlot::addContextReference(uint16_t context_id)
{
    if(context_id > 32)
    {
        throw std::runtime_error("Only 32 contexts are supported");
    }

    _context_reference |= 1UL << context_id;
}
void TileCacheSlot::removeContextReference(uint16_t context_id)
{
    if(context_id > 32)
    {
        throw std::runtime_error("Only 32 contexts are supported");
    }

    _context_reference &= ~(1UL << context_id);
}
static const unsigned int num_to_bits[16] = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4};

unsigned int countSetBitsRec(unsigned int num)
{
    int nibble = 0;
    if(0 == num)
        return num_to_bits[0];

    nibble = num & 0xf;

    return num_to_bits[nibble] + countSetBitsRec(num >> 4);
}

uint16_t TileCacheSlot::getContextReferenceCount() { return (uint16_t)countSetBitsRec(_context_reference); }
void TileCacheSlot::removeAllContextReferences() { _context_reference = 0u; }
TileCache::TileCache(size_t tileByteSize, size_t slotCount)
{
    _tileByteSize = tileByteSize;
    _slotCount = slotCount;
    _buffer = new uint8_t[tileByteSize * slotCount];
    _slots = new slot_type[slotCount];
    _locks = new std::mutex[slotCount];

    for(size_t i = 0; i < slotCount; ++i)
    {
        _slots[i].setId(i);
        _slots[i].setBuffer(&_buffer[tileByteSize * i]);
        _slots[i].setCache(this);

        _lru.push(&_slots[i]);
    }
}

slot_type* TileCache::requestSlotForReading(pre::AtlasFile* resource, uint64_t tile_id, uint16_t context_id)
{
    std::lock_guard<std::mutex> lock(_idsLock);

    auto iter = _ids.find(std::make_pair(resource, tile_id));

    if(iter == _ids.end())
    {
        return nullptr;
    }

    auto slot = iter->second;

    if(slot == nullptr)
    {
        if(VTConfig::get_instance().is_verbose())
        {
            std::cerr << "IDX slot is null." << std::endl;
        }
        return nullptr;
    }

    {
        std::lock_guard<std::mutex> lockSlot(_locks[slot->getId()]);
        slot->addContextReference(context_id);
        slot->setState(slot_type::STATE::READING);
    }

    return slot;
}

slot_type* TileCache::requestSlotForWriting()
{
    std::lock_guard<std::mutex> lock(_lruLock);
    while(!_lru.empty())
    {
        TileCacheSlot* slot = _lru.front();
        _lru.pop();

        {
            std::lock_guard<std::mutex> lockSlot(_locks[slot->getId()]);
            if(slot->compareState(TileCacheSlot::STATE::FREE))
            {
                slot->setState(slot_type::STATE::WRITING);
                return slot;
            }
            else if(slot->compareState(TileCacheSlot::STATE::OCCUPIED))
            {
                unregisterOccupiedId(slot->getResource(), slot->getTileId());
                slot->removeAllContextReferences();
                slot->setState(slot_type::STATE::WRITING);
                return slot;
            }
        }
    };

    return nullptr;
}

void TileCache::registerOccupiedId(pre::AtlasFile* resource, uint64_t tile_id, slot_type* slot)
{
    std::unique_lock<std::mutex> lockSlot(_locks[slot->getId()], std::defer_lock);
    std::unique_lock<std::mutex> lock(_idsLock, std::defer_lock);
    std::unique_lock<std::mutex> lockLRU(_lruLock, std::defer_lock);

    std::lock(lockSlot, lock, lockLRU); // transition to scoped_lock when migrating to C++17

    if(slot->compareState(TileCacheSlot::WRITING))
    {
        _ids.insert(std::make_pair(std::make_pair(resource, tile_id), slot));
        slot->setState(slot_type::STATE::OCCUPIED);

        _lru.push(slot);
        _lruRepopulationCV.notify_one();
    }
}

void TileCache::removeContextReferenceFromReadId(pre::AtlasFile* resource, uint64_t tile_id, uint16_t context_id)
{
    TileCacheSlot* slot = nullptr;

    {
        std::lock_guard<std::mutex> lock(_idsLock);

        auto iter = _ids.find(std::make_pair(resource, tile_id));

        if(iter == _ids.end())
        {
            if(VTConfig::get_instance().is_verbose())
            {
                std::cerr << "Context reference removal from a slot, which does not exist in IDX." << std::endl;
            }
            return;
        }

        slot = iter->second;
    }

    if(slot == nullptr || !slot->compareState(TileCacheSlot::READING))
    {
        if(VTConfig::get_instance().is_verbose())
        {
            std::cerr << "Context reference removal from a slot, which is not read." << std::endl;
        }
        return;
    }

    {
        std::unique_lock<std::mutex> lockSlot(_locks[slot->getId()], std::defer_lock);
        std::unique_lock<std::mutex> lock(_idsLock, std::defer_lock);
        std::unique_lock<std::mutex> lockLRU(_lruLock, std::defer_lock);

        std::lock(lockSlot, lock, lockLRU); // transition to scoped_lock when migrating to C++17

        slot->removeContextReference(context_id);

        if(slot->getContextReferenceCount() == 0)
        {
            slot->setState(TileCacheSlot::OCCUPIED);
            _lru.push(slot);
            _lruRepopulationCV.notify_one();
        }
    }
}

void TileCache::unregisterOccupiedId(pre::AtlasFile* resource, uint64_t tile_id)
{
    std::lock_guard<std::mutex> lock(_idsLock);

    auto iter = _ids.find(std::make_pair(resource, tile_id));

    if(iter == _ids.end())
    {
        if(VTConfig::get_instance().is_verbose())
        {
            std::cerr << "Erasing IDX slot, which does not exist in IDX." << std::endl;
        }
        return;
    }

    _ids.erase(iter);
}

TileCache::~TileCache()
{
    delete[] _buffer;
    delete[] _slots;
    delete[] _locks;
}

void TileCache::print()
{
    std::cout << "Slots: " << std::endl;

    for(size_t i = 0; i < _slotCount; ++i)
    {
        auto slot = &_slots[i];

        if(slot->compareState(slot_type::STATE::FREE))
        {
            std::cout << "\tx ";
        }
        else
        {
            std::cout << "\t  ";
        }

        std::cout << slot->getTileId() << std::endl;
    }

    std::cout << std::endl << "IDs:" << std::endl;

    for(auto pair : _ids)
    {
        std::cout << "\t" << pair.second->getId() << " " << pair.first.first << " " << pair.first.second << " --> " << pair.second->getResource() << " " << pair.second->getTileId() << std::endl;
    }

    std::cout << std::endl;
}
void TileCache::waitUntilLRURepopulation(std::chrono::milliseconds maxTime)
{
    std::unique_lock<std::mutex> lk(_lruLock);

    if(!_lru.empty())
    {
        return;
    }

    _lruRepopulationCV.wait_until(lk, std::chrono::system_clock::now() + maxTime, [&]() -> bool { return !_lru.empty(); });
}
} // namespace ooc
} // namespace vt