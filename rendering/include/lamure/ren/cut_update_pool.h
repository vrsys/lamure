// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef REN_CUT_UPDATE_POOL_H_
#define REN_CUT_UPDATE_POOL_H_

#include <vector>
#include <lamure/ren/semaphore.h>
#include <lamure/utils.h>
#include <lamure/ren/config.h>

#include <lamure/ren/model_database.h>
#include <lamure/ren/cut_database.h>
#include <lamure/ren/policy.h>

#include <lamure/ren/cut.h>
#include <lamure/ren/ooc_cache.h>
#include <lamure/ren/gpu_cache.h>
#include <lamure/memory_status.h>
#include <lamure/ren/camera.h>
#include <lamure/ren/cut_update_queue.h>
#include <lamure/ren/cut_update_index.h>


namespace lamure {
namespace ren {

class CutUpdatePool
{
public:
                            CutUpdatePool(const context_t context_id,
                                const node_t upload_budget_in_nodes,
                                const node_t render_budget_in_nodes);
    virtual                 ~CutUpdatePool();

    const uint32_t          num_threads() const { return num_threads_; };

    void                    DispatchCutUpdate(char* current_gpu_storage_A, char* current_gpu_storage_B);
    const bool              IsRunning();

protected:
    void                    Initialize();
    const bool              Prepare();

    void                    SplitNode(const CutUpdateIndex::Action& item);
    void                    CollapseNode(const CutUpdateIndex::Action& item);
    void                    CutUpdateSplitAgain(const CutUpdateIndex::Action& split_action);

    const bool              IsAllNodesInCut(const model_t model_id, const std::vector<node_t>& node_ids, const std::set<node_t>& cut);
    const bool              IsNodeInFrustum(const view_t view_id, const model_t model_id, const node_t node_id, const scm::gl::frustum& frustum);
    const bool              IsNoNodeInFrustum(const view_t view_id, const model_t model_id, const std::vector<node_t>& node_ids, const scm::gl::frustum& frustum);

    const float             CalculateNodeError(const view_t view_id, const model_t model_id, const node_t node_id);


    /*virtual*/ void        Run();
    void                    Shutdown();

    void                    CutMaster();
    void                    CutAnalysis(view_t view_id, model_t model_id);
    void                    CutUpdate();
    void                    compileTransferList();
    void                    compilerenderList();
#ifdef LAMURE_CUT_UPDATE_ENABLE_PREFETCHING
    void                    PrefetchRoutine();             
#endif

private:
    bool                    is_shutdown();

    context_t               context_id_;
    
    bool                    locked_;
    semaphore               semaphore_;
    std::mutex              mutex_;

    uint32_t                num_threads_;
    std::vector<std::thread> threads_;

    bool                    shutdown_;

    CutUpdateQueue          job_queue_;

    GpuCache*               gpu_cache_;
    CutUpdateIndex*         index_;

    std::vector<CutdatabaseRecord::SlotUpdateDescr> transfer_list_;
    std::vector<std::vector<std::vector<Cut::NodeSlotAggregate>>> render_list_;

    char*                   current_gpu_storage_A_;
    char*                   current_gpu_storage_B_;
    char*                   current_gpu_storage_;

    CutdatabaseRecord::Temporarybuffer current_gpu_buffer_;

    std::map<view_t, Camera> user_cameras_;
    std::map<view_t, float> height_divided_by_top_minus_bottoms_;
    std::map<model_t, scm::math::mat4f> model_transforms_;
    std::map<model_t, float> model_thresholds_;

    scm::math::mat4f        previous_camera_view_;

    size_t                  upload_budget_in_nodes_;
    size_t                  render_budget_in_nodes_;
    size_t                  out_of_core_budget_in_nodes_;

#ifdef LAMURE_CUT_UPDATE_ENABLE_MODEL_TIMEOUT
    size_t                  cut_update_counter_;
    std::map<model_t, size_t> model_freshness_;
#endif

#ifdef LAMURE_CUT_UPDATE_ENABLE_PREFETCHING
    std::vector<CutUpdateIndex::Action> pending_prefetch_set_;
#endif


#ifdef LAMURE_CUT_UPDATE_ENABLE_REPEAT_MODE
    boost::timer::cpu_timer master_timer_;

    boost::timer::nanosecond_type last_frame_elapsed_;
#endif

    semaphore               master_semaphore_;
    bool                    master_dispatched_;

};


} } // namespace lamure


#endif // REN_CUT_UPDATE_POOL_H_
