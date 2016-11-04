// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef REN_CUT_DATABASE_H_
#define REN_CUT_DATABASE_H_

#include <map>
#include <lamure/utils.h>
#include <lamure/types.h>
#include <mutex>

#include <lamure/ren/platform.h>
#include <lamure/ren/cut.h>
#include <lamure/ren/cut_database_record.h>

namespace lamure {
namespace ren {

class CutUpdatePool;

class RENDERING_DLL Cutdatabase
{
public:

                        Cutdatabase(const Cutdatabase&) = delete;
                        Cutdatabase& operator=(const Cutdatabase&) = delete;
    virtual             ~Cutdatabase();

    static Cutdatabase* get_instance();

    void                reset();

    Cut&                GetCut(const context_t context_id, const view_t view_id, const model_t model_id);
    std::vector<CutdatabaseRecord::SlotUpdateDescr>& GetUpdatedSet(const context_t context_id);

    const bool          IsfrontModified(const context_t context_id);
    void                SetIsfrontModified(const context_t context_id, const bool front_modified);
    const bool          IsSwapRequired(const context_t context_id);
    void                SignalUploadcomplete(const context_t context_id);

    const CutdatabaseRecord::Temporarybuffer Getbuffer(const context_t context_id);
    void                Swap(const context_t context_id);
    void                SendCamera(const context_t context_id, const view_t view_id, const Camera& camera);
    void                SendheightDividedByTopMinusBottom(context_t const context_id, view_t const view_id, const float& height_divided_by_top_minus_bottom);
    void                SendTransform(const context_t context_id, const model_t model_id, const scm::math::mat4f& transform);
    void                Sendrendered(const context_t context_id, const model_t model_id);
    void                SendThreshold(const context_t context_id, const model_t model_id, const float threshold);

protected:
                        Cutdatabase();
    static bool         is_instanced_;
    static Cutdatabase* single_;

    friend class        CutUpdatePool;

    void                expand(const context_t context_id);
    void                ReceiveCameras(const context_t context_id, std::map<view_t, Camera>& cameras);
    void                ReceiveheightDividedByTopMinusBottoms(const context_t context_id, std::map<view_t, float>& height_divided_by_top_minus_bottom);
    void                ReceiveTransforms(const context_t context_id, std::map<model_t, scm::math::mat4f>& transforms);
    void                Receiverendered(const context_t context_id, std::set<model_t>& rendered);
    void                ReceiveImportance(const context_t context_id, std::map<model_t, float>& importance);
    void                ReceiveThresholds(const context_t context_id, std::map<model_t, float>& thresholds);

    void                LockRecord(const context_t context_id);
    void                UnlockRecord(const context_t context_id);

    void                Setbuffer(const context_t context_id, const CutdatabaseRecord::Temporarybuffer buffer);

    void                SetIsSwapRequired(const context_t context_id, const bool front_modified);

    void                SetUpdatedSet(const context_t context_id, std::vector<CutdatabaseRecord::SlotUpdateDescr>& updated_set);

    void                SetCut(const context_t context_id, const view_t view_id, const model_t model_id, Cut& cut);

private:
    /* data */
    static std::mutex   mutex_;

    std::map<context_t, CutdatabaseRecord*> records_;

};


} } // namespace lamure


#endif // REN_CUT_DATABASE_H_
