// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef VT_OOC_TILEPROVIDER_H
#define VT_OOC_TILEPROVIDER_H

#include <cstddef>
#include <cstdint>
#include <set>
#include <lamure/vt/pre/AtlasFile.h>
#include <lamure/vt/ooc/TileRequestMap.h>
#include <lamure/vt/ooc/TileCache.h>
#include <lamure/vt/ooc/TileLoader.h>

namespace vt
{
namespace ooc
{
typedef uint64_t id_type;
typedef uint32_t priority_type;

class TileProvider
{
  protected:
    std::mutex _resourcesLock;
    std::set<pre::AtlasFile*> _resources;
    TileRequestMap _requestsMap;
    TileLoader _loader;

    std::mutex _cacheLock;
    TileCache* _cache;

    pre::Bitmap::PIXEL_FORMAT _pxFormat;
    size_t _tilePxWidth;
    size_t _tilePxHeight;
    size_t _tileByteSize;

  public:
    TileProvider();

    ~TileProvider();

    void start(size_t maxMemSize);

    pre::AtlasFile* loadResource(const char* fileName);

    TileCacheSlot* getTile(pre::AtlasFile* resource, id_type id, priority_type priority, uint16_t context_id);
    void ungetTile(pre::AtlasFile* resource, id_type id, uint16_t context_id);

    void stop();

    void print();

    bool wait(std::chrono::milliseconds maxTime = std::chrono::milliseconds::zero());
};
} // namespace ooc
} // namespace vt

#endif // TILE_PROVIDER_TILEPROVIDER_H
