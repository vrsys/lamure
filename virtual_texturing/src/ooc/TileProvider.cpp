// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/vt/ooc/TileProvider.h>

namespace vt
{
namespace ooc
{
TileProvider::TileProvider() : _resourcesLock(), _cacheLock()
{
    _cache = nullptr;
    _tileByteSize = 0;
}

TileProvider::~TileProvider()
{
    for(auto resource : _resources)
    {
        delete resource;
    }

    delete _cache;
}

void TileProvider::start(size_t maxMemSize)
{
    if(_tileByteSize == 0)
    {
        throw std::runtime_error("TileProvider tries to start loading Tiles of size 0.");
    }

    auto slotCount = maxMemSize / _tileByteSize;

    if(slotCount == 0)
    {
        throw std::runtime_error("TileProvider tries to start with Cache of size 0.");
    }

    _cache = new TileCache(_tileByteSize, slotCount);
    _loader.writeTo(_cache);
    _loader.start();
}

pre::AtlasFile* TileProvider::loadResource(const char* fileName)
{
    std::lock_guard<std::mutex> lock(_resourcesLock);

    for(auto atlas : _resources)
    {
        if(std::strcmp(atlas->getFileName(), fileName) == 0)
        {
            return atlas;
        }
    }

    auto atlas = new pre::AtlasFile(fileName);

    if(_tileByteSize == 0)
    {
        _pxFormat = atlas->getPixelFormat();
        _tilePxWidth = atlas->getTileWidth();
        _tilePxHeight = atlas->getTileHeight();
        _tileByteSize = atlas->getTileByteSize();
    }

    if(_pxFormat != atlas->getPixelFormat() || _tilePxWidth != atlas->getTileWidth() || _tilePxHeight != atlas->getTileHeight() || _tileByteSize != atlas->getTileByteSize())
    {
        throw std::runtime_error("Trying to add resource with conflicting format.");
    }

    _resources.insert(atlas);

    return atlas;
}

TileCacheSlot* TileProvider::getTile(pre::AtlasFile* resource, id_type tile_id, priority_type priority, uint16_t context_id)
{
    std::lock_guard<std::mutex> lock(_cacheLock);

    if(_cache == nullptr)
    {
        throw std::runtime_error("Trying to get Tile before starting TileProvider.");
    }

    auto slot = _cache->requestSlotForReading(resource, tile_id, context_id);

    if(slot != nullptr)
    {
        return slot;
    }

    auto req = _requestsMap.getRequest(resource, tile_id);

    if(req != nullptr)
    {
        // if one wants to rensert according to priority, this should happen here
        req->setPriority(std::max(req->getPriority(), priority));

        return nullptr;
    }

    req = new TileRequest();

    req->setResource(resource);
    req->setId(tile_id);
    req->setPriority(priority);

    _requestsMap.insertRequest(req);

    _loader.request(req);

    return nullptr;
}

void TileProvider::stop() { _loader.stop(); }

void TileProvider::print()
{
    std::lock_guard<std::mutex> lock(_cacheLock);
    _cache->print();
}

bool TileProvider::wait(std::chrono::milliseconds maxTime) { return _requestsMap.waitUntilEmpty(maxTime); }

void TileProvider::ungetTile(pre::AtlasFile* resource, id_type tile_id, uint16_t context_id)
{
    std::lock_guard<std::mutex> lock(_cacheLock);

    if(_cache == nullptr)
    {
        throw std::runtime_error("Trying to unget Tile before starting TileProvider.");
    }

    _cache->removeContextReferenceFromReadId(resource, tile_id, context_id);
}

} // namespace ooc
} // namespace vt
