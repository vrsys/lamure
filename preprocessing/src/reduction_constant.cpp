// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/reduction_constant.h>

#include <lamure/pre/basic_algorithms.h>
#include <lamure/utils.h>

#include <queue>

#if WIN32
  #include <ppl.h>
#else
  #include <parallel/algorithm>
#endif

namespace lamure {
namespace pre {

surfel reduction_constant::
create_representative(const std::vector<surfel>& input)
{
    assert(input.size() > 0);

    //const float fan_factor = 2;
    //const float mult = sqrt(1.f + 1.f / fan_factor)*1.0f;

//    real radius = input.front().radius() * mult;

    if (input.size() == 1) {
        return input.front();
//        return surfel(input.front().pos(), input.front().color(), radius, input.front().normal());
    }

    vec3r pos = vec3r(0);
    vec3f nml = vec3f(0);
    vec3f col = vec3f(0);
    real weight_sum = 0.0;
    for (const auto& surfel : input)
    {
        real weight = 1.0; //surfel.radius();
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

    for (const auto& surfel : input) {
        real dist = scm::math::distance(pos, surfel.pos());
        if (radius < dist + surfel.radius()) radius = dist + surfel.radius();
    }

    return surfel(pos, vec3b(col.x, col.y, col.z), radius, nml);
}

std::pair<vec3ui, vec3b> reduction_constant::
compute_grid_dimensions(const std::vector<surfel_mem_array*>& input,
                        const bounding_box& bounding_box,
                        const uint32_t surfels_per_node)
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

    if  ((sorted_bb_dimensions[0].first == 0) &&
        (sorted_bb_dimensions[1].first == 0) &&
        (sorted_bb_dimensions[2].first == 0))
    {
        // 0 dimensions
        bb_zero_size = true;
    }
    else if ((sorted_bb_dimensions[0].first == 0) &&
            (sorted_bb_dimensions[1].first == 0))
    {
        // 1 dimension
        sorted_locked_grid_dimensions[0] = true;
        sorted_locked_grid_dimensions[1] = true;
        sorted_grid_dimensions = vec3ui(1,1,1);
    }
    else if (sorted_bb_dimensions[0].first == 0)
    {
        // 2 dimensions
        sorted_locked_grid_dimensions[0] = true;

        if (sorted_bb_dimensions[1].first < sorted_bb_dimensions[2].first)
        {
            sorted_grid_dimensions = vec3ui(1,1,floor(sorted_bb_dimensions[2].first/sorted_bb_dimensions[1].first));
            if (sorted_grid_dimensions[2] > max_axis_ratio)
                sorted_grid_dimensions[2] = max_axis_ratio;
        }
        else
        {
            sorted_grid_dimensions = vec3ui(1,floor(sorted_bb_dimensions[1].first/sorted_bb_dimensions[2].first),1);
            if (sorted_grid_dimensions[1] > max_axis_ratio)
                sorted_grid_dimensions[1] = max_axis_ratio;
        }

    } else {
        // 3 dimensions
        sorted_grid_dimensions = vec3ui(1,floor(sorted_bb_dimensions[1].first/sorted_bb_dimensions[0].first),floor(sorted_bb_dimensions[2].first/sorted_bb_dimensions[0].first));
        if (sorted_grid_dimensions[1] > max_axis_ratio)
            sorted_grid_dimensions[1] = max_axis_ratio;
        if (sorted_grid_dimensions[2] > max_axis_ratio)
            sorted_grid_dimensions[2] = max_axis_ratio;

    }

    if (!bb_zero_size) { // at least 1 dimesion

        // revert sorting
        grid_dimensions[sorted_bb_dimensions[0].second] = sorted_grid_dimensions[0];
        locked_grid_dimensions[sorted_bb_dimensions[0].second] = sorted_locked_grid_dimensions[0];

        grid_dimensions[sorted_bb_dimensions[1].second] = sorted_grid_dimensions[1];
        locked_grid_dimensions[sorted_bb_dimensions[1].second] = sorted_locked_grid_dimensions[1];

        grid_dimensions[sorted_bb_dimensions[2].second] = sorted_grid_dimensions[2];
        locked_grid_dimensions[sorted_bb_dimensions[2].second] = sorted_locked_grid_dimensions[2];

        // adapt total number of grid cells to number of surfels per node
        uint32_t total_grid_dimensions = (grid_dimensions[0]*grid_dimensions[1]*grid_dimensions[2]);

        if (total_grid_dimensions < surfels_per_node) // if less cells than surfels per node: increase
        {
            while (((grid_dimensions[0]+1)*(grid_dimensions[1]+1)*(grid_dimensions[2]+1)) < surfels_per_node)
            {
                if (!locked_grid_dimensions[0])
                    grid_dimensions[0] = grid_dimensions[0]+1;
                if (!locked_grid_dimensions[1])
                    grid_dimensions[1] = grid_dimensions[1]+1;
                if (!locked_grid_dimensions[2])
                    grid_dimensions[2] = grid_dimensions[2]+1;
                total_grid_dimensions = (grid_dimensions[0]*grid_dimensions[1]*grid_dimensions[2]);
            }
        }
        else if (total_grid_dimensions > surfels_per_node) // if more cells than surfels per node: decrease
        {
            while (total_grid_dimensions > surfels_per_node)
            {
                if (grid_dimensions[0] != 1)
                    grid_dimensions[0] = grid_dimensions[0]-1;
                if (grid_dimensions[1] != 1)
                    grid_dimensions[1] = grid_dimensions[1]-1;
                if (grid_dimensions[2] != 1)
                    grid_dimensions[2] = grid_dimensions[2]-1;
                total_grid_dimensions = (grid_dimensions[0]*grid_dimensions[1]*grid_dimensions[2]);
            }
        }

        // adapt occupied number of grid cells to number of surfels per node
        while (true) {

            // create grid
            std::vector<std::vector<std::vector<bool>>> grid(grid_dimensions[0],std::vector<std::vector<bool>>(grid_dimensions[1], std::vector<bool>(grid_dimensions[2])));

            for (uint32_t i = 0; i < grid_dimensions[0]; ++i)
            {
                for (uint32_t j = 0; j < grid_dimensions[1]; ++j)
                {
                    for (uint32_t k = 0; k < grid_dimensions[2]; ++k)
                    {
                        grid[i][j][k] = false;
                    }
                }
            }

            // check which cell a surfel occupies
            vec3r cell_size = vec3r(fabs(bb_dimensions[0]/grid_dimensions[0]),fabs(bb_dimensions[1]/grid_dimensions[1]),fabs(bb_dimensions[2]/grid_dimensions[2]));

            for (uint32_t i = 0; i < input.size(); ++i)
            {
                for (uint32_t j = 0; j < input[i]->length(); ++j)
                {
                    surfel surfel = input[i]->read_surfel(j);

                    vec3r surfel_pos = surfel.pos() - bounding_box.min();
                    if (surfel_pos.x < 0.f) surfel_pos.x = 0.f;
                    if (surfel_pos.y < 0.f) surfel_pos.y = 0.f;
                    if (surfel_pos.z < 0.f) surfel_pos.z = 0.f;

                    vec3ui index;

                    if (locked_grid_dimensions[0]) {
                        index[0] = 0;
                    } else {
                        index[0] = floor(surfel_pos[0]/cell_size[0]);
                    }

                    if (locked_grid_dimensions[1]) {
                        index[1] = 0;
                    } else {
                        index[1] = floor(surfel_pos[1]/cell_size[1]);
                    }

                    if (locked_grid_dimensions[2]) {
                        index[2] = 0;
                    } else {
                         index[2] = floor(surfel_pos[2]/cell_size[2]);
                    }

                    if ((index[0] != 0) && (index[0] == grid_dimensions[0]))
                        index[0] = grid_dimensions[0]-1;

                    if ((index[1] != 0) && (index[1] == grid_dimensions[1]))
                        index[1] = grid_dimensions[1]-1;

                    if ((index[2] != 0) && (index[2] == grid_dimensions[2]))
                        index[2] = grid_dimensions[2]-1;

                    grid[index[0]][index[1]][index[2]] = true;

                }
            }

            // count occupied cells
            uint32_t occupied_cells = 0;

            for (uint32_t i = 0; i < grid_dimensions[0]; ++i)
            {
                for (uint32_t j = 0; j < grid_dimensions[1]; ++j)
                {
                    for (uint32_t k = 0; k < grid_dimensions[2]; ++k)
                    {
                        if (grid[i][j][k])
                            ++occupied_cells;
                    }
                }
            }

            // check if finished
            if ((occupied_cells > surfels_per_node) || (grid_dimensions[0]*grid_dimensions[1]*grid_dimensions[2] > 100000))  {

                if (grid_dimensions[0] != 1)
                    grid_dimensions[0] = grid_dimensions[0]-1;
                if (grid_dimensions[1] != 1)
                    grid_dimensions[1] = grid_dimensions[1]-1;
                if (grid_dimensions[2] != 1)
                    grid_dimensions[2] = grid_dimensions[2]-1;
                break;

            } else {

                if (!locked_grid_dimensions[0])
                    grid_dimensions[0] = grid_dimensions[0]+1;
                if (!locked_grid_dimensions[1])
                    grid_dimensions[1] = grid_dimensions[1]+1;
                if (!locked_grid_dimensions[2])
                    grid_dimensions[2] = grid_dimensions[2]+1;

            }
        }
    }
    else
    {
        grid_dimensions = vec3ui(1,1,1); // 0 dimensions, every surfel hast the same position
    }

    return std::make_pair(grid_dimensions, locked_grid_dimensions);

}

surfel_mem_array reduction_constant::
create_lod(real& reduction_error,
          const std::vector<surfel_mem_array*>& input,
          const real avg_radius_all_nodes,
          const uint32_t surfels_per_node,
          const bvh& tree,
          const size_t start_node_id) const
{
    // compute bounding box for actual surfels
    bounding_box bbox = basic_algorithms::compute_aabb(*input[0], true);

    for (auto child_surfels = input.begin() + 1; child_surfels !=input.end(); ++child_surfels)
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
    std::vector<std::vector<std::vector<std::list<surfel>*>>> grid(grid_dimensions[0],std::vector<std::vector<std::list<surfel>*>>(grid_dimensions[1], std::vector<std::list<surfel>*>(grid_dimensions[2])));

    for (uint32_t i = 0; i < grid_dimensions[0]; ++i)
    {
        for (uint32_t j = 0; j < grid_dimensions[1]; ++j)
        {
            for (uint32_t k = 0; k < grid_dimensions[2]; ++k)
            {
                grid[i][j][k] = new std::list<surfel>;
            }
        }
    }

    // sort surfels into grid
    vec3r cell_size = vec3r(fabs(bb_dimensions[0]/grid_dimensions[0]),fabs(bb_dimensions[1]/grid_dimensions[1]),fabs(bb_dimensions[2]/grid_dimensions[2]));

    for (uint32_t i = 0; i < input.size(); ++i)
    {
        for (uint32_t j = 0; j < input[i]->length(); ++j)
        {
            surfel surfel = input[i]->read_surfel(j);

            vec3r surfel_pos = surfel.pos() - bbox.min();
            if (surfel_pos.x < 0.f) surfel_pos.x = 0.f;
            if (surfel_pos.y < 0.f) surfel_pos.y = 0.f;
            if (surfel_pos.z < 0.f) surfel_pos.z = 0.f;

            vec3ui index;

            if (locked_grid_dimensions[0]) {
                index[0] = 0;
            } else {
                index[0] = floor(surfel_pos[0]/cell_size[0]);
            }

            if (locked_grid_dimensions[1]) {
                index[1] = 0;
            } else {
                index[1] = floor(surfel_pos[1]/cell_size[1]);
            }

            if (locked_grid_dimensions[2]) {
                index[2] = 0;
            } else {
                 index[2] = floor(surfel_pos[2]/cell_size[2]);
            }

            if ((index[0] != 0) && (index[0] == grid_dimensions[0]))
                index[0] = grid_dimensions[0]-1;

            if ((index[1] != 0) && (index[1] == grid_dimensions[1]))
                index[1] = grid_dimensions[1]-1;

            if ((index[2] != 0) && (index[2] == grid_dimensions[2]))
                index[2] = grid_dimensions[2]-1;

            grid[index[0]][index[1]][index[2]]->push_back(surfel);

        }
    }


    // move grid cells into priority queue

    std::priority_queue<surfel_cluster_with_error, std::vector<surfel_cluster_with_error>, order_by_size> cell_pq;
    uint32_t surfel_count = 0;

    for (uint32_t i = 0; i < grid_dimensions[0]; ++i)
    {
        for (uint32_t j = 0; j < grid_dimensions[1]; ++j)
        {
            for (uint32_t k = 0; k < grid_dimensions[2]; ++k)
            {
                cell_pq.push({grid[i][j][k], 0.1});
                surfel_count += grid[i][j][k]->size();
                grid[i][j][k] = 0;
            }
        }
    }

    // merge surfels
    while (surfel_count > surfels_per_node)
    {

        std::list<surfel>* input_cluster = cell_pq.top().cluster;
        float merge_treshold = cell_pq.top().merge_treshold;
        cell_pq.pop();

        uint32_t input_cluster_size = input_cluster->size();
        surfel_count -= input_cluster_size;
        bool early_termination = false;

        real min_radius = input_cluster->begin()->radius();
        real max_radius = input_cluster->begin()->radius();

        auto start = input_cluster->begin();
        std::advance(start, 1);

        for (auto surfel = start; surfel != input_cluster->end(); ++surfel) {
            if (surfel->radius() < min_radius)
                min_radius = surfel->radius();
            else if (surfel->radius() > max_radius)
                max_radius = surfel->radius();
        }

        //real radius_range = max_radius - min_radius;

        std::list<surfel>* output_cluster = new std::list<surfel>;

        while(input_cluster->size() != 0)
        {

            std::vector<surfel> surfels_to_merge;
            surfels_to_merge.push_back(input_cluster->front());
            input_cluster->pop_front();

            std::list<surfel>::iterator surfel_to_compare = input_cluster->begin();

            while(surfel_to_compare != input_cluster->end())
            {
                // angle
                vec3f normal1 = surfels_to_merge.front().normal();
                vec3f normal2 = (*surfel_to_compare).normal();

                bool flip_normal = false;

                float dot_product = scm::math::dot(normal1,normal2);

                if (dot_product > 1.0)
                    dot_product = 1.0;
                else if ((dot_product < -1.0))
                    dot_product = -1.0;

                if (dot_product < 0) {
                    flip_normal = true;
                    dot_product *= -1;
                }
                float angle = acos(dot_product);
                float angle_normalized = angle/(0.5*M_PI);

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

                angle_normalized = 0.f;

                if(angle_normalized <= merge_treshold)
                {
                    if (flip_normal) {
                        surfel_to_compare->normal() = surfel_to_compare->normal() * (-1.0);
                    }

                    surfels_to_merge.push_back(*surfel_to_compare);
                    surfel_to_compare = input_cluster->erase(surfel_to_compare);

                    if (( surfel_count + input_cluster->size() + output_cluster->size() + 1) <= surfels_per_node) {
                        early_termination = true;
                        break;
                    }

                } else {
                    std::advance(surfel_to_compare,1);
                }
            }
            output_cluster->push_back(create_representative(surfels_to_merge));

            if (early_termination) {
                output_cluster->insert(output_cluster->end(), input_cluster->begin(), input_cluster->end());
                break;
            }

        }

        surfel_count += output_cluster->size();

        if (input_cluster_size == output_cluster->size()) {
            merge_treshold += 0.1;

        }

        delete input_cluster;
        input_cluster = output_cluster;
        cell_pq.push({input_cluster, merge_treshold});

    }

    surfel_mem_array mem_array(std::make_shared<surfel_vector>(surfel_vector()), 0, 0);

    while (!cell_pq.empty())
    {
        std::list<surfel>* cluster = cell_pq.top().cluster;
        cell_pq.pop();

        for(std::list<surfel>::iterator surfel = cluster->begin(); surfel != cluster->end(); ++surfel)
        {
            mem_array.mem_data()->push_back(*surfel);
        }

        delete cluster;
    }

    mem_array.set_length(mem_array.mem_data()->size());

    reduction_error = 0; // TODO

    return mem_array;
}

} // namespace pre
} // namespace lamure
