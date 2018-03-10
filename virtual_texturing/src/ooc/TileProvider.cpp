#include <lamure/vt/ooc/TileProvider.h>

namespace vt{
    namespace ooc{
        TileProvider::TileProvider() {
            _cache = nullptr;
            _tileByteSize = 0;
        }

        TileProvider::~TileProvider(){
            for(auto resource : _resources){
                delete resource;
            }

            delete _cache;
        }

        void TileProvider::start(size_t maxMemSize){
            if(_tileByteSize == 0){
                throw std::runtime_error("TileProvider tries to start loading Tiles of size 0.");
            }

            auto slotCount = maxMemSize / _tileByteSize;

            if(slotCount == 0){
                throw std::runtime_error("TileProvider tries to start with Cache of size 0.");
            }

            _cache = new TileCache(_tileByteSize, slotCount);
            _loader.writeTo(_cache);
            _loader.start();
        }

        AtlasFile *TileProvider::addResource(const char *fileName){
            auto atlas = new AtlasFile(fileName);
            auto tileByteSize = atlas->getTileByteSize();

            if(tileByteSize > _tileByteSize){
                _tileByteSize = tileByteSize;
            }

            _resources.insert(atlas);

            return atlas;
        }

        TileCacheSlot *TileProvider::getTile(AtlasFile *resource, id_type id, priority_type priority){
            if(_cache == nullptr){
                throw std::runtime_error("Trying to retrieve Tile before starting TileProvider.");
            }

            auto slot = _cache->readSlotById(resource, id);

            if(slot != nullptr){
                return slot;
            }

            auto req = _requests.getRequest(resource, id);

            if(req != nullptr){
                req->setPriority(priority);

                return nullptr;
            }

            req = new TileRequest;

            req->setResource(resource);
            req->setId(id);
            req->setPriority(priority);

            _requests.insertRequest(req);

            _loader.request(req);

            return nullptr;
        }

        void TileProvider::ungetTile(TileCacheSlot *slot){
            _cache->setSlotReady(slot);
        }

        void TileProvider::stop(){
            _loader.stop();
        }

        void TileProvider::print(){
            _cache->print();
        }
    }
}
