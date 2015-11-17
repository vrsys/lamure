// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_BASIC_ALGORITHMS_H_
#define PRE_BASIC_ALGORITHMS_H_

#include <lamure/pre/common.h>
#include <lamure/pre/surfel_disk_array.h>
#include <lamure/pre/surfel_mem_array.h>
#include <lamure/bounding_box.h>

namespace lamure {
namespace pre
{

/**
* A group of basic algorithms, which are used during tree construction.
*/
class BasicAlgorithms
{
public:
    
    struct SurfelGroupProperties {
        real        rep_radius;
        vec3r       centroid;
        BoundingBox bounding_box;
    };

    template <class T>
    using SplittedArray = std::vector<std::pair<T, BoundingBox>>;

                        BasicAlgorithms() = delete;

    static BoundingBox  ComputeAABB(const SurfelMemArray& sa, 
                                    const bool parallelize = true);

    static BoundingBox  ComputeAABB(const SurfelDiskArray& sa, 
                                    const size_t buffer_size,
                                    const bool parallelize = true);

    static void         TranslateSurfels(SurfelMemArray& sa,
                                         const vec3r& translation);

    static void         TranslateSurfels(SurfelDiskArray& sa,
                                         const vec3r& translation,
                                         const size_t buffer_size);

    static SurfelGroupProperties
                        ComputeProperties(const SurfelMemArray& sa,
                                          const RepRadiusAlgorithm rep_radius_algo);

    static void         SortAndSplit(SurfelMemArray& sa,
                                     SplittedArray<SurfelMemArray>& out,
                                     const BoundingBox& box,
                                     const uint8_t split_axis,
                                     const uint8_t fan_factor, 
                                     const bool parallelize = false);

    static void         SortAndSplit(SurfelDiskArray& sa,
                                     SplittedArray<SurfelDiskArray>& out,
                                     const BoundingBox& box,
                                     const uint8_t split_axis,
                                     const uint8_t fan_factor, 
                                     const size_t memory_limit);
private:

    template <class T>
    static void         SplitSurfelArray(T& sa,
                                         SplittedArray<T>& out,
                                         const BoundingBox& box,
                                         const uint8_t split_axis,
                                         const uint8_t fan_factor);

};


} } // namespace lamure

#endif // PRE_BASIC_ALGORITHMS_H_

