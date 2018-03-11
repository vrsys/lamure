#include <lamure/vt/ooc/TileRequestMap.h>

namespace vt{
    namespace ooc{
        TileRequestMap::TileRequestMap() : Observer() {

        }

        TileRequestMap::~TileRequestMap(){
            for(auto iter = _map.begin(); iter != _map.end(); ++iter){
                delete iter->second;
            }
        }

        TileRequest *TileRequestMap::getRequest(pre::AtlasFile *resource, uint64_t id){
            std::lock_guard<std::mutex> lock(_lock);

            auto iter = _map.find(std::make_pair(resource, id));

            if(iter == _map.end()){
                return nullptr;
            }

            return iter->second;
        }

        bool TileRequestMap::insertRequest(TileRequest *req){
            std::lock_guard<std::mutex> lock(_lock);

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

        void TileRequestMap::inform(event_type event, Observable *observable){
            std::unique_lock<std::mutex> lock(_lock);
            auto req = (TileRequest*)observable;
            _map.erase(std::make_pair(req->getResource(), req->getId()));
            delete req;

            auto empty = _map.empty();

            lock.unlock();

            if(empty){
                _allRequestsProcessed.notify_all();
            }
        }

        bool TileRequestMap::waitUntilEmpty(std::chrono::milliseconds maxTime){
            std::unique_lock<std::mutex> lock(_lock);

            if(_map.empty()){
                return true;
            }

            _allRequestsProcessed.wait_for(lock, maxTime, [this]{
                return _map.empty();
            });
        }
    }
}