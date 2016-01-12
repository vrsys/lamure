// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/reduction_k_clustering.h>

#include <lamure/pre/basic_algorithms.h>
#include <lamure/utils.h>

#include <queue>
#include <array>
#include <utility>   // std::pair
#include <algorithm>    // std::max
#include <math.h>  //floor

namespace lamure {
namespace pre {




int reduction_k_clustering::
get_largest_dim(vec3f const& avg_norlam){

    // ^add abs

    if (!((avg_norlam.x == avg_norlam.y)&&(avg_norlam.z == avg_norlam.y))){

        if (avg_norlam.x == std::max(avg_norlam.x, avg_norlam.y) && avg_norlam.y >= avg_norlam.z){

            return 0;
        }
        else if (avg_norlam.y == std::max(avg_norlam.x, avg_norlam.y) && avg_norlam.x >= avg_norlam.z) {

            return 1;

        }
        else if (avg_norlam.z == std::max(avg_norlam.z, avg_norlam.y) && avg_norlam.y >= avg_norlam.x){

            return 2;
        }

    }
    else return 0; //^set to rand.


}

std::vector<surfel_with_neighbours> reduction_k_clustering::
get_inital_cluster_seeds(vec3f const& avg_norlam, std::vector<std::shared_ptr<surfel_with_neighbours>> input_surfels){


    int group_num = 8; //^ member var. if user-defind value needed  //^consider different index distribution funbction depending on this mun.
    uint16_t x_coord, y_coord ;  // 2D coordinate mapping
    uint16_t group_id;
    int num_elemets = -1;
    std::pair<>;


    std::array <std::vector<std::shared_ptr<surfel_with_neighbours>, group_num> cluster_array; // container with 8 (in this case) subgroups

    int anv_dim = get_largest_dim(avg_norlam);

    for (int i == 0, i < group_num, i++){


    }

    for (auto current_surfel : input_surfels){
        x_coord = std::floor((current_surfel->pos()[(avg_dim + 1) % 3] )/(current_surfel->radius()));
        y_coord = std::floor((current_surfel->pos()[(avg_dim + 2) % 3] )/(current_surfel->radius()));
        group_id = (x_coord*3 + y_coord) % group_id; //^formula needs to reconsidered for user-defined group_num
        cluster_array[group_id].push_back(current_surfel);

    }



}

surfel_mem_array reduction_k_clustering::
create_lod(real& reduction_error,
          const std::vector<surfel_mem_array*>& input,
          const uint32_t surfels_per_node) const
{



    return mem_array;
}

} // namespace pre
} // namespace lamure
