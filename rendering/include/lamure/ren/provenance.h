// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef REN_PROVENANCE_H_
#define REN_PROVENANCE_H_

#include <lamure/ren/cache_index.h>
#include <lamure/ren/cache_queue.h>
#include <lamure/ren/config.h>
#include <lamure/ren/lod_stream.h>
#include <lamure/ren/model_database.h>
#include <lamure/ren/provenance_stream.h>
#include <lamure/ren/semaphore.h>
#include <lamure/types.h>
#include <lamure/utils.h>
#include <map>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

namespace lamure
{
namespace ren
{
struct provenance_data
{
    float dummy;
};
}
} // namespace lamure

#endif // REN_PROVENANCE_H_
