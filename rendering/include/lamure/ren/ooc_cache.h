// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef REN_OOC_CACHE_H_
#define REN_OOC_CACHE_H_

#include <map>
#include <queue>
#include <lamure/utils.h>
#include <lamure/ren/config.h>
#include <lamure/ren/cache.h>
#include <lamure/ren/ooc_pool.h>

#include <lamure/ren/platform.h>
#include <lamure/ren/model_database.h>
#include <lamure/ren/policy.h>


namespace lamure {
namespace ren {

class RENDERING_DLL OocCache : public Cache
{
public:

                        OocCache(const OocCache&) = delete;
                        OocCache& operator=(const OocCache&) = delete;
    virtual             ~OocCache();

    static OocCache*    GetInstance();

    void                RegisterNode(const model_t model_id, const node_t node_id, const int32_t priority);
    char*               NodeData(const model_t model_id, const node_t node_id);

    const bool          IsNodeResidentAndAquired(const model_t model_id, const node_t node_id);

    void                Refresh();

    void                LockPool();
    void                UnlockPool();

    void                StartMeasure();
    void                EndMeasure();

protected:

                        OocCache(const size_t num_slots);
    static bool         is_instanced_;
    static OocCache*    single_;

private:
    static std::mutex   mutex_;

    char*               cache_data_;
    uint32_t            maintenance_counter_;
    OocPool*            pool_;
};


} } // namespace lamure


#endif // REN_OOC_CACHE_H_
