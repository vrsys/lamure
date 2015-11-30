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

std::mutex Cutdatabase::mutex_;
bool Cutdatabase::is_instanced_ = false;
Cutdatabase* Cutdatabase::single_ = nullptr;

Cutdatabase::
Cutdatabase() {

}

Cutdatabase::
~Cutdatabase() {
    std::lock_guard<std::mutex> lock(mutex_);

    is_instanced_ = false;

    for (auto& record_it : records_) {
        CutdatabaseRecord* record = record_it.second;
        if (record != nullptr) {
            delete record;
            record = nullptr;
        }
    }

    records_.clear();
}

Cutdatabase* Cutdatabase::
get_instance() {
    if (!is_instanced_) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!is_instanced_) {
            single_ = new Cutdatabase();
            is_instanced_ = true;
        }

        return single_;
    }
    else {
        return single_;
    }
}

void Cutdatabase::
reset() {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& record_it : records_) {
        CutdatabaseRecord* record = record_it.second;
        if (record != nullptr) {
            delete record;
            record = nullptr;
        }
    }

    records_.clear();
}

void Cutdatabase::
expand(const context_t context_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    //critical section: check for missing entry again
    //to prevent double initialization

    auto it = records_.find(context_id);

    if (it == records_.end()) {
        records_[context_id] = new CutdatabaseRecord(context_id);
    }
}

void Cutdatabase::
Swap(const context_t context_id) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        it->second->Swapfront();
    }
    else {
        expand(context_id);
        Swap(context_id);
    }
}

void Cutdatabase::
SendCamera(context_t const context_id, view_t const view_id, const Camera& camera) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        it->second->SetCamera(view_id, camera);
    }
    else {
        expand(context_id);
        SendCamera(context_id, view_id, camera);
    }
}

void Cutdatabase::
SendheightDividedByTopMinusBottom(context_t const context_id, view_t const view_id, const float& height_divided_by_top_minus_bottom) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        it->second->SetheightDividedByTopMinusBottom(view_id, height_divided_by_top_minus_bottom);
    }
    else {
        expand(context_id);
        SendheightDividedByTopMinusBottom(context_id, view_id, height_divided_by_top_minus_bottom);
    }
}

void Cutdatabase::
SendTransform(context_t const context_id, model_t const model_id, const scm::math::mat4f& transform) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        it->second->SetTransform(model_id, transform);
    }
    else {
        expand(context_id);
        SendTransform(context_id, model_id, transform);
    }
}

void Cutdatabase::
Sendrendered(const context_t context_id, const model_t model_id) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        it->second->Setrendered(model_id);
    }
    else {
        expand(context_id);
        Sendrendered(context_id, model_id);
    }
}

void Cutdatabase::
SendThreshold(context_t const context_id, model_t const model_id, const float threshold) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        it->second->SetThreshold(model_id, threshold);
    }
    else {
        expand(context_id);
        SendThreshold(context_id, model_id, threshold);
    }
}


void Cutdatabase::
ReceiveCameras(const context_t context_id, std::map<view_t, Camera>& cameras) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        it->second->ReceiveCameras(cameras);
    }
    else {
        expand(context_id);
        ReceiveCameras(context_id, cameras);
    }
}

void Cutdatabase::
ReceiveheightDividedByTopMinusBottoms(const context_t context_id, std::map<view_t, float>& height_divided_by_top_minus_bottom) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        it->second->ReceiveheightDividedByTopMinusBottoms(height_divided_by_top_minus_bottom);
    }
    else {
        expand(context_id);
        ReceiveheightDividedByTopMinusBottoms(context_id, height_divided_by_top_minus_bottom);
    }
}


void Cutdatabase::
ReceiveTransforms(const context_t context_id, std::map<model_t, scm::math::mat4f>& transforms) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        it->second->ReceiveTransforms(transforms);
    }
    else {
        expand(context_id);
        ReceiveTransforms(context_id, transforms);
    }

}

void Cutdatabase::
Receiverendered(const context_t context_id, std::set<model_t>& rendered) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        it->second->Receiverendered(rendered);
    }
    else {
        expand(context_id);
        Receiverendered(context_id, rendered);
    }
}

void Cutdatabase::
ReceiveThresholds(const context_t context_id, std::map<model_t, float>& thresholds) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        it->second->ReceiveThresholds(thresholds);
    }
    else {
        expand(context_id);
        ReceiveThresholds(context_id, thresholds);
    }

}

void Cutdatabase::
SetCut(const context_t context_id, const view_t view_id, const model_t model_id, Cut& cut) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        it->second->SetCut(view_id, model_id, cut);
    }
    else {
        expand(context_id);
        SetCut(context_id, view_id, model_id, cut);
    }
}

Cut& Cutdatabase::
GetCut(const context_t context_id, const view_t view_id, const model_t model_id) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        return it->second->GetCut(view_id, model_id);
    }
    else {
        expand(context_id);
        return GetCut(context_id, view_id, model_id);
    }
}

std::vector<CutdatabaseRecord::SlotUpdateDescr>& Cutdatabase::
GetUpdatedSet(const context_t context_id) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        return it->second->GetUpdatedSet();
    }
    else {
        expand(context_id);
        return GetUpdatedSet(context_id);
    }
}

void Cutdatabase::
SetUpdatedSet(const context_t context_id, std::vector<CutdatabaseRecord::SlotUpdateDescr>& updated_set) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        it->second->SetUpdatedSet(updated_set);
    }
    else {
        expand(context_id);
        SetUpdatedSet(context_id, updated_set);
    }
}

const bool Cutdatabase::
IsfrontModified(const context_t context_id) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        return it->second->IsfrontModified();
    }
    else {
        expand(context_id);
        return IsfrontModified(context_id);
    }
}

const bool Cutdatabase::
IsSwapRequired(const context_t context_id) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        return it->second->IsSwapRequired();
    }
    else {
        expand(context_id);
        return IsSwapRequired(context_id);
    }
}


void Cutdatabase::
SetIsfrontModified(const context_t context_id, const bool front_modified) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        it->second->SetIsfrontModified(front_modified);
    }
    else {
        expand(context_id);
        SetIsfrontModified(context_id, front_modified);
    }
}

void Cutdatabase::
SetIsSwapRequired(const context_t context_id, const bool swap_required) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        it->second->SetIsSwapRequired(swap_required);
    }
    else {
        expand(context_id);
        SetIsSwapRequired(context_id, swap_required);
    }
}

void Cutdatabase::
SignalUploadcomplete(const context_t context_id) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        it->second->SignalUploadcomplete();
    }
    else {
        expand(context_id);
        SignalUploadcomplete(context_id);
    }
}

const CutdatabaseRecord::Temporarybuffer Cutdatabase::
Getbuffer(const context_t context_id) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        return it->second->Getbuffer();
    }
    else {
        expand(context_id);
        return Getbuffer(context_id);
    }
}

void Cutdatabase::
Setbuffer(const context_t context_id, const CutdatabaseRecord::Temporarybuffer buffer) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        it->second->Setbuffer(buffer);
    }
    else {
        expand(context_id);
        Setbuffer(context_id, buffer);
    }
}

void Cutdatabase::
LockRecord(const context_t context_id) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        it->second->Lockfront();
    }
    else {
        expand(context_id);
        LockRecord(context_id);
    }
}

void Cutdatabase::
UnlockRecord(const context_t context_id) {
    auto it = records_.find(context_id);

    if (it != records_.end()) {
        it->second->Unlockfront();
    }
    else {
        expand(context_id);
        UnlockRecord(context_id);
    }

}


} // namespace ren

} // namespace lamure




