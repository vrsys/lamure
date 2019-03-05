// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/vt/ooc/TileLoader.h>
#include <lamure/vt/VTConfig.h>

namespace vt
{
namespace ooc
{
TileLoader::TileLoader() : HeapProcessor() {}

void TileLoader::beforeStart() {}

bool TileLoader::process(TileRequest* req)
{
    if(!req->isAborted())
    {
        auto res = req->getResource();
        auto slot = _cache->requestSlotForWriting();

        if(slot == nullptr)
        {
            if(VTConfig::get_instance().is_verbose())
            {
                std::cerr << "LRU cache depletion reached." << std::endl;
            }
            req->erase();
            return false;
        }

        res->getTile(req->getId(), slot->getBuffer());

        // provide information on contained tile
        slot->setSize(res->getTileByteSize());
        slot->setResource(res);
        slot->setTileId(req->getId());

        // make slot accessible for reading
        _cache->registerOccupiedId(res, req->getId(), slot);
    }

    // erase request, because it is processed
    req->erase();
    // delete req;

    return true;
}

void TileLoader::beforeStop() {}
} // namespace ooc
} // namespace vt