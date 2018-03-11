//
// Created by sebastian on 26.02.18.
//

#ifndef VT_OOC_TILEREQUEST_H
#define VT_OOC_TILEREQUEST_H


#include <lamure/vt/PriorityHeap.h>
#include <lamure/vt/pre/AtlasFile.h>
#include <lamure/vt/Observable.h>

namespace vt{
    namespace ooc{
        class TileRequest : public PriorityHeapContent<uint32_t>, public Observable {
        protected:
            AtlasFile *_resource;
            uint64_t _id;

        public:
            explicit TileRequest();

            void setResource(AtlasFile *resource);

            AtlasFile *getResource();

            void setId(uint64_t id);

            uint64_t getId();

            void erase();
        };
    }
}


#endif //TILE_PROVIDER_TILEREQUEST_H
