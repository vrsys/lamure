// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_BUILDER_H_
#define PRE_BUILDER_H_

#include <string>

#include <lamure/pre/platform.h>
#include <lamure/pre/common.h>

#include <boost/filesystem.hpp>

#include <lamure/pre/logger.h>

namespace lamure
{
namespace pre
{

class reduction_strategy;
class radius_computation_strategy;
class normal_computation_strategy;

class PREPROCESSING_DLL builder
{
public:

    struct descriptor
    {
        std::string input_file;
        std::string working_directory;
        std::string prov_file;
        uint32_t max_fan_factor;
        size_t surfels_per_node;
        uint16_t final_stage;
        bool compute_normals_and_radii;
        bool keep_intermediate_files;
        bool resample;
        float memory_budget;
        float radius_multiplier;
        size_t buffer_size;
        uint16_t number_of_neighbours;
        bool translate_to_origin;
        uint16_t number_of_outlier_neighbours;
        float outlier_ratio;

        rep_radius_algorithm rep_radius_algo;
        reduction_algorithm reduction_algo;
        radius_computation_algorithm radius_computation_algo;
        normal_computation_algorithm normal_computation_algo;
    };

    explicit builder(const descriptor &desc);

    virtual             ~builder();
    builder(const builder &other) = delete;
    builder &operator=(const builder &other) = delete;

    bool construct();
    bool resample();

private:
    reduction_strategy *get_reduction_strategy(reduction_algorithm algo) const;
    radius_computation_strategy *get_radius_strategy(radius_computation_algorithm algo) const;
    normal_computation_strategy *get_normal_strategy(normal_computation_algorithm algo) const;
    boost::filesystem::path convert_to_binary(std::string const& input_filename, std::string const &input_type) const;
    boost::filesystem::path downsweep(boost::filesystem::path input_file, uint16_t start_stage) const;
    boost::filesystem::path upsweep(boost::filesystem::path input_file,
                                    uint16_t start_stage,
                                    reduction_strategy const *reduction_strategy,
                                    normal_computation_strategy const *normal_comp_strategy,
                                    radius_computation_strategy const *radius_comp_strategy) const;
    bool resample_surfels(boost::filesystem::path const &input_file) const;
    bool reserialize(boost::filesystem::path const &input_file, uint16_t start_stage) const;

    size_t calculate_memory_limit() const;

    descriptor desc_;
    size_t memory_limit_;
    boost::filesystem::path base_path_;
};

} // namespace pre
} // namespace lamure

#endif // PRE_BUILDER_H_
