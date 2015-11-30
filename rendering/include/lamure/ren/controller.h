// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef REN_CONTROLLER_H_
#define REN_CONTROLLER_H_

#include <unordered_map>
#include <lamure/utils.h>
#include <lamure/types.h>

#include <lamure/ren/platform.h>
#include <lamure/ren/cut_update_pool.h>
#include <lamure/ren/gpu_context.h>


namespace lamure {
namespace ren {

class RENDERING_DLL Controller
{
public:
    typedef size_t gua_context_desc_t;
    typedef view_t gua_view_desc_t;
    typedef std::string gua_model_desc_t;

    static Controller* get_instance();

                        Controller(const Controller&) = delete;
                        Controller& operator=(const Controller&) = delete;
    virtual             ~Controller();

    void                SignalSystemreset();
    void                resetSystem();
    const bool          IsSystemresetSignaled(const context_t context_id);

    context_t           DeduceContextId(const gua_context_desc_t context_desc);
    view_t              DeduceViewId(const gua_context_desc_t context_desc, const gua_view_desc_t view_desc);
    model_t             DeduceModelId(const gua_model_desc_t& model_desc);

    const bool          IsModelPresent(const gua_model_desc_t model_desc);
    const context_t     NumContextsRegistered();

    void                Dispatch(const context_t context_id, scm::gl::render_device_ptr device);
    const bool          IsCutUpdateInProgress(const context_t context_id);

    scm::gl::buffer_ptr GetContextbuffer(const context_t context_id, scm::gl::render_device_ptr device);
    scm::gl::vertex_array_ptr GetContextMemory(const context_t context_id, scm::gl::render_device_ptr device);


protected:
                        Controller();
    static bool         is_instanced_;
    static Controller*  single_;

private:
    static std::mutex   mutex_;

    std::unordered_map<context_t, CutUpdatePool*> cut_update_pools_;
    std::unordered_map<context_t, GpuContext*> gpu_contexts_;

    std::unordered_map<gua_context_desc_t, context_t> context_map_;
    context_t num_contexts_registered_;

    std::unordered_map<context_t, std::unordered_map<gua_view_desc_t, view_t>> view_map_;
    std::vector<view_t> num_views_registered_;

    std::unordered_map<gua_model_desc_t, model_t> model_map_;
    model_t             num_models_registered_;

    std::unordered_map<context_t, std::queue<bool>> reset_flags_history_;

};


} } // namespace lamure


#endif // REN_CONTROLLER_H_
