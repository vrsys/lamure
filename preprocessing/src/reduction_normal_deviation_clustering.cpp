// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/reduction_normal_deviation_clustering.h>

#include <lamure/pre/basic_algorithms.h>
#include <lamure/utils.h>

#include <queue>

#if WIN32
#include <ppl.h>
#else
#include <parallel/algorithm>
#endif

namespace lamure
{
namespace pre
{
surfel reduction_normal_deviation_clustering::create_representative(const std::vector<surfel> &input)
{
    assert(input.size() > 0);

    if(input.size() == 1)
        return input.front();

    vec3r pos = vec3r(0);
    vec3f nml = vec3f(0);
    vec3f col = vec3f(0);
    real weight_sum = 0.f;
    for(const auto &surfel : input)
    {
        real weight = 1.0; // surfel.radius();
        weight_sum += weight;

        pos += weight * surfel.pos();
        nml += float(weight) * surfel.normal();
        col += surfel.color();
    }

    pos /= weight_sum;
    nml /= weight_sum;
    col /= input.size();

    nml = scm::math::normalize(nml);

    real radius = 0.0;

    for(const auto &surfel : input)
    {
        real dist = scm::math::distance(pos, surfel.pos());
        if(radius < dist + surfel.radius())
            radius = dist + surfel.radius();
    }

    return surfel(pos, vec3b((const uint8_t)col.x, (const uint8_t)col.y, (const uint8_t)col.z), radius, nml);
}

std::pair<vec3ui, vec3b> reduction_normal_deviation_clustering::compute_grid_dimensions(const std::vector<surfel_mem_array *> &input, const bounding_box &bounding_box,
                                                                                        const uint32_t surfels_per_node) const
{
    uint16_t max_axis_ratio = 1000;

    vec3r bb_dimensions = bounding_box.get_dimensions();

    // find axis relations
    // mark axes where every surfel has the same position as locked
    // sort bb_axes by size to make code readable
    vec3ui grid_dimensions;
    vec3b locked_grid_dimensions;
    locked_grid_dimensions[0] = false;
    locked_grid_dimensions[1] = false;
    locked_grid_dimensions[2] = false;

    std::vector<value_index_pair> sorted_bb_dimensions;
    sorted_bb_dimensions.push_back(std::make_pair(bb_dimensions[0], 0));
    sorted_bb_dimensions.push_back(std::make_pair(bb_dimensions[1], 1));
    sorted_bb_dimensions.push_back(std::make_pair(bb_dimensions[2], 2));

    std::sort(sorted_bb_dimensions.begin(), sorted_bb_dimensions.end());

    vec3ui sorted_grid_dimensions;
    vec3b sorted_locked_grid_dimensions;
    sorted_locked_grid_dimensions[0] = false;
    sorted_locked_grid_dimensions[1] = false;
    sorted_locked_grid_dimensions[2] = false;

    bool bb_zero_size = false;

    if((sorted_bb_dimensions[0].first == 0) && (sorted_bb_dimensions[1].first == 0) && (sorted_bb_dimensions[2].first == 0))
    {
        // 0 dimensions
        bb_zero_size = true;
    }
    else if((sorted_bb_dimensions[0].first == 0) && (sorted_bb_dimensions[1].first == 0))
    {
        // 1 dimension
        sorted_locked_grid_dimensions[0] = true;
        sorted_locked_grid_dimensions[1] = true;
        sorted_grid_dimensions = vec3ui(1, 1, 1);
    }
    else if(sorted_bb_dimensions[0].first == 0)
    {
        // 2 dimensions
        sorted_locked_grid_dimensions[0] = true;

        if(sorted_bb_dimensions[1].first < sorted_bb_dimensions[2].first)
        {
            sorted_grid_dimensions = vec3ui(1, 1, floor(sorted_bb_dimensions[2].first / sorted_bb_dimensions[1].first));
            if(sorted_grid_dimensions[2] > max_axis_ratio)
                sorted_grid_dimensions[2] = max_axis_ratio;
        }
        else
        {
            sorted_grid_dimensions = vec3ui(1, floor(sorted_bb_dimensions[1].first / sorted_bb_dimensions[2].first), 1);
            if(sorted_grid_dimensions[1] > max_axis_ratio)
                sorted_grid_dimensions[1] = max_axis_ratio;
        }
    }
    else
    {
        // 3 dimensions
        sorted_grid_dimensions = vec3ui(1, floor(sorted_bb_dimensions[1].first / sorted_bb_dimensions[0].first), floor(sorted_bb_dimensions[2].first / sorted_bb_dimensions[0].first));
        if(sorted_grid_dimensions[1] > max_axis_ratio)
            sorted_grid_dimensions[1] = max_axis_ratio;
        if(sorted_grid_dimensions[2] > max_axis_ratio)
            sorted_grid_dimensions[2] = max_axis_ratio;
    }

    if(!bb_zero_size)
    { // at least 1 dimesion

        // revert sorting
        grid_dimensions[sorted_bb_dimensions[0].second] = sorted_grid_dimensions[0];
        locked_grid_dimensions[sorted_bb_dimensions[0].second] = sorted_locked_grid_dimensions[0];

        grid_dimensions[sorted_bb_dimensions[1].second] = sorted_grid_dimensions[1];
        locked_grid_dimensions[sorted_bb_dimensions[1].second] = sorted_locked_grid_dimensions[1];

        grid_dimensions[sorted_bb_dimensions[2].second] = sorted_grid_dimensions[2];
        locked_grid_dimensions[sorted_bb_dimensions[2].second] = sorted_locked_grid_dimensions[2];

        // adapt total number of grid cells to number of surfels per node
        uint32_t total_grid_dimensions = (grid_dimensions[0] * grid_dimensions[1] * grid_dimensions[2]);

        if(total_grid_dimensions < surfels_per_node) // if less cells than surfels per node: increase
        {
            while(((grid_dimensions[0] + 1) * (grid_dimensions[1] + 1) * (grid_dimensions[2] + 1)) < surfels_per_node)
            {
                if(!locked_grid_dimensions[0])
                    grid_dimensions[0] = grid_dimensions[0] + 1;
                if(!locked_grid_dimensions[1])
                    grid_dimensions[1] = grid_dimensions[1] + 1;
                if(!locked_grid_dimensions[2])
                    grid_dimensions[2] = grid_dimensions[2] + 1;
                total_grid_dimensions = (grid_dimensions[0] * grid_dimensions[1] * grid_dimensions[2]);
            }
        }
        else if(total_grid_dimensions > surfels_per_node) // if more cells than surfels per node: decrease
        {
            while(total_grid_dimensions > surfels_per_node)
            {
                if(grid_dimensions[0] != 1)
                    grid_dimensions[0] = grid_dimensions[0] - 1;
                if(grid_dimensions[1] != 1)
                    grid_dimensions[1] = grid_dimensions[1] - 1;
                if(grid_dimensions[2] != 1)
                    grid_dimensions[2] = grid_dimensions[2] - 1;
                total_grid_dimensions = (grid_dimensions[0] * grid_dimensions[1] * grid_dimensions[2]);
            }
        }

        size_t termination_ctr = 0;

        // adapt occupied number of grid cells to number of surfels per node
        while(true)
        {
            // safety check
            if(termination_ctr++ >= 40000)
            {
                LOGGER_WARN("Reached maximum number of iterations in ndc while computing grid dimensions!");
                break;
            }

            // create grid
            std::vector<std::vector<std::vector<bool>>> grid(grid_dimensions[0], std::vector<std::vector<bool>>(grid_dimensions[1], std::vector<bool>(grid_dimensions[2])));

            for(uint32_t i = 0; i < grid_dimensions[0]; ++i)
            {
                for(uint32_t j = 0; j < grid_dimensions[1]; ++j)
                {
                    for(uint32_t k = 0; k < grid_dimensions[2]; ++k)
                    {
                        grid[i][j][k] = false;
                    }
                }
            }

            // check which cell a surfel occupies
            vec3r cell_size = vec3r(fabs(bb_dimensions[0] / grid_dimensions[0]), fabs(bb_dimensions[1] / grid_dimensions[1]), fabs(bb_dimensions[2] / grid_dimensions[2]));

            for(uint32_t i = 0; i < input.size(); ++i)
            {
                for(uint32_t j = 0; j < input[i]->length(); ++j)
                {
                    vec3r surfel_pos = input[i]->read_surfel_ref(j).pos() - bounding_box.min();
                    if(surfel_pos.x < 0.f)
                        surfel_pos.x = 0.f;
                    if(surfel_pos.y < 0.f)
                        surfel_pos.y = 0.f;
                    if(surfel_pos.z < 0.f)
                        surfel_pos.z = 0.f;

                    vec3ui index;

                    if(locked_grid_dimensions[0])
                    {
                        index[0] = 0;
                    }
                    else
                    {
                        index[0] = floor(surfel_pos[0] / cell_size[0]);
                    }

                    if(locked_grid_dimensions[1])
                    {
                        index[1] = 0;
                    }
                    else
                    {
                        index[1] = floor(surfel_pos[1] / cell_size[1]);
                    }

                    if(locked_grid_dimensions[2])
                    {
                        index[2] = 0;
                    }
                    else
                    {
                        index[2] = floor(surfel_pos[2] / cell_size[2]);
                    }

                    if((index[0] != 0) && (index[0] == grid_dimensions[0]))
                        index[0] = grid_dimensions[0] - 1;

                    if((index[1] != 0) && (index[1] == grid_dimensions[1]))
                        index[1] = grid_dimensions[1] - 1;

                    if((index[2] != 0) && (index[2] == grid_dimensions[2]))
                        index[2] = grid_dimensions[2] - 1;

                    grid[index[0]][index[1]][index[2]] = true;
                }
            }

            // count occupied cells
            uint32_t occupied_cells = 0;

            for(uint32_t i = 0; i < grid_dimensions[0]; ++i)
            {
                for(uint32_t j = 0; j < grid_dimensions[1]; ++j)
                {
                    for(uint32_t k = 0; k < grid_dimensions[2]; ++k)
                    {
                        if(grid[i][j][k])
                            ++occupied_cells;
                    }
                }
            }

            // check if finished
            if((occupied_cells > surfels_per_node) || (grid_dimensions[0] * grid_dimensions[1] * grid_dimensions[2] > 100000))
            {
                if(grid_dimensions[0] != 1)
                    grid_dimensions[0] = grid_dimensions[0] - 1;
                if(grid_dimensions[1] != 1)
                    grid_dimensions[1] = grid_dimensions[1] - 1;
                if(grid_dimensions[2] != 1)
                    grid_dimensions[2] = grid_dimensions[2] - 1;
                break;
            }
            else
            {
                if(!locked_grid_dimensions[0])
                    grid_dimensions[0] = grid_dimensions[0] + 1;
                if(!locked_grid_dimensions[1])
                    grid_dimensions[1] = grid_dimensions[1] + 1;
                if(!locked_grid_dimensions[2])
                    grid_dimensions[2] = grid_dimensions[2] + 1;
            }
        }
    }
    else
    {
        grid_dimensions = vec3ui(1, 1, 1); // 0 dimensions, every surfel hast the same position
    }

    return std::make_pair(grid_dimensions, locked_grid_dimensions);
}

surfel_mem_array reduction_normal_deviation_clustering::create_lod(real &reduction_error, const std::vector<surfel_mem_array *> &input, std::vector<LoDMetaData> &deviations,
                                                                   const uint32_t surfels_per_node, const bvh &tree, const size_t start_node_id) const
{
    // compute bounding box for actual surfels
    bounding_box bbox = basic_algorithms::compute_aabb(*input[0], true);

    for(auto child_surfels = input.begin() + 1; child_surfels != input.end(); ++child_surfels)
    {
        bounding_box child_bb = basic_algorithms::compute_aabb(*(*child_surfels), true);
        bbox.expand(child_bb);
    }

    vec3r bb_dimensions = bbox.get_dimensions();

    // compute grid dimensions
    std::pair<vec3ui, vec3b> grid_data = compute_grid_dimensions(input, bbox, surfels_per_node);
    vec3ui grid_dimensions = grid_data.first;
    vec3b locked_grid_dimensions = grid_data.second;

    // create grid
    std::vector<std::vector<std::vector<std::list<surfel> *>>> grid(grid_dimensions[0],
                                                                    std::vector<std::vector<std::list<surfel> *>>(grid_dimensions[1], std::vector<std::list<surfel> *>(grid_dimensions[2])));

    for(uint32_t i = 0; i < grid_dimensions[0]; ++i)
    {
        for(uint32_t j = 0; j < grid_dimensions[1]; ++j)
        {
            for(uint32_t k = 0; k < grid_dimensions[2]; ++k)
            {
                grid[i][j][k] = new std::list<surfel>;
            }
        }
    }

    // sort surfels into grid
    vec3r cell_size = vec3r(fabs(bb_dimensions[0] / grid_dimensions[0]), fabs(bb_dimensions[1] / grid_dimensions[1]), fabs(bb_dimensions[2] / grid_dimensions[2]));

    for(uint32_t i = 0; i < input.size(); ++i)
    {
        for(uint32_t j = 0; j < input[i]->length(); ++j)
        {
            vec3r surfel_pos = input[i]->read_surfel_ref(j).pos() - bbox.min();
            if(surfel_pos.x < 0.f)
                surfel_pos.x = 0.f;
            if(surfel_pos.y < 0.f)
                surfel_pos.y = 0.f;
            if(surfel_pos.z < 0.f)
                surfel_pos.z = 0.f;

            vec3ui index;

            if(locked_grid_dimensions[0])
            {
                index[0] = 0;
            }
            else
            {
                index[0] = (uint32_t)floor(surfel_pos[0] / cell_size[0]);
            }

            if(locked_grid_dimensions[1])
            {
                index[1] = 0;
            }
            else
            {
                index[1] = (uint32_t)floor(surfel_pos[1] / cell_size[1]);
            }

            if(locked_grid_dimensions[2])
            {
                index[2] = 0;
            }
            else
            {
                index[2] = (uint32_t)floor(surfel_pos[2] / cell_size[2]);
            }

            if((index[0] != 0) && (index[0] == grid_dimensions[0]))
                index[0] = grid_dimensions[0] - 1;

            if((index[1] != 0) && (index[1] == grid_dimensions[1]))
                index[1] = grid_dimensions[1] - 1;

            if((index[2] != 0) && (index[2] == grid_dimensions[2]))
                index[2] = grid_dimensions[2] - 1;

            grid[index[0]][index[1]][index[2]]->push_back(input[i]->read_surfel_ref(j));
        }
    }

    // move grid cells into priority queue

    std::priority_queue<surfel_cluster_with_error, std::vector<surfel_cluster_with_error>, order_by_size> cell_pq;
    uint32_t surfel_count = 0;

    // TODO: inject a parallel structure with provenance data
    std::priority_queue<provenance_cluster, std::vector<provenance_cluster>, pro_order_by_size> prov_pq;

    for(uint32_t i = 0; i < grid_dimensions[0]; ++i)
    {
        for(uint32_t j = 0; j < grid_dimensions[1]; ++j)
        {
            for(uint32_t k = 0; k < grid_dimensions[2]; ++k)
            {
                cell_pq.push({grid[i][j][k], 0.1f});
                prov_pq.push({generate_provenance_empties(grid[i][j][k])});
                surfel_count += grid[i][j][k]->size();
                grid[i][j][k] = 0;
            }
        }
    }

    size_t termination_ctr = 0;

    // merge surfels

    while(surfel_count > surfels_per_node)
    {
        // safety check
        if(termination_ctr++ >= 30000)
        {
            LOGGER_WARN("Reached maximum number of iterations in ndc!"
                        << " Current surfel count:" << surfel_count);
            break;
        }

        std::list<surfel> *input_cluster = cell_pq.top().cluster;
        float merge_treshold = cell_pq.top().merge_treshold;
        cell_pq.pop();

        // TODO: shadow the actions on the cell queue
        prov_pq.pop();

        //        printf("\ncell_pq.size(): %lu ", cell_pq.size());
        //        printf("\nprov_pq.size(): %lu ", prov_pq.size());

        uint32_t input_cluster_size = (uint32_t)input_cluster->size();
        surfel_count -= input_cluster_size;
        bool early_termination = false;

        real min_radius = input_cluster->begin()->radius();
        real max_radius = input_cluster->begin()->radius();

        auto start = input_cluster->begin();
        std::advance(start, 1);

        for(auto surfel = start; surfel != input_cluster->end(); ++surfel)
        {
            if(surfel->radius() < min_radius)
                min_radius = surfel->radius();
            else if(surfel->radius() > max_radius)
                max_radius = surfel->radius();
        }

        // real radius_range = max_radius - min_radius;

        std::list<surfel> *output_cluster = new std::list<surfel>;

        // TODO: create a container to cell data
        std::list<LoDMetaData> provenance_cluster = std::list<LoDMetaData>();

        // TODO: extract input surfels, keep a vector of states (merged/unmerged)
        //        std::vector<surfel> input_surfels(input_cluster->size());
        //        std::vector<bool> input_surfels_states(input_cluster->size());

        while(input_cluster->size() != 0)
        {
            std::vector<surfel> surfels_to_merge;
            surfels_to_merge.push_back(input_cluster->front());
            // TODO: register input surfel
            //            input_surfels.push_back(input_cluster->front());
            //            input_surfels_states.push_back(true);

            input_cluster->pop_front();

            std::list<surfel>::iterator surfel_to_compare = input_cluster->begin();

            while(surfel_to_compare != input_cluster->end())
            {
                // angle
                vec3f normal1 = surfels_to_merge.front().normal();
                vec3f normal2 = (*surfel_to_compare).normal();

                bool flip_normal = false;

                float dot_product = scm::math::dot(normal1, normal2);

                if(dot_product > 1.0)
                    dot_product = 1.0f;
                else if((dot_product < -1.0))
                    dot_product = -1.0f;

                if(dot_product < 0)
                {
                    flip_normal = true;
                    dot_product *= -1;
                }
                float angle = (float)acos(dot_product);
                float angle_normalized = (float)(angle / (0.5 * M_PI));

                // color difference
                /*
                auto color1 = surfels_to_merge.front().color();
                auto color2 = (*surfel_to_compare).color();

                float color1_normalized = (color1[0] + color1[1] + color1[2])/255*3.0;
                float color2_normalized = (color2[0] + color2[1] + color2[2])/255*3.0;

                float color_difference_normalized = std::abs(color1_normalized - color2_normalized);
                */

                // radius
                /*
                real radius1 = surfels_to_merge.front().radius();
                real radius2 = (*surfel_to_compare).radius();

                real radius_weight_normalized = 0;

                if (radius_range != 0)
                    radius_weight_normalized = ((radius1 - min_radius) + (radius2 - min_radius)) / (2.0*radius_range);
                */

                if(angle_normalized <= merge_treshold)
                {
                    if(flip_normal)
                    {
                        surfel_to_compare->normal() = surfel_to_compare->normal() * (-1.0);
                    }

                    surfels_to_merge.push_back(*surfel_to_compare);

                    // TODO: remember the surfel
                    //                    input_surfels.push_back(*surfel_to_compare);
                    //                    input_surfels_states.push_back(true);

                    surfel_to_compare = input_cluster->erase(surfel_to_compare);

                    if((surfel_count + input_cluster->size() + output_cluster->size() + 1) <= surfels_per_node)
                    {
                        early_termination = true;
                        break;
                    }
                }
                else
                {
                    // TODO: remember the surfel
                    //                    input_surfels.push_back(*surfel_to_compare);
                    //                    input_surfels_states.push_back(false);

                    std::advance(surfel_to_compare, 1);
                }
            }
            surfel repr = create_representative(surfels_to_merge);

            LoDMetaData data = calculate_deviations(repr, surfels_to_merge);
            data._debug_red = float(repr.color().x);
            data._debug_green = float(repr.color().y);
            data._debug_blue = float(repr.color().z);

            output_cluster->push_back(repr);

            // TODO: shadow the actions on the cell container
            provenance_cluster.push_back(data);

            //            printf("\nsurfel_count: %u ", surfel_count);
            //            printf("\ndeviations: %lu ", deviations.size());
            //            printf("\ninput_cluster->size(): %lu ", input_cluster->size());
            //            printf("\noutput_cluster->size(): %lu ", output_cluster->size());

            if(early_termination)
            {
                // TODO: calculate provenance data here from output_cluster->end() and surfels_to_merge
                // TODO: catch leftover surfels
                //                for(int64_t i = input_surfels.size(); i >= 0; i--)
                //                {
                //                    if(input_surfels_states[i])
                //                    {
                //                        LoDMetaData data = calculate_deviations(output_cluster->back(), surfels_to_merge);
                //                        data._debug_red = float(input_surfels[i].color().x);
                //                        data._debug_green = float(input_surfels[i].color().y);
                //                        data._debug_blue = float(input_surfels[i].color().z);
                //                        deviations.push_back(data);
                //                    }
                //                    //                    else{
                //                    //                        LoDMetaData empties;
                //                    //                        empties._mean_absolute_deviation = -1;
                //                    //                        empties._coefficient_of_variation = -1;
                //                    //                        empties._standard_deviation = -1;
                //                    //                        empties._debug_red = float(input_surfels[i].color().x);
                //                    //                        empties._debug_green = float(input_surfels[i].color().y);
                //                    //                        empties._debug_blue = float(input_surfels[i].color().z);
                //                    //                        deviations.push_back(empties);
                //                    //                    }
                //                }

                std::list<surfel>::iterator p_surfel = input_cluster->begin();

                while(p_surfel != input_cluster->end())
                {
                    LoDMetaData empty;
                    empty._mean_absolute_deviation = -1;
                    empty._coefficient_of_variation = -1;
                    empty._standard_deviation = -1;
                    empty._debug_red = float((*p_surfel).color().x);
                    empty._debug_green = float((*p_surfel).color().y);
                    empty._debug_blue = float((*p_surfel).color().z);

                    // TODO: shadow the actions on the cell container
                    provenance_cluster.push_back(empty);
                    std::advance(p_surfel, 1);
                }

                output_cluster->insert(output_cluster->end(), input_cluster->begin(), input_cluster->end());

                //                printf("\nearly_termination: %i", early_termination);
                //                printf("\nsurfel_count: %u ", surfel_count);
                //                printf("\ndeviations: %lu ", deviations.size());
                //                printf("\ninput_cluster->size(): %lu ", input_cluster->size());
                //                printf("\noutput_cluster->size(): %lu ", output_cluster->size());

                break;
            }

            // TODO: calculate provenance data here from output_cluster->end() and surfels_to_merge
            //            LoDMetaData data = calculate_deviations(output_cluster->back(), surfels_to_merge);
            //            for(int64_t i = input_surfels.size(); i >= 0; i--)
            //            {
            //                if(input_surfels_states[i])
            //                {
            //                    LoDMetaData data = calculate_deviations(output_cluster->back(), surfels_to_merge);
            //                    data._debug_red = float(input_surfels[i].color().x);
            //                    data._debug_green = float(input_surfels[i].color().y);
            //                    data._debug_blue = float(input_surfels[i].color().z);
            //                    deviations.push_back(data);
            //                }
            //            }
        }

        surfel_count += output_cluster->size();

        if(input_cluster_size == output_cluster->size())
        {
            merge_treshold += 0.1;
        }

        delete input_cluster;
        input_cluster = output_cluster;
        cell_pq.push({input_cluster, merge_treshold});

        // TODO: shadow the actions on the cell queue
        prov_pq.push({provenance_cluster});
    }

    surfel_mem_array mem_array(std::make_shared<surfel_vector>(surfel_vector()), 0, 0);

    while(!cell_pq.empty())
    {
        std::list<surfel> *cluster = cell_pq.top().cluster;
        cell_pq.pop();

        // TODO: shadow the actions on the cell queue
        std::list<LoDMetaData> prov_cluster = prov_pq.top().cluster;
        prov_pq.pop();

        std::list<surfel>::iterator surfel = cluster->begin();
        std::list<LoDMetaData>::iterator meta_data = prov_cluster.begin();

        for(; surfel != cluster->end() && meta_data != prov_cluster.end(); ++surfel, ++meta_data)
        {
            mem_array.mem_data()->push_back(*surfel);
            deviations.push_back(*meta_data);

//            printf("\ncell_red: %f ", (double)(*surfel).color().r);
//            printf("\ndev_red: %f ", (*meta_data)._debug_red);
        }

        delete cluster;
    }

    std::reverse(deviations.begin(), deviations.end());

    mem_array.set_length(mem_array.mem_data()->size());

    //    printf("\nLENGTH: %lu ", mem_array.length());
    //    printf("\nDEVIATIONS: %lu ", deviations.size());

    reduction_error = 0; // TODO

    return mem_array;
}

reduction_normal_deviation_clustering::LoDMetaData reduction_normal_deviation_clustering::calculate_deviations(surfel repr, const std::vector<surfel> &input) const
{
    assert(input.size() > 0);

    LoDMetaData data;

    data._mean_absolute_deviation = 0;
    data._standard_deviation = 0;
    data._coefficient_of_variation = -1;

    if(input.size() == 1)
        return data;

    vec3f repr_normal = repr.normal();

    for(const auto &surfel : input)
    {
        vec3f normal = surfel.normal();

        double absolute_deviation = scm::math::abs(acos(scm::math::dot(repr_normal, normal)) / (0.5 * M_PI));
        double sq_deviation = scm::math::sqr(absolute_deviation);

        data._mean_absolute_deviation += absolute_deviation;
        data._standard_deviation += sq_deviation;
    }

    data._mean_absolute_deviation /= (float)input.size();
    data._standard_deviation = (float)scm::math::sqrt(data._standard_deviation / (double)input.size());
    data._coefficient_of_variation = (float)data._mean_absolute_deviation != 0 ? data._standard_deviation / data._mean_absolute_deviation : -1;

    //    printf("\nMAD: %e ", data._mean_absolute_deviation);
    //    printf("\nSTD: %e ", data._standard_deviation);
    //    printf("\nCoV: %e ", data._coefficient_of_variation);

    return data;
}

std::list<provenance_reduction_strategy::LoDMetaData> reduction_normal_deviation_clustering::generate_provenance_empties(std::list<surfel> *&surfels) const
{
    std::list<provenance_reduction_strategy::LoDMetaData> empties = std::list<provenance_reduction_strategy::LoDMetaData>();

    for(std::list<surfel>::iterator p_surfel = surfels->begin(); p_surfel != surfels->end(); ++p_surfel)
    {
        provenance_reduction_strategy::LoDMetaData empty;
        empty._mean_absolute_deviation = -1;
        empty._coefficient_of_variation = -1;
        empty._standard_deviation = -1;
        empty._debug_red = float((*p_surfel).color().x);
        empty._debug_green = float((*p_surfel).color().y);
        empty._debug_blue = float((*p_surfel).color().z);
        empties.push_back(empty);
    }

    return empties;
}

} // namespace pre
} // namespace lamure
