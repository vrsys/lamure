//
// Created by sebastian on 26.02.18.
//

#ifndef VT_OOC_TILEPROVIDER_H
#define VT_OOC_TILEPROVIDER_H

#include <cstddef>
#include <cstdint>
#include <set>
#include <lamure/vt/pre/AtlasFile.h>
#include <lamure/vt/ooc/TileRequestMap.h>
#include <lamure/vt/ooc/TileCache.h>
#include <lamure/vt/ooc/TileLoader.h>

namespace vt{
    namespace ooc{
        typedef uint64_t id_type;
        typedef uint32_t priority_type;

        class TileProvider {
        protected:
            std::set<AtlasFile*> _resources;
            TileRequestMap _requests;
            TileLoader _loader;
            TileCache *_cache;
            size_t _tileByteSize;

        public:
            TileProvider();

            ~TileProvider();

            void start(size_t maxMemSize);

            AtlasFile *addResource(const char *fileName);

            TileCacheSlot *getTile(AtlasFile *resource, id_type id, priority_type priority);

            void ungetTile(TileCacheSlot *slot);

            void stop();

            void print();

            bool wait(std::chrono::milliseconds maxTime = std::chrono::milliseconds::zero());
        };
    }
}

#endif //TILE_PROVIDER_TILEPROVIDER_H
