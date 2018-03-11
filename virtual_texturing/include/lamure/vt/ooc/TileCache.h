//
// Created by sebastian on 06.03.18.
//

#ifndef VT_OOC_TILECACHE_H
#define VT_OOC_TILECACHE_H

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <iostream>
#include <lamure/vt/PriorityHeap.h>
#include <lamure/vt/pre/AtlasFile.h>

namespace vt {
    namespace ooc {
        class TileCache;

        class TileCacheSlot : public PriorityHeapContent<uint64_t> {
        public:
            enum STATE{
                FREE = 1,
                WRITING,
                READING,
                OCCUPIED
            };

        protected:
            STATE _state;
            uint8_t *_buffer;
            size_t _size;
            size_t _id;
            //assoc_data_type _assocData;

            AtlasFile *_resource;
            uint64_t _tileId;

            TileCache *_cache;


        public:
            TileCacheSlot();

            ~TileCacheSlot();

            bool hasState(STATE state);

            void setTileId(uint64_t tileId);

            uint64_t getTileId();

            void setCache(TileCache *cache);

            void setState(STATE state);

            void setId(size_t id);

            size_t getId();

            void setBuffer(uint8_t *buffer);

            uint8_t *getBuffer();

            void setSize(size_t size);

            size_t getSize();

            void updateLastUsed();

            uint64_t getLastUsed();

            /*void setAssocData(assoc_data_type assocData);

            assoc_data_type getAssocData();*/

            void removeFromLRU();

            void setResource(AtlasFile* res);

            AtlasFile *getResource();

            void removeFromIDS();
        };

        class TileCache {
        protected:
            typedef TileCacheSlot slot_type;

            size_t _tileByteSize;
            size_t _slotCount;
            uint8_t *_buffer;
            slot_type *_slots;
            std::mutex *_locks;

            PriorityHeap<uint64_t> _leastRecentlyUsed;

            std::mutex _idsLock;
            std::map<std::pair<AtlasFile *, uint64_t>, slot_type *> _ids;

        public:
            TileCache(size_t tileByteSize, size_t slotCount);

            slot_type *readSlotById(AtlasFile *resource, uint64_t id);

            slot_type *writeSlot(std::chrono::milliseconds maxTime = std::chrono::milliseconds::zero());

            void setSlotReady(slot_type *slot);

            void unregisterId(AtlasFile *resource, uint64_t id);

            ~TileCache();

            void print();
        };
    }
}


#endif //TILE_PROVIDER_TILECACHE_H
