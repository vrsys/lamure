#include <lamure/vt/ooc/TileRequestMap.h>

namespace vt{
    namespace ooc{
        TileRequestMap::TileRequestMap() : seb::Observer() {

        }

        TileRequestMap::~TileRequestMap(){
            for(auto iter = _map.begin(); iter != _map.end(); ++iter){
                delete iter->second;
            }
        }

        TileRequest *TileRequestMap::getRequest(AtlasFile *resource, uint64_t id){
            std::lock_guard<mutex> lock(_lock);

            auto iter = _map.find(std::make_pair(resource, id));

            if(iter == _map.end()){
                return nullptr;
            }

            return iter->second;
        }

        bool TileRequestMap::insertRequest(TileRequest *req){
            std::lock_guard<mutex> lock(_lock);

            auto resource = req->getResource();
            auto id = req->getId();
            auto iter = _map.find(std::make_pair(resource, id));

            if(iter == _map.end()){
                req->observe(0, this);
                _map.insert(std::make_pair(std::make_pair(resource, id), req));

                return true;
            }

            return false;
        }

        void TileRequestMap::inform(seb::event_type event, seb::Observable *observable){
            std::lock_guard<mutex> lock(_lock);
            auto req = (TileRequest*)observable;
            _map.erase(std::make_pair(req->getResource(), req->getId()));
            delete req;
        }
    }
}