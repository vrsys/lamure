// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/vt/ooc/TileRequest.h>

namespace vt
{
namespace ooc
{
TileRequest::TileRequest() : Observable()
{
    _resource = nullptr;
    _aborted = false;
}

void TileRequest::setResource(pre::AtlasFile* resource) { _resource = resource; }

pre::AtlasFile* TileRequest::getResource() { return _resource; }

void TileRequest::setId(uint64_t id) { _id = id; }

uint64_t TileRequest::getId() { return _id; }

void TileRequest::erase() { this->inform(0); }

void TileRequest::abort() { _aborted = true; }

bool TileRequest::isAborted() { return _aborted; }
void TileRequest::setPriority(priority_type priority) { _priority = priority; }
priority_type TileRequest::getPriority() { return _priority; }
} // namespace ooc
} // namespace vt