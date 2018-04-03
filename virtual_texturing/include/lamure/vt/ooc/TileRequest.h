//
// Created by sebastian on 26.02.18.
//

#ifndef VT_OOC_TILEREQUEST_H
#define VT_OOC_TILEREQUEST_H


//#include <lamure/vt/PriorityHeap.h>
#include <lamure/vt/pre/AtlasFile.h>
#include <lamure/vt/Observable.h>

namespace vt{
    namespace ooc{
        class TileRequest : /*public PriorityHeapContent<uint32_t>,*/ public Observable {
        protected:
            pre::AtlasFile *_resource;
            uint64_t _id;
            uint32_t _priority;
            bool _aborted;

        public:
            explicit TileRequest();

            void setResource(pre::AtlasFile *resource);

            pre::AtlasFile *getResource();

            void setId(uint64_t id);

            uint64_t getId();

            void setPriority(uint32_t priority){
                _priority = priority;
            }

            uint32_t getPriority(){
                return _priority;
            }

            void erase();

            void abort();

            bool isAborted();
        };
    }
}


#endif //TILE_PROVIDER_TILEREQUEST_H
