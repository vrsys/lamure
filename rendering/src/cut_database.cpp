// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/ren/cut_database.h>


namespace lamure
{

namespace ren
{

std::mutex CutDatabase::mutex_;
bool CutDatabase::is_instanced_ = false;
CutDatabase* CutDatabase::single_ = nullptr;

CutDatabase::
CutDatabase() {

}

CutDatabase::
~CutDatabase() {
    std::lock_guard<std::mutex> lock(mutex_);

    is_instanced_ = false;

    for (auto& record_it : records_) {
        CutDatabaseRecord* record = record_it.second;
        if (record != nullptr) {
            delete record;
            record = nullptr;
        }
    }

    records_.clear();
}

CutDatabase* CutDatabase::
GetInstance() {
    if (!is_instanced_) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!is_instanced_) {
            single_ = new CutDatabase();
            is_instanced_ = true;
        }

        return single_;
    }
    else {
        return single_;
    }
}

void CutDatabase::
Reset() {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& record_it : records_) {
        CutDatabaseRecord* record = record_it.second;
        if (record != nullptr) {
            delete record;
            record = nullptr;
        }
    }

    records_.clear();
}

void CutDatabase::
Expand(const context_t context_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    //critical section: check for missing entry again
    //to prevent double initialization

    auto it = records_.find(context_id);

    if (it == records_.end()) {
        records_[context_id] = new CutDatabaseRecord(context_id);
    }
}

void CutDatabase::
Swap(const context_t context_id) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        it->second->SwapFront();
    }
    else {
        Expand(context_id);
        Swap(context_id);
    }
}

void CutDatabase::
SendCamera(context_t const context_id, view_t const view_id, const Camera& camera) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        it->second->SetCamera(view_id, camera);
    }
    else {
        Expand(context_id);
        SendCamera(context_id, view_id, camera);
    }
}

void CutDatabase::
SendHeightDividedByTopMinusBottom(context_t const context_id, view_t const view_id, const float& height_divided_by_top_minus_bottom) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        it->second->SetHeightDividedByTopMinusBottom(view_id, height_divided_by_top_minus_bottom);
    }
    else {
        Expand(context_id);
        SendHeightDividedByTopMinusBottom(context_id, view_id, height_divided_by_top_minus_bottom);
    }
}

void CutDatabase::
SendTransform(context_t const context_id, model_t const model_id, const scm::math::mat4f& transform) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        it->second->SetTransform(model_id, transform);
    }
    else {
        Expand(context_id);
        SendTransform(context_id, model_id, transform);
    }
}

void CutDatabase::
SendRendered(const context_t context_id, const model_t model_id) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        it->second->SetRendered(model_id);
    }
    else {
        Expand(context_id);
        SendRendered(context_id, model_id);
    }
}

void CutDatabase::
SendThreshold(context_t const context_id, model_t const model_id, const float threshold) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        it->second->SetThreshold(model_id, threshold);
    }
    else {
        Expand(context_id);
        SendThreshold(context_id, model_id, threshold);
    }
}


void CutDatabase::
ReceiveCameras(const context_t context_id, std::map<view_t, Camera>& cameras) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        it->second->ReceiveCameras(cameras);
    }
    else {
        Expand(context_id);
        ReceiveCameras(context_id, cameras);
    }
}

void CutDatabase::
ReceiveHeightDividedByTopMinusBottoms(const context_t context_id, std::map<view_t, float>& height_divided_by_top_minus_bottom) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        it->second->ReceiveHeightDividedByTopMinusBottoms(height_divided_by_top_minus_bottom);
    }
    else {
        Expand(context_id);
        ReceiveHeightDividedByTopMinusBottoms(context_id, height_divided_by_top_minus_bottom);
    }
}


void CutDatabase::
ReceiveTransforms(const context_t context_id, std::map<model_t, scm::math::mat4f>& transforms) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        it->second->ReceiveTransforms(transforms);
    }
    else {
        Expand(context_id);
        ReceiveTransforms(context_id, transforms);
    }

}

void CutDatabase::
ReceiveRendered(const context_t context_id, std::set<model_t>& rendered) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        it->second->ReceiveRendered(rendered);
    }
    else {
        Expand(context_id);
        ReceiveRendered(context_id, rendered);
    }
}

void CutDatabase::
ReceiveThresholds(const context_t context_id, std::map<model_t, float>& thresholds) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        it->second->ReceiveThresholds(thresholds);
    }
    else {
        Expand(context_id);
        ReceiveThresholds(context_id, thresholds);
    }

}

void CutDatabase::
SetCut(const context_t context_id, const view_t view_id, const model_t model_id, Cut& cut) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        it->second->SetCut(view_id, model_id, cut);
    }
    else {
        Expand(context_id);
        SetCut(context_id, view_id, model_id, cut);
    }
}

Cut& CutDatabase::
GetCut(const context_t context_id, const view_t view_id, const model_t model_id) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        return it->second->GetCut(view_id, model_id);
    }
    else {
        Expand(context_id);
        return GetCut(context_id, view_id, model_id);
    }
}

std::vector<CutDatabaseRecord::SlotUpdateDescr>& CutDatabase::
GetUpdatedSet(const context_t context_id) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        return it->second->GetUpdatedSet();
    }
    else {
        Expand(context_id);
        return GetUpdatedSet(context_id);
    }
}

void CutDatabase::
SetUpdatedSet(const context_t context_id, std::vector<CutDatabaseRecord::SlotUpdateDescr>& updated_set) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        it->second->SetUpdatedSet(updated_set);
    }
    else {
        Expand(context_id);
        SetUpdatedSet(context_id, updated_set);
    }
}

const bool CutDatabase::
IsFrontModified(const context_t context_id) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        return it->second->IsFrontModified();
    }
    else {
        Expand(context_id);
        return IsFrontModified(context_id);
    }
}

const bool CutDatabase::
IsSwapRequired(const context_t context_id) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        return it->second->IsSwapRequired();
    }
    else {
        Expand(context_id);
        return IsSwapRequired(context_id);
    }
}


void CutDatabase::
SetIsFrontModified(const context_t context_id, const bool front_modified) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        it->second->SetIsFrontModified(front_modified);
    }
    else {
        Expand(context_id);
        SetIsFrontModified(context_id, front_modified);
    }
}

void CutDatabase::
SetIsSwapRequired(const context_t context_id, const bool swap_required) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        it->second->SetIsSwapRequired(swap_required);
    }
    else {
        Expand(context_id);
        SetIsSwapRequired(context_id, swap_required);
    }
}

void CutDatabase::
SignalUploadComplete(const context_t context_id) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        it->second->SignalUploadComplete();
    }
    else {
        Expand(context_id);
        SignalUploadComplete(context_id);
    }
}

const CutDatabaseRecord::TemporaryBuffer CutDatabase::
GetBuffer(const context_t context_id) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        return it->second->GetBuffer();
    }
    else {
        Expand(context_id);
        return GetBuffer(context_id);
    }
}

void CutDatabase::
SetBuffer(const context_t context_id, const CutDatabaseRecord::TemporaryBuffer buffer) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        it->second->SetBuffer(buffer);
    }
    else {
        Expand(context_id);
        SetBuffer(context_id, buffer);
    }
}

void CutDatabase::
LockRecord(const context_t context_id) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        it->second->LockFront();
    }
    else {
        Expand(context_id);
        LockRecord(context_id);
    }
}

void CutDatabase::
UnlockRecord(const context_t context_id) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        it->second->UnlockFront();
    }
    else {
        Expand(context_id);
        UnlockRecord(context_id);
    }

}


} // namespace ren

} // namespace lamure




