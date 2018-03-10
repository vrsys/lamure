//
// Created by sebastian on 26.02.18.
//

#ifndef VT_OOC_TILEREQUESTMAP_H
#define VT_OOC_TILEREQUESTMAP_H


#include <cstdint>
#include <lamure/vt/pre/AtlasFile.h>
#include <lamure/vt/ooc/TileRequest.h>
#include <lamure/vt/Observer.h>

namespace vt{
    namespace ooc{
        class TileRequestMap : public Observer {
        protected:
            std::map<std::pair<AtlasFile*, uint64_t>, TileRequest*> _map;
            std::mutex _lock;

            std::condition_variable _allRequestsProcessed;

        public:
            TileRequestMap();

            ~TileRequestMap();

            TileRequest *getRequest(AtlasFile *resource, uint64_t id);

            bool insertRequest(TileRequest *req);

            void inform(seb::event_type event, seb::Observable *observable);

            bool waitUntilEmpty(std::chrono::milliseconds maxTime = std::chrono::milliseconds::zero());
        };
    }
}


#endif //TILE_PROVIDER_TILEREQUESTMAP_H
