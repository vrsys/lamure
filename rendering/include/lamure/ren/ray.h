// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef REN_RAY_H_
#define REN_RAY_H_

#include <stack>
#include <queue>
#include <mutex>
#include <thread>
#include <cstdlib>
#include <bitset>

#include <lamure/types.h>
#include <lamure/ren/platform.h>
#include <lamure/ren/bvh.h>
#include <lamure/ren/model_database.h>
#include <lamure/ren/ooc_cache.h>
#include <lamure/ren/lod_point_cloud.h>
#include <lamure/ren/semaphore.h>

#include <scm/core/math.h>
#include <scm/gl_core/primitives/box.h>

namespace lamure {
namespace ren {

class RENDERING_DLL Ray
{

public:

    //this is for splat-based picking
    struct Intersection {

        scm::math::vec3f position_;
        scm::math::vec3f normal_;
        float distance_;
        float error_;

        Intersection()
        : position_(scm::math::vec3::zero()), normal_(scm::math::vec3f::one()), distance_(0.f), error_(std::numeric_limits<float>::max()) {

        };

        Intersection(const scm::math::vec3f& position, const scm::math::vec3f& normal)
        : position_(position), normal_(normal), distance_(0.f), error_(std::numeric_limits<float>::max()) {

        };
    };

    //this is for bvh-based picking
    struct IntersectionBvh {
        scm::math::vec3f position_;
        float tmin_;
        float tmax_;
        float representative_radius_;
        std::string bvh_filename_;
        
        IntersectionBvh()
        : position_(scm::math::vec3::zero()), 
          tmin_(std::numeric_limits<float>::max()), 
          tmax_(std::numeric_limits<float>::lowest()), 
          representative_radius_(std::numeric_limits<float>::max()),
          bvh_filename_("") {

        };

    };

    Ray();
    Ray(const scm::math::vec3f& origin, const scm::math::vec3f& direction, const float max_distance);
    ~Ray();

    const scm::math::vec3f& origin() const { return origin_; };
    const scm::math::vec3f& direction() const { return direction_; };
    const float max_distance() const { return max_distance_; };

    void set_origin(const scm::math::vec3f& origin) { origin_ = origin; };
    void set_direction(const scm::math::vec3f& direction) { direction_ = direction; };
    void set_max_distance(const float max_distance) { max_distance_ = max_distance; };

    //this is a interpolation picking interface,
    //(all models, splat-based, fits a plane)
    const bool Intersect(const float aabb_scale,
                         scm::math::vec3f& ray_up_vector,
                         const float cone_diameter,
                         const unsigned int max_depth,
                         const unsigned int surfel_skip,
                         Intersection& intersection);

    //this is a BVH-only picking interface,
    //(all models, BVH-based, disambiguation)
    const bool IntersectBvh(const std::set<std::string>& model_filenames,
                            const float aabb_scale,
                            IntersectionBvh& intersection);

    //this is a splat-based pick of a single model,
    //(single model, splat-based)
    const bool IntersectModel(const model_t model_id,
                              const scm::math::mat4f& model_transform,
                              const float aabb_scale,
                              const unsigned int max_depth,
                              const unsigned int surfel_skip,
                              const bool is_wysiwyg,
                              Intersection& intersection);
  
    //this is a BVH-based pick of a single model,
    //(single model, BVH-based)
    const bool IntersectModelBvh(const model_t model_id,
                                 const scm::math::mat4f& model_transform,
                                 const float aabb_scale,
                                 IntersectionBvh& intersection);

protected:
 
    const bool IntersectModelUnsafe(const model_t model_id,
                              const scm::math::mat4f& model_transform,
                              const float aabb_scale,
                              const unsigned int max_depth,
                              const unsigned int surfel_skip,
                              const bool is_wysiwyg,
                              Intersection& intersection);
    static const bool IntersectAabb(const scm::gl::boxf& bb,
                                    const scm::math::vec3f& ray_origin,
                                    const scm::math::vec3f& ray_direction,
                                    scm::math::vec2f& t);
    static const bool IntersectSurfel(const LodPointCloud::SerializedSurfel& surfel,
                                      const scm::math::vec3f& ray_origin,
                                      const scm::math::vec3f& ray_direction,
                                      float& t);
private:
    scm::math::vec3f origin_;
    scm::math::vec3f direction_;
    float max_distance_;

};


class RayQueue {
public:

    struct RayJob {
        Ray ray_;
        int id_;

        RayJob()
        : ray_(scm::math::vec3f::zero(), scm::math::vec3f::zero(), -1.f), id_(-1) {

        }

        RayJob(unsigned int id, const Ray& ray)
        : ray_(ray), id_(id) {

        }
    };

    RayQueue();
    ~RayQueue();

    void PushJob(const RayJob& job);
    const RayJob PopJob();

    void Wait();
    void Relaunch();
    const bool IsShutdown();
    const unsigned int NumJobs();

private:
    std::queue<RayJob> queue_;
    std::mutex mutex_;
    Semaphore semaphore_;
    bool is_shutdown_;

};

}
}

#endif
