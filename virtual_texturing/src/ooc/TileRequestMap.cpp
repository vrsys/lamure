// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/vt/ooc/TileRequestMap.h>

namespace vt
{
namespace ooc
{
TileRequestMap::TileRequestMap() : Observer() {}

TileRequestMap::~TileRequestMap()
{
    for(auto& iter : _map)
    {
        delete iter.second;
    }
}

TileRequest* TileRequestMap::getRequest(pre::AtlasFile* resource, uint64_t tile_id)
{
    std::lock_guard<std::mutex> lock(_mapLock);

    auto iter = _map.find(std::make_pair(resource, tile_id));

    if(iter == _map.end())
    {
        return nullptr;
    }

    return iter->second;
}

bool TileRequestMap::insertRequest(TileRequest* req)
{
    std::lock_guard<std::mutex> lock(_mapLock);

    auto resource = req->getResource();
    auto id = req->getId();
    auto iter = _map.find(std::make_pair(resource, id));

    if(iter == _map.end())
    {
        req->observe(0, this);
        auto pair = std::make_pair(resource, id);
        _map[pair] = req;

        return true;
    }

    return false;
}

void TileRequestMap::inform(event_type event, Observable* observable)
{
    bool empty;

    {
        std::unique_lock<std::mutex> lock(_mapLock);
        auto req = (TileRequest*)observable;
        _map.erase(std::make_pair(req->getResource(), req->getId()));
        delete req;

        empty = _map.empty();
    }

    if(empty)
    {
        _allRequestsProcessed.notify_all();
    }
}

bool TileRequestMap::waitUntilEmpty(std::chrono::milliseconds maxTime)
{
    std::unique_lock<std::mutex> lock(_mapLock);

    if(_map.empty())
    {
        return true;
    }

    return _allRequestsProcessed.wait_until(lock, std::chrono::system_clock::now() + maxTime, [this] { return _map.empty(); });
}
} // namespace ooc
} // namespace vt