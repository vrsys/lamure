
#ifndef LAMURE_MESH_BVH_H_
#define LAMURE_MESH_BVH_H_

#include <lamure/types.h>
#include <lamure/mesh/triangle.h>
#include <lamure/mesh/triangle_chartid.h>
#include <lamure/mesh/edge_merger.h>
#include <lamure/ren/bvh.h>

#include <lamure/mesh/polyhedron.h>

#include <map>
#include <vector>

namespace lamure
{
namespace mesh
{
class bvh : public lamure::ren::bvh
{
  public:
    bvh(std::vector<Triangle_Chartid>& triangles, uint32_t primitives_per_node);
    ~bvh();

    void write_lod_file(const std::string& lod_filename);

    std::vector<Triangle_Chartid>& get_triangles(uint32_t node_id) { return triangles_map_[node_id]; }

  protected:
    struct bvh_node
    {
        uint32_t depth_; // depth of the node in the hierarchy
        vec3f min_;
        vec3f max_;
        uint64_t begin_; // first triangle id that belongs to this node
        uint64_t end_;   // last triangle
    };

    void create_hierarchy(std::vector<Triangle_Chartid>& triangles);

    void simplify(std::vector<Triangle_Chartid>& left_child_tris, std::vector<Triangle_Chartid>& right_child_tris, std::vector<Triangle_Chartid>& output_tris, bool contrain_edges);

    // node_id -> triangles
    std::map<uint32_t, std::vector<Triangle_Chartid>> triangles_map_;


#ifdef FLUSH_APP_STATE
  public:
    friend class boost::serialization::access;

    bvh() = default;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar& triangles_map_;
        ar& num_nodes_;
        ar& fan_factor_;
        ar& depth_;
        ar& primitives_per_node_;
        ar& size_of_primitive_;
        // ar& bounding_boxes_;
        ar& centroids_;
        ar& visibility_;
        ar& avg_primitive_extent_;
        ar& max_primitive_extent_deviation_;
        ar& filename_;
        ar& translation_;
        ar& primitive_;
    }
#endif
};

} // namespace mesh
} // namespace lamure

#endif