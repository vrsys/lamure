// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef VT_OOC_TILEREQUEST_H
#define VT_OOC_TILEREQUEST_H

#include <lamure/vt/platform.h>
#include <lamure/vt/pre/AtlasFile.h>
#include <lamure/vt/Observable.h>

namespace vt
{
namespace ooc
{
typedef float priority_type;
class VT_DLL TileRequest : public Observable
{
  protected:
    pre::AtlasFile* _resource;
    uint64_t _id;
    priority_type _priority;
    bool _aborted;

  public:
    explicit TileRequest();

    void setResource(pre::AtlasFile* resource);

    pre::AtlasFile* getResource();

    void setId(uint64_t id);

    uint64_t getId();

    void setPriority(priority_type priority);

    priority_type getPriority();

    void erase();

    void abort();

    bool isAborted();
};
} // namespace ooc
} // namespace vt

#endif // TILE_PROVIDER_TILEREQUEST_H
