// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef VT_OOC_TILECACHE_H
#define VT_OOC_TILECACHE_H

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <lamure/vt/platform.h>
#include <lamure/vt/pre/AtlasFile.h>
#include <map>
#include <mutex>
#include <queue>

namespace vt
{
namespace ooc
{
class TileCache;

class VT_DLL TileCacheSlot
{
  public:
    enum STATE
    {
        FREE = 1,
        WRITING = 2, // locked for writing
        READING = 3, // locked for reading
        OCCUPIED = 4
    };

  protected:
    STATE _state;
    uint8_t* _buffer;
    size_t _size;
    size_t _id;
    uint32_t _context_reference;

    pre::AtlasFile* _resource;
    uint64_t _tileId;

    TileCache* _cache;

  public:
    TileCacheSlot();
    ~TileCacheSlot();

    bool compareState(STATE state);

    void setTileId(uint64_t tileId);

    uint64_t getTileId();

    void setCache(TileCache* cache);

    void setState(STATE state);

    void setId(size_t id);

    size_t getId();

    void setBuffer(uint8_t* buffer);

    uint8_t* getBuffer();

    void setSize(size_t size);

    size_t getSize();

    void setResource(pre::AtlasFile* res);

    pre::AtlasFile* getResource();

    void addContextReference(uint16_t context_id);
    void removeContextReference(uint16_t context_id);
    uint16_t getContextReferenceCount();
    void removeAllContextReferences();
};

class VT_DLL TileCache
{
  protected:
    typedef TileCacheSlot slot_type;

    size_t _tileByteSize;
    size_t _slotCount;

    std::mutex* _locks;
    uint8_t* _buffer;
    slot_type* _slots;

    std::mutex _lruLock;
    std::condition_variable _lruRepopulationCV;
    std::queue<TileCacheSlot*> _lru;

    std::mutex _idsLock;
    std::map<std::pair<pre::AtlasFile*, uint64_t>, slot_type*> _ids;

  public:
    TileCache(size_t tileByteSize, size_t slotCount);
    ~TileCache();

    slot_type* requestSlotForReading(pre::AtlasFile* resource, uint64_t tile_id, uint16_t context_id);
    slot_type* requestSlotForWriting();

    void removeContextReferenceFromReadId(pre::AtlasFile* resource, uint64_t tile_id, uint16_t context_id);

    void registerOccupiedId(pre::AtlasFile* resource, uint64_t tile_id, slot_type* slot);
    void unregisterOccupiedId(pre::AtlasFile* resource, uint64_t tile_id);

    void waitUntilLRURepopulation(std::chrono::milliseconds maxTime = std::chrono::milliseconds::zero());

    void print();
};
} // namespace ooc
} // namespace vt

#endif // TILE_PROVIDER_TILECACHE_H
