// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef REN_LAMURE_POLICY_H_
#define REN_LAMURE_POLICY_H_

#include <mutex>

#include <lamure/ren/platform.h>
#include <lamure/utils.h>
#include <lamure/types.h>
#include <lamure/memory.h>
#include <lamure/config.h>

namespace lamure {
namespace ren {

class RENDERING_DLL Policy
{
public:
                        Policy(const Policy&) = delete;
                        Policy& operator=(const Policy&) = delete;
    virtual             ~Policy();

    static Policy*      get_instance();

    void                set_reset_system(const bool reset_system) { reset_system_ = reset_system; };
    void                set_max_upload_budget_in_mb(const size_t max_upload_budget) { max_upload_budget_in_mb_ = max_upload_budget; };
    void                set_render_budget_in_mb(const size_t render_budget) { render_budget_in_mb_ = render_budget; };
    void                set_out_of_core_budget_in_mb(const size_t out_of_core_budget) { out_of_core_budget_in_mb_ = out_of_core_budget; };

    const bool          reset_system() const { return reset_system_; };
    const size_t        max_upload_budget_in_mb() const { return max_upload_budget_in_mb_; };
    const size_t        render_budget_in_mb() const { return render_budget_in_mb_; };
    const size_t        out_of_core_budget_in_mb() const { return out_of_core_budget_in_mb_; };

protected:

                        Policy();
    static bool         is_instanced_;
    static Policy*      single_;

private:
    /* data */

    static std::mutex   mutex_;

    bool                reset_system_;

    size_t              max_upload_budget_in_mb_;
    size_t              render_budget_in_mb_;
    size_t              out_of_core_budget_in_mb_;

};


} } // namespace lamure

#endif // REN_LAMURE_POLICY_H_
