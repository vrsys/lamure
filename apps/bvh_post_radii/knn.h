
#ifndef KNN_H_INCLUDED
#define KNN_H_INCLUDED

#include <vector>
#include <unordered_set>

#include <lamure/ren/bvh.h>
#include <lamure/ren/lod_stream.h>

#include "types.h"

namespace knn {


static bool contains_sphere(const scm::gl::boxf& box, const Sphere& sphere) {

    scm::math::vec3f s_min = sphere.center_ - sphere.radius_;
    scm::math::vec3f s_max = sphere.center_ + sphere.radius_;

    bool min_contained = box.min_vertex().x < s_min.x && s_min.x < box.max_vertex().x &&
                       box.min_vertex().y < s_min.y && s_min.y < box.max_vertex().y &&
                       box.min_vertex().z < s_min.z && s_min.z < box.max_vertex().z;

    bool max_contained = box.min_vertex().x < s_max.x && s_max.x < box.max_vertex().x &&
                       box.min_vertex().y < s_max.y && s_max.y < box.max_vertex().y &&
                       box.min_vertex().z < s_max.z && s_max.z < box.max_vertex().z;

    return min_contained && max_contained;
}


static bool intersects(const Sphere& sphere, const scm::gl::boxf& box) {
    float dist_sqr = sphere.radius_ * sphere.radius_;

    if (sphere.center_.x < box.min_vertex().x)
        dist_sqr -= std::pow((sphere.center_.x - box.min_vertex().x), 2);
    else if (sphere.center_.x > box.max_vertex().x)
        dist_sqr -= std::pow((sphere.center_.x - box.max_vertex().x), 2);

    if (sphere.center_.y < box.min_vertex().y)
        dist_sqr -= std::pow((sphere.center_.y - box.min_vertex().y), 2);
    else if (sphere.center_.y > box.max_vertex().y)
        dist_sqr -= std::pow((sphere.center_.y - box.max_vertex().y), 2);

    if (sphere.center_.z < box.min_vertex().z)
        dist_sqr -= std::pow((sphere.center_.z - box.min_vertex().z), 2);
    else if (sphere.center_.z > box.max_vertex().z)
        dist_sqr -= std::pow((sphere.center_.z - box.max_vertex().z), 2);

    return dist_sqr > 0;
}


static void get_all_descendant_leafs(const lamure::ren::Bvh* bvh, const lamure::node_t node_id, unsigned int leaf_depth, std::vector<lamure::node_t>& descendant_leafs) {

  unsigned int node_depth = bvh->GetDepthOfNode(node_id);

  if (node_depth == leaf_depth) {
    descendant_leafs.push_back(node_id);
  }
  else {
    for (unsigned int i = 0; i < bvh->fan_factor(); ++i) {
      get_all_descendant_leafs(bvh, bvh->GetChildId(node_id, i), leaf_depth, descendant_leafs);
    }
  }

}

static scm::math::vec3f get_point_of_interest(
    const lamure::ren::Bvh* bvh,
    const lamure::ren::LodStream* lod_access,
    lamure::node_t node_id, 
    size_t splat_id) {
    
    Surfel* surfel = new Surfel();
    size_t node_size_in_bytes = bvh->surfels_per_node() * sizeof(Surfel);
    lod_access->Read((char*)surfel, (size_t)node_id * node_size_in_bytes + (splat_id*sizeof(Surfel)), sizeof(Surfel));

    return scm::math::vec3f(surfel->x, surfel->y, surfel->z);
    
}

static void find_nearest_neighbours(const lamure::ren::Bvh* bvh,
    const lamure::ren::LodStream* lod_access,
    const NodeSplatId& splat_of_interest,
    unsigned int num_neighbours,
    std::vector<std::pair<Surfel, float>>& neighbours,
    bool out_of_core,
    Surfel* surfels) {

    lamure::node_t current_node_id = splat_of_interest.node_id_;
    unsigned int knn_leaf_depth = bvh->GetDepthOfNode(current_node_id);
    size_t node_size_in_bytes = bvh->surfels_per_node() * sizeof(Surfel);

    size_t node_offset_in_splats = 0;
    if (out_of_core) {
        //read initial node
        lod_access->Read((char*)surfels, (size_t)current_node_id * node_size_in_bytes, node_size_in_bytes);
    }
    else {
        node_offset_in_splats = current_node_id * bvh->surfels_per_node();
    }
    
    Surfel center_splat = surfels[node_offset_in_splats + splat_of_interest.splat_id_];
    scm::math::vec3f center = scm::math::vec3f(center_splat.x, center_splat.y, center_splat.z);


    std::vector<std::pair<Surfel, float>> candidates;
    float max_distance_squared = 0.f;

    //check initial node
    for (size_t i = 0; i < bvh->surfels_per_node(); ++i) {
        if (i != splat_of_interest.splat_id_) {
            const Surfel& surfel = surfels[node_offset_in_splats + i];
            float distance_squared = scm::math::length_sqr(center - scm::math::vec3f(surfel.x, surfel.y, surfel.z));

            if (candidates.size() < num_neighbours || distance_squared < max_distance_squared) {

                candidates.push_back(std::make_pair(surfel, distance_squared));

                for (int32_t k = (int32_t)candidates.size() - 1; k > 0; --k) {
                    if (candidates[k].second < candidates[k - 1].second) {
                        std::pair<Surfel, float> temp = candidates [k - 1];
                        candidates[k - 1] = candidates[k];
                        candidates[k] = temp;
                    }
                    else {
                        break;
                    }
                }
/*
                std::sort(candidates.begin(), candidates.end(), [](const std::pair<Surfel, float>& a, const std::pair<Surfel, float>& b)->bool { 
                    return a.second < b.second; 
                });
*/                                  
                if (candidates.size() > num_neighbours) {
                    candidates.pop_back();
                }
                
                //max_distance_squared = candidates.back().second;
                max_distance_squared = std::max(max_distance_squared, candidates.back().second);

            }

        }

    }


    std::unordered_set<lamure::node_t> processed_leafs;
    processed_leafs.insert(current_node_id);
    Sphere knn_sphere = Sphere(center, scm::math::sqrt(max_distance_squared));

    //traverse rest of tree
    while (!contains_sphere(bvh->bounding_boxes()[current_node_id], knn_sphere)) {

        if (current_node_id == 0) {
            break;
        }
        current_node_id = bvh->GetParentId(current_node_id);

        std::vector<lamure::node_t> descendant_leafs;
        get_all_descendant_leafs(bvh, current_node_id, knn_leaf_depth, descendant_leafs);

        for (const auto& leaf_id : descendant_leafs) {
            //check if leaf already processed
            if (processed_leafs.find(leaf_id) != processed_leafs.end()) {
                continue;
            }

            if (leaf_id == splat_of_interest.node_id_) {
                continue;
            }

            if (leaf_id == lamure::invalid_node_t) {
                continue;
            }


            //check if leaf relevant
            if (intersects(knn_sphere, bvh->bounding_boxes()[leaf_id])) {

                if (out_of_core) {
                    //load leaf data
                    lod_access->Read((char*)surfels, (size_t)leaf_id * node_size_in_bytes, node_size_in_bytes);
                }
                else {
                    node_offset_in_splats = leaf_id * bvh->surfels_per_node();
                }
                
                for (size_t i = 0; i < bvh->surfels_per_node(); ++i) {

                    const Surfel& surfel = surfels[node_offset_in_splats + i];
                    float distance_squared = scm::math::length_sqr(center - scm::math::vec3f(surfel.x, surfel.y, surfel.z));

                    if (candidates.size() < num_neighbours || distance_squared < max_distance_squared) {

                        candidates.push_back(std::make_pair(surfel, distance_squared));

                        for (int32_t k = (int32_t)candidates.size() - 1; k > 0; --k) {
                            if (candidates[k].second < candidates[k - 1].second) {
                                std::pair<Surfel, float> temp = candidates [k - 1];
                                candidates[k - 1] = candidates[k];
                                candidates[k] = temp;
                            }
                            else {
                                break;
                            }
                        }

/*
                std::sort(candidates.begin(), candidates.end(), [](const std::pair<Surfel, float>& a, const std::pair<Surfel, float>& b)->bool { 
                    return a.second < b.second; 
                });
*/                                        
                        if (candidates.size() > num_neighbours) {
                            candidates.pop_back();
                        }

                        //max_distance_squared = candidates.back().second;
                        max_distance_squared = std::max(max_distance_squared, candidates.back().second);
                        

                    }

                }

                processed_leafs.insert(leaf_id);
                knn_sphere = Sphere(center, scm::math::sqrt(max_distance_squared));


            }

        }
        
        descendant_leafs.clear();

    }
    

    //std::copy(candidates.begin(), std::min(candidates.begin()+num_neighbours, candidates.end()), neighbours.begin());

    uint32_t num_found = std::min((uint64_t)num_neighbours, (uint64_t)candidates.size());
    for (unsigned int i = 0; i < num_found; ++i) {
        std::pair<Surfel, float>& pair = candidates[i];
        //dbg: is poi contained?
        if (pair.first.x == center.x && pair.first.y == center.y && pair.first.z == center.z) {
            continue;
        }
        neighbours.push_back(pair);
    }
    
    candidates.clear();
    processed_leafs.clear();
    
    

}



}

#endif
