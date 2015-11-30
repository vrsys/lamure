// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/ren/model_database.h>
#include <lamure/ren/controller.h>

namespace lamure
{

namespace ren
{

std::mutex Modeldatabase::mutex_;
bool Modeldatabase::is_instanced_ = false;
Modeldatabase* Modeldatabase::single_ = nullptr;

Modeldatabase::
Modeldatabase()
: num_models_(0),
  num_models_pending_(0),
  surfels_per_node_(0),
  surfels_per_node_pending_(0),
  size_of_surfel_(0) {

}

Modeldatabase::
~Modeldatabase() {
    std::lock_guard<std::mutex> lock(mutex_);

    is_instanced_ = false;

    for (const auto& model_it : models_) {
        lod_point_cloud* model = model_it.second;
        if (model != nullptr) {
            delete model;
            model = nullptr;
        }
    }

    models_.clear();
}

Modeldatabase* Modeldatabase::
get_instance() {
    if (!is_instanced_) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!is_instanced_) { //double-checked locking
            single_ = new Modeldatabase();
            is_instanced_ = true;
        }

        return single_;
    }
    else {
        return single_;
    }
}

void Modeldatabase::
Apply() {
    std::lock_guard<std::mutex> lock(mutex_);

    num_models_ = num_models_pending_;
    surfels_per_node_ = surfels_per_node_pending_;
}

const model_t Modeldatabase::
AddModel(const std::string& filepath, const std::string& model_key) {

    if (Controller::get_instance()->IsModelPresent(model_key)) {
        return Controller::get_instance()->DeduceModelId(model_key);
    }

    lod_point_cloud* model = new lod_point_cloud(filepath);

    if (model->is_loaded()) {
        const bvh* bvh = model->get_bvh();

        if (num_models_ == 0) {
            std::lock_guard<std::mutex> lock(mutex_);
            if (num_models_ == 0) {
                size_of_surfel_ = bvh->size_of_surfel();
                surfels_per_node_pending_ = bvh->surfels_per_node();
                surfels_per_node_ = surfels_per_node_pending_;
            }
        }
        else {
            if (size_of_surfel_ != bvh->size_of_surfel()) {
                throw std::runtime_error(
                    "PLOD: Modeldatabase::Incompatible surfel size");
            }
        }

        if (bvh->surfels_per_node() > surfels_per_node_pending_) {
            std::lock_guard<std::mutex> lock(mutex_);
            if (bvh->surfels_per_node() > surfels_per_node_pending_) {
                surfels_per_node_pending_ = bvh->surfels_per_node();
            }
        }

        model_t model_id = 0;

        {
            std::lock_guard<std::mutex> lock(mutex_);

            model_id = Controller::get_instance()->DeduceModelId(model_key);

            model->model_id_ = model_id;
            models_[model_id] = model;

            ++num_models_pending_;

            num_models_ = num_models_pending_;
            surfels_per_node_ = surfels_per_node_pending_;

            Controller::get_instance()->SignalSystemreset();
        }

#ifdef LAMURE_ENABLE_INFO
        std::cout << "PLOD: model " << model_id << ": " << filepath << std::endl;
#endif
        return model_id;

    }
    else {
        throw std::runtime_error(
            "PLOD: Modeldatabase::Model was not loaded");
    }

    return invalid_model_t;

}

lod_point_cloud* Modeldatabase::
GetModel(const model_t model_id) {
    if (models_.find(model_id) != models_.end()) {
        return models_[model_id];
    }

    throw std::runtime_error(
        "PLOD: Modeldatabase::Model was not found:" + model_id);

    return nullptr;
}


} // namespace ren

} // namespace lamure


