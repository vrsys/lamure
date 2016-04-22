// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/basic_algorithms.h>

#include <lamure/pre/io/file.h>
#include <lamure/pre/external_sort.h>
#include <lamure/math/bounding_box.h>

#if WIN32
  #include <ppl.h>
#else
  #include <parallel/algorithm>
#endif

#include <cstring>

namespace lamure {
namespace pre 
{

math::bounding_box_t basic_algorithms::
compute_aabb(const surfel_mem_array& sa,
            const bool parallelize)
{
    ASSERT(!sa.is_empty());
    ASSERT(sa.length() > 0);

    vec3r_t min = sa.read_surfel_ref(0).pos();
    vec3r_t max = min;

    const auto begin = sa.mem_data()->begin() + sa.offset();
    const auto end = sa.mem_data()->begin() + sa.offset() + sa.length();

    if (!parallelize) {
        for (auto s = begin; s != end; ++s) {
            if (s->pos()[0] < min[0]) min[0] = s->pos()[0];
            if (s->pos()[1] < min[1]) min[1] = s->pos()[1];
            if (s->pos()[2] < min[2]) min[2] = s->pos()[2];
            if (s->pos()[0] > max[0]) max[0] = s->pos()[0];
            if (s->pos()[1] > max[1]) max[1] = s->pos()[1];
            if (s->pos()[2] > max[2]) max[2] = s->pos()[2];
        }
    }
    else {
        // INFO: openMP 3.1 supports min/max reduction. Available in GCC 4.7
        //       or above. The code below doesn't require openMP 3.1.
        #pragma omp parallel sections
        {
            {
                for (auto s = begin; s != end; ++s)
                    if (s->pos()[0] < min[0])
                        min[0] = s->pos()[0]; }
            #pragma omp section
            {
                for (auto s = begin; s != end; ++s)
                    if (s->pos()[1] < min[1])
                        min[1] = s->pos()[1]; }
            #pragma omp section
            {
                for (auto s = begin; s != end; ++s)
                    if (s->pos()[2] < min[2])
                        min[2] = s->pos()[2]; }
            #pragma omp section
            {
                for (auto s = begin; s != end; ++s)
                    if (s->pos()[0] > max[0])
                        max[0] = s->pos()[0]; }
            #pragma omp section
            {
                for (auto s = begin; s != end; ++s)
                    if (s->pos()[1] > max[1])
                        max[1] = s->pos()[1]; }
            #pragma omp section
            {
                for (auto s = begin; s != end; ++s)
                    if (s->pos()[2] > max[2])
                        max[2] = s->pos()[2]; }
        }
    }
    return math::bounding_box_t(min, max);
}

math::bounding_box_t basic_algorithms::
compute_aabb(const surfel_disk_array& sa,
            const size_t buffer_size,
            const bool parallelize)
{
    ASSERT(!sa.is_empty());
    ASSERT(sa.length() > 0);

    vec3r_t min = vec3r_t(std::numeric_limits<real_t>::max(),
                      std::numeric_limits<real_t>::max(),
                      std::numeric_limits<real_t>::max());
    vec3r_t max = vec3r_t(std::numeric_limits<real_t>::lowest(),
                      std::numeric_limits<real_t>::lowest(),
                      std::numeric_limits<real_t>::lowest());

    const size_t surfels_in_buffer = buffer_size / sizeof(surfel);

    for (size_t i = 0; i < sa.length(); i += surfels_in_buffer) {

        const size_t offset = sa.offset() + i;
        const size_t len = (i + surfels_in_buffer > sa.length()) ?
            sa.length() - i :
            surfels_in_buffer;

        surfel_vector data(len);
        sa.file()->read(&data, 0, offset, len);

        if (!parallelize) {

            for (size_t s = 0; s < len; ++s) {
                if (data[s].pos()[0] < min[0]) min[0] = data[s].pos()[0];
                if (data[s].pos()[1] < min[1]) min[1] = data[s].pos()[1];
                if (data[s].pos()[2] < min[2]) min[2] = data[s].pos()[2];
                if (data[s].pos()[0] > max[0]) max[0] = data[s].pos()[0];
                if (data[s].pos()[1] > max[1]) max[1] = data[s].pos()[1];
                if (data[s].pos()[2] > max[2]) max[2] = data[s].pos()[2];
            }
        }
        else {
            #pragma omp parallel sections
            {
                {
                    for (size_t s = 0; s < len; ++s)
                        if (data[s].pos()[0] < min[0])
                            min[0] = data[s].pos()[0]; }
                #pragma omp section
                {
                    for (size_t s = 0; s < len; ++s)
                        if (data[s].pos()[1] < min[1])
                            min[1] = data[s].pos()[1]; }
                #pragma omp section
                {
                    for (size_t s = 0; s < len; ++s)
                        if (data[s].pos()[2] < min[2])
                            min[2] = data[s].pos()[2]; }
                #pragma omp section
                {
                    for (size_t s = 0; s < len; ++s)
                        if (data[s].pos()[0] > max[0])
                            max[0] = data[s].pos()[0]; }
                #pragma omp section
                {
                    for (size_t s = 0; s < len; ++s)
                        if (data[s].pos()[1] > max[1])
                            max[1] = data[s].pos()[1]; }
                #pragma omp section
                {
                    for (size_t s = 0; s < len; ++s)
                        if (data[s].pos()[2] > max[2])
                            max[2] = data[s].pos()[2]; }
            }
        }
    }
    return math::bounding_box_t(min, max);
}

void basic_algorithms::
translate_surfels(surfel_mem_array& sa,
                 const vec3r_t& translation)
{
    ASSERT(!sa.is_empty());
    ASSERT(sa.length() > 0);

    const auto begin = sa.mem_data()->begin() + sa.offset();
    const auto end = sa.mem_data()->begin() + sa.offset() + sa.length();

    for (auto s = begin; s != end; ++s) {
        s->pos() += translation;
    }
}

void basic_algorithms::
translate_surfels(surfel_disk_array& sa,
                 const vec3r_t& translation,
                 const size_t buffer_size)
{
    ASSERT(!sa.is_empty());
    ASSERT(sa.length() > 0);

    const size_t surfels_in_buffer = buffer_size / sizeof(surfel);

    for (size_t i = 0; i < sa.length(); i += surfels_in_buffer) {
        const size_t offset = sa.offset() + i;
        const size_t len = (i + surfels_in_buffer > sa.length()) ?
            sa.length() - i :
            surfels_in_buffer;

        surfel_vector data(len);
        sa.file()->read(&data, 0, offset, len);

        for (size_t s = 0; s < len; ++s) {
            data[s].pos() += translation;
        }
        sa.file()->write(&data, 0, offset, len);
    }
}

void basic_algorithms::
sort_and_split(surfel_mem_array& sa,
             splitted_array<surfel_mem_array>& out,
             const math::bounding_box_t& box,
             const uint8_t split_axis,
             const uint8_t fan_factor,
             const bool parallelize)
{
    ASSERT(!sa.is_empty());
    ASSERT(sa.length() > 0);

    if (parallelize) {
#if WIN32
      // todo: find platform independent sort
      Concurrency::parallel_sort(sa.mem_data()->begin() + sa.offset(),
        sa.mem_data()->begin() + sa.offset() + sa.length(),
        surfel::compare(split_axis));
#else
      __gnu_parallel::sort(sa.mem_data()->begin() + sa.offset(),
        sa.mem_data()->begin() + sa.offset() + sa.length(),
        surfel::compare(split_axis));
#endif
    } else {
      std::sort(sa.mem_data()->begin() + sa.offset(),
        sa.mem_data()->begin() + sa.offset() + sa.length(),
        surfel::compare(split_axis));
    }

    split_surfel_array<surfel_mem_array>(sa, out, box, split_axis, fan_factor);
}

void basic_algorithms::
sort_and_split(surfel_disk_array& sa,
             splitted_array<surfel_disk_array>& out,
             const math::bounding_box_t& box,
             const uint8_t split_axis,
             const uint8_t fan_factor,
             const size_t memory_limit)
{
    external_sort::sort(sa, memory_limit, surfel::compare(split_axis));
    split_surfel_array<surfel_disk_array>(sa, out, box, split_axis, fan_factor);
}

template <class T>
void basic_algorithms::
split_surfel_array(T& sa,
                 splitted_array<T>& out,
                 const math::bounding_box_t& box,
                 const uint8_t split_axis,
                 const uint8_t fan_factor)
{
    using Traits = surfel_array_traits<T>;
    LAMURE_ASSERT(Traits::is_in_core || Traits::is_out_of_core, "Wrong type");

    const uint32_t child_size = (int)sa.length() / fan_factor;
    uint32_t remainder = sa.length() % fan_factor;

    for (uint32_t i = 0; i < fan_factor; ++i) {
        uint32_t child_first;
        if (i == 0)
            child_first = sa.offset();
        else
            child_first = out[i-1].first.length()+out[i-1].first.offset();

        uint32_t child_last = child_first+child_size;
        if (remainder > 0) {
            ++child_last;
            --remainder;
        }

        auto child_array = T(sa, child_first, child_last - child_first);
        out.push_back(std::make_pair(child_array, math::bounding_box_t()));
    }

    // compute bounding boxes

    std::vector<real_t> splits;

    for (size_t i = 0; i < out.size() - 1; ++i) {
        real_t p0 = out[i].first.read_surfel(out[i].first.length() - 1).pos()[split_axis];
        real_t p1 = out[i + 1].first.read_surfel(0).pos()[split_axis];

        splits.push_back((p1 - p0) / 2.0 + p0);
    }

    for (size_t i = 0; i < out.size(); ++i) {
        vec3r_t child_max = box.max();
        vec3r_t child_min = box.min();

        if (i == 0) {
            child_max[split_axis] = splits[0];
        }
        else if (i == out.size() - 1) {
            child_min[split_axis] = splits[splits.size() - 1];
        }
        else {
            child_min[split_axis] = splits[i - 1];
            child_max[split_axis] = splits[i];
        }

        out[i].second = math::bounding_box_t(child_min, child_max);
    }
}

basic_algorithms::surfel_group_properties basic_algorithms::
compute_properties(const surfel_mem_array& sa,
                   const rep_radius_algorithm rep_radius_algo,
                   bool use_radii_for_node_expansion)
{
    ASSERT(!sa.is_empty());
    ASSERT(rep_radius_algo == rep_radius_algorithm::arithmetic_mean ||
           rep_radius_algo == rep_radius_algorithm::geometric_mean ||
           rep_radius_algo == rep_radius_algorithm::harmonic_mean);

    surfel_group_properties props = {0.0, vec3r_t(0.0), math::bounding_box_t()};

//    if (rep_radius_algo == rep_radius_algorithm::geometric_mean)
//        props.rep_radius = 1.0;

    size_t counter = 0;

    for (size_t i = 0; i < sa.length(); ++i) {
        surfel s = sa.read_surfel_ref(i);
        
        props.bbox.expand_by_disk(s.pos(), s.normal(), s.radius());

        if (s.radius() <= 0.0)
            continue;

        switch (rep_radius_algo) {
            case rep_radius_algorithm::arithmetic_mean: props.rep_radius += s.radius(); break;
            case rep_radius_algorithm::geometric_mean:  props.rep_radius += log(s.radius()); break;
            case rep_radius_algorithm::harmonic_mean:   props.rep_radius += 1.0 / s.radius(); break;
        }

        props.centroid += s.pos();
        ++counter;
    }

    if (counter > 0) {

        switch (rep_radius_algo) {
            case rep_radius_algorithm::arithmetic_mean: props.rep_radius /= static_cast<real_t>(counter); break;
            case rep_radius_algorithm::geometric_mean:  props.rep_radius = exp(props.rep_radius / static_cast<real_t>(counter)); break;
            case rep_radius_algorithm::harmonic_mean:   props.rep_radius = static_cast<real_t>(counter) / props.rep_radius; break;
        }

        props.centroid /= static_cast<real_t>(counter);

    }

    return props;
}

}} // namespace lamure

