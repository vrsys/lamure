// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef LAMURE_LOD_POLICY_H_
#define LAMURE_LOD_POLICY_H_

#include <lamure/platform_lod.h>
#include <lamure/types.h>
#include <lamure/util/memory.h>
#include <lamure/lod/config.h>

#include <mutex>

namespace lamure {
namespace lod {

class LOD_DLL policy
{
public:
                        policy(const policy&) = delete;
                        policy& operator=(const policy&) = delete;
    virtual             ~policy();

    static policy*      get_instance();

    void                set_reset_system(const bool reset_system) { reset_system_ = reset_system; };
    void                set_max_upload_budget_in_mb(const size_t max_upload_budget) { max_upload_budget_in_mb_ = max_upload_budget; };
    void                set_render_budget_in_mb(const size_t render_budget) { render_budget_in_mb_ = render_budget; };
    void                set_out_of_core_budget_in_mb(const size_t out_of_core_budget) { out_of_core_budget_in_mb_ = out_of_core_budget; };

    const bool          reset_system() const { return reset_system_; };
    const size_t        max_upload_budget_in_mb() const { return max_upload_budget_in_mb_; };
    const size_t        render_budget_in_mb() const { return render_budget_in_mb_; };
    const size_t        out_of_core_budget_in_mb() const { return out_of_core_budget_in_mb_; };

    const int32_t       window_width() const { return window_width_; };
    const int32_t       window_height() const { return window_height_; };
    void                set_window_width(const int32_t window_width) { window_width_ = window_width; };
    void                set_window_height(const int32_t window_height) { window_height_ = window_height; };

protected:

                        policy();
    static bool         is_instanced_;
    static policy*      single_;

private:
    static std::mutex   mutex_;

    bool                reset_system_;

    size_t              max_upload_budget_in_mb_;
    size_t              render_budget_in_mb_;
    size_t              out_of_core_budget_in_mb_;

    int32_t             window_width_;
    int32_t             window_height_;

};


} } // namespace lamure

#endif // LAMURE_LOD_POLICY_H_
