#include <lamure/vt/ooc/TileLoader.h>

namespace vt {
    namespace ooc {
        TileLoader::TileLoader() : HeapProcessor(){

        }

        void TileLoader::beforeStart(){
        }

        void TileLoader::process(TileRequest *req){
            auto res = req->getResource();
            auto slot = _cache->writeSlot(std::chrono::milliseconds(10));

            if(slot == nullptr){
                throw std::runtime_error("Cache seems to be full.");
            }

            res->getTile(req->getId(), slot->getBuffer());

            // provide information on contained tile
            slot->setSize(res->getTileByteSize());
            slot->setResource(res);
            slot->setTileId(req->getId());

            // make slot accessible for reading
            _cache->setSlotReady(slot);

            // erase request, because it is processed
            req->erase();
        }

        void TileLoader::beforeStop(){

        }
    }
}