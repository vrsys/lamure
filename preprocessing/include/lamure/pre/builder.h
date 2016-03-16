// Copyright (c) 2014 Bauhaus-Universitaet Weimar
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

namespace lamure {
namespace pre {

class PREPROCESSING_DLL builder
{
public:

    struct descriptor {
        std::string     input_file;
        std::string     working_directory;
        uint32_t        max_fan_factor;
        size_t          surfels_per_node;
        uint16_t        final_stage;
        bool            compute_normals_and_radii;
        bool            keep_intermediate_files;
        bool            resample;
        float           memory_ratio;
        size_t          buffer_size;
        uint16_t        number_of_neighbours;
        bool            translate_to_origin;
        uint16_t        number_of_outlier_neighbours;
        float           outlier_ratio;

        rep_radius_algorithm          rep_radius_algo;
        reduction_algorithm           reduction_algo;
        radius_computation_algorithm  radius_computation_algo;
        normal_computation_algorithm  normal_computation_algo;
    };

    explicit            builder(const descriptor& desc);

    virtual             ~builder();
                        builder(const builder& other) = delete;
    builder&            operator=(const builder& other) = delete;

    bool                construct();

private:
    descriptor           desc_;
    size_t               memory_limit_;
    boost::filesystem::path base_path_;
};


} // namespace pre
} // namespace lamure

#endif // PRE_BUILDER_H_
