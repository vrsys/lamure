// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef LAMURE_LOD_MODEL_DATABASE_H_
#define LAMURE_LOD_MODEL_DATABASE_H_

#include <lamure/types.h>
#include <lamure/lod/dataset.h>
#include <lamure/lod/config.h>
#include <lamure/platform_lod.h>

#include <unordered_map>
#include <mutex>

namespace lamure {
namespace lod {

class LOD_DLL model_database
{
public:

                        model_database(const model_database&) = delete;
                        model_database& operator=(const model_database&) = delete;
    virtual             ~model_database();

    static model_database* get_instance();

    const model_t       add_model(const std::string& filepath, const std::string& model_key);
    dataset*            get_model(const model_t model_id);
    void                apply();

    const model_t       num_models() const { return num_datasets_; };

    const size_t        get_primitive_size(const bvh::primitive_type type) const;
    const size_t        get_node_size(const model_t model_id) const;

    const size_t        get_slot_size() const;
    const size_t        get_primitives_per_node() const;
protected:

                        model_database();
    static bool         is_instanced_;
    static model_database* single_;

private:
    static std::mutex   mutex_;

    std::unordered_map<model_t, dataset*> datasets_;

    model_t             num_datasets_;
    model_t             num_datasets_pending_;
    size_t              primitives_per_node_;
    size_t              primitives_per_node_pending_;


};


} } // namespace lamure


#endif // LAMURE_LOD_MODEL_DATABASE_H_
