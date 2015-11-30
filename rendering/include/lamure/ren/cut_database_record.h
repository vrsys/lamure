// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef REN_CUT_DATABASE_RECORD_H_
#define REN_CUT_DATABASE_RECORD_H_

#include <unordered_map>
#include <lamure/utils.h>
#include <lamure/types.h>
#include <mutex>
#include <lamure/ren/cut.h>
#include <lamure/ren/camera.h>
#include <lamure/ren/config.h>

namespace lamure {
namespace ren {

class CutdatabaseRecord
{
public:

    enum Temporarybuffer
    {
        BUFFER_A = 0,
        BUFFER_B = 1,
        BUFFER_COUNT = 2
    };

    enum Recordfront
    {
        FRONT_A = 0,
        FRONT_B = 1,
        FRONT_COUNT = 2
    };

    struct SlotUpdateDescr
    {
        explicit SlotUpdateDescr(
            const slot_t src,
            const slot_t dst)
            : src_(src), dst_(dst) {};

        slot_t src_;
        slot_t dst_;
    };

                        CutdatabaseRecord(const context_t context_id);
                        CutdatabaseRecord(const CutdatabaseRecord&) = delete;
                        CutdatabaseRecord& operator=(const CutdatabaseRecord&) = delete;
    virtual             ~CutdatabaseRecord();

    void                SetCut(const view_t view_id, const model_t model_id, Cut& cut);
    Cut&                GetCut(const view_t view_id, const model_t model_id);

    std::vector<SlotUpdateDescr>& GetUpdatedSet();
    void                SetUpdatedSet(std::vector<SlotUpdateDescr>& updated_set);

    const bool          IsfrontModified() const;
    const bool          IsSwapRequired() const;
    void                SetIsfrontModified(const bool front_modified);
    void                SetIsSwapRequired(const bool swap_required);
    void                SignalUploadcomplete();

    const Temporarybuffer Getbuffer() const;
    void                Setbuffer(const Temporarybuffer buffer);

    void                Swapfront();

    void                Lockfront();
    void                Unlockfront();

    void                SetCamera(const view_t view_id, const Camera& camera);
    void                SetheightDividedByTopMinusBottom(const view_t view_id, float const height_divided_by_top_minus_bottom);
    void                SetTransform(const model_t model_id, const scm::math::mat4f& transform);
    void                Setrendered(const model_t model_id);
    void                SetThreshold(const model_t model_id, const float threshold);

    void                ReceiveCameras(std::map<view_t, Camera>& cameras);
    void                ReceiveheightDividedByTopMinusBottoms(std::map<view_t, float>& height_divided_by_top_minus_bottoms);
    void                ReceiveTransforms(std::map<model_t, scm::math::mat4f>& transforms);
    void                Receiverendered(std::set<model_t>& rendered);
    void                ReceiveThresholds(std::map<model_t, float>& thresholds);

protected:

    void                expandfrontA(const view_t view_id, const model_t model_id);
    void                expandfrontB(const view_t view_id, const model_t model_id);

private:
    /* data */
    std::mutex          mutex_;

    context_t           context_id_;

    bool                is_swap_required_;

    Recordfront         current_front_;
    
    //dim: [model_id][view_id]
    std::vector<std::vector<Cut>> front_a_cuts_;
    std::vector<std::vector<Cut>> front_b_cuts_;

    bool                front_a_is_modified_;
    bool                front_b_is_modified_;

    Temporarybuffer     front_a_buffer_;
    Temporarybuffer     front_b_buffer_;

    std::vector<SlotUpdateDescr> front_a_updated_set_;
    std::vector<SlotUpdateDescr> front_b_updated_set_;

    std::map<view_t, Camera> front_a_cameras_;
    std::map<view_t, Camera> front_b_cameras_;

    std::map<view_t, float> front_a_height_divided_by_top_minus_bottom_;
    std::map<view_t, float> front_b_height_divided_by_top_minus_bottom_;

    std::map<model_t, scm::math::mat4f> front_a_transforms_;
    std::map<model_t, scm::math::mat4f> front_b_transforms_;

    std::set<model_t> front_a_rendered_;
    std::set<model_t> front_b_rendered_;

    std::map<model_t, float> front_a_thresholds_;
    std::map<model_t, float> front_b_thresholds_;
};


} } // namespace lamure


#endif // REN_CUT_DATABASE_RECORD_H_
