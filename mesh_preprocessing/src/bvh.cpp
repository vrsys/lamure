
#include <lamure/mesh/bvh.h>
#include <lamure/mesh/polyhedron.h>
#include <lamure/ren/lod_stream.h>
#include <lamure/mesh/tools.h>

#include <limits>
#include <queue>
#include <algorithm>
#include <iostream>

#include <CGAL/IO/print_wavefront.h>

// Stop-condition policy
#include <CGAL/Surface_mesh_simplification/Policies/Edge_collapse/Count_stop_predicate.h>
#include <CGAL/Surface_mesh_simplification/Policies/Edge_collapse/Count_ratio_stop_predicate.h>
#include <CGAL/Surface_mesh_simplification/Policies/Edge_collapse/Edge_length_cost.h>
#include <CGAL/Surface_mesh_simplification/Policies/Edge_collapse/Midpoint_placement.h>

#include <CGAL/Surface_mesh_simplification/Policies/Edge_collapse/LindstromTurk_cost.h>
#include <CGAL/Surface_mesh_simplification/Policies/Edge_collapse/LindstromTurk_placement.h>

// need update to use:
#include <CGAL/Surface_mesh_simplification/Policies/Edge_collapse/Bounded_normal_change_placement.h>

#include <CGAL/Surface_mesh_simplification/edge_collapse.h>

namespace SMS = CGAL::Surface_mesh_simplification;

namespace lamure
{
namespace mesh
{
bvh::bvh(std::vector<Triangle_Chartid>& triangles, uint32_t primitives_per_node) : lamure::ren::bvh()
{
    fan_factor_ = 2;
    size_of_primitive_ = sizeof(vertex);
    primitive_ = primitive_type::TRIMESH;
    primitives_per_node_ = primitives_per_node;

    create_hierarchy(triangles);
}

bvh::~bvh() {}

void bvh::create_hierarchy(std::vector<Triangle_Chartid>& triangles)
{
    // Determine the bounding box of all tris ---root
    vec3f min(std::numeric_limits<float>::max());
    vec3f max(std::numeric_limits<float>::lowest());

    for(const auto& tri : triangles)
    { // for each
        auto centroid = tri.get_centroid();
        if(centroid.x < min.x)
            min.x = centroid.x;
        if(centroid.y < min.y)
            min.y = centroid.y;
        if(centroid.z < min.z)
            min.z = centroid.z;

        if(centroid.x > max.x)
            max.x = centroid.x;
        if(centroid.y > max.y)
            max.y = centroid.y;
        if(centroid.z > max.z)
            max.z = centroid.z;
    }

    std::queue<bvh_node> q;
    bvh_node root{
        0, // depth
        min,
        max,
        0,               // begin
        triangles.size() // end
    };
    q.push(root);

    std::vector<bvh_node> nodes;

    // remember the root
    nodes.push_back(root);

    while(q.size() > 0)
    {
        bvh_node node = q.front(); // get the first element
        q.pop();                   // remove the first element

        // Determine longest  axis of the node
        vec3f extend = node.max_ - node.min_;
        int32_t axis = 0; // 0 = x axis, 1 = y axis, 2 = z axis
        if(extend.y > extend.x)
        {
            axis = 1;
            if(extend.z > extend.y)
            {
                axis = 2;
            }
        }
        else if(extend.z > extend.x)
        {
            axis = 2;
        }

        // Sort all tris (of current node) by their centroid along the longest axis
        std::sort(triangles.begin() + node.begin_, triangles.begin() + node.end_, [&](const Triangle_Chartid& a, const Triangle_Chartid& b) {
            if(axis == 0)
            {
                return a.get_centroid().x < b.get_centroid().x;
            }
            else if(axis == 1)
            {
                return a.get_centroid().y < b.get_centroid().y;
            }
            else if(axis == 2)
            {
                return a.get_centroid().z < b.get_centroid().z;
            }

            return true;
        });

        // determine split
        uint64_t split_id = (node.begin_ + node.end_) / 2;

        // first create the children

        bvh_node left_child{node.depth_ + 1, node.min_, node.max_, node.begin_, split_id};

        bvh_node right_child{node.depth_ + 1, node.min_, node.max_, split_id, node.end_};

        // determine bounds of both children
        if(axis == 0)
        {
            left_child.max_.x = triangles[split_id].get_centroid().x;
            right_child.min_.x = triangles[split_id + 1].get_centroid().x;
        }
        else if(axis == 1)
        {
            left_child.max_.y = triangles[split_id].get_centroid().y;
            right_child.min_.y = triangles[split_id + 1].get_centroid().y;
        }
        else if(axis == 2)
        {
            left_child.max_.z = triangles[split_id].get_centroid().z;
            right_child.min_.z = triangles[split_id + 1].get_centroid().z;
        }

        if(left_child.end_ - left_child.begin_ > primitives_per_node_ || right_child.end_ - right_child.begin_ > primitives_per_node_)
        {
            q.push(left_child);
            q.push(right_child);
        }

        nodes.push_back(left_child);
        nodes.push_back(right_child);

        if(left_child.depth_ > depth_)
        {
            std::cout << "depth: " << left_child.depth_ << " (+" << get_length_of_depth(left_child.depth_) << " nodes)" << std::endl;
        }
        depth_ = std::max(left_child.depth_, depth_);

    } // end of while

    std::cout << "Downsweep done" << std::endl;
    std::cout << "hierarchy depth: " << depth_ << std::endl;
    std::cout << "number of nodes: " << nodes.size() << std::endl;
    std::cout << "hierarchy min: " << nodes[0].min_ << std::endl;
    std::cout << "hierarchy max: " << nodes[0].max_ << std::endl;

    // populate the triangles map (but only from the leaf level)
    {
        uint32_t first_node = get_first_node_id_of_depth(depth_);
        uint32_t num_of_nodes = get_length_of_depth(depth_);

        // check the actual number of tris per node
        primitives_per_node_ = 0;
        for(uint32_t node_id = first_node; node_id < first_node + num_of_nodes; ++node_id)
        {
            bvh_node& node = nodes[node_id];
            primitives_per_node_ = std::max(primitives_per_node_, (uint32_t)(node.end_ - node.begin_));
        };

        std::cout << "actual triangles per node " << primitives_per_node_ << std::endl;

        for(uint32_t node_id = first_node; node_id < first_node + num_of_nodes; ++node_id)
        {
            bvh_node& node = nodes[node_id];

            // copy triangles to triangle map
            for(uint64_t tri = node.begin_; tri < node.end_; ++tri)
            {
                triangles_map_[node_id].push_back(triangles[tri]);
            }
        }
    }

    std::cout << "triangles map populated" << std::endl;

    // upsweep:
    // for each depth starting at (max depth)-1, depth--
    //  for each node at that depth
    //    take all triangles from the two children
    //    simplify these (half the number of triangles)

    uint32_t num_nodes_todo = 0;
    for(int d = depth_ - 1; d >= 0; d--)
    {
        num_nodes_todo += get_length_of_depth(d);
    }
    uint32_t num_nodes_done = 0;
    int prev_percent = -1;

    for(int d = depth_ - 1; d >= 0; d--)
    {
        uint32_t first_node = get_first_node_id_of_depth(d);
        uint32_t num_of_nodes = get_length_of_depth(d);

        std::vector<uint32_t> nodes_todo;
        for(uint32_t i = first_node; i < first_node + num_of_nodes; ++i)
        {
            nodes_todo.push_back(i);
        }

        // lambda for parallel version

        auto lambda_simplify = [&](uint64_t i, uint32_t id) -> void {
            uint32_t node_id = nodes_todo[i];

            int percent = (int)(((float)num_nodes_done / (float)num_nodes_todo) * 100.f);
            if(percent != prev_percent)
            {
                prev_percent = percent;
                std::cout << "Simplification: " << percent << " %" << std::endl;
            }

            uint32_t left_child = get_child_id(node_id, 0);
            uint32_t right_child = get_child_id(node_id, 1);

            // std::cout << "simplifying nodes " << left_child << " " << right_child << " into " << node_id << std::endl;

            // std::cout << "simplifying node " << node_id << " with constraint\n";
            // try simplification with edge constraint
            simplify(triangles_map_[left_child], triangles_map_[right_child], triangles_map_[node_id], true);

            if(triangles_map_[node_id].size() > primitives_per_node_)
            {
                // simplify without constraint
                std::cout << "simplifying node " << node_id << " without constraint\n";

                triangles_map_[node_id].clear();
                simplify(triangles_map_[left_child], triangles_map_[right_child], triangles_map_[node_id], false);
            }

            if(triangles_map_[node_id].size() > primitives_per_node_)
            {
                std::cout << "WARNING! @node_id " << node_id << " : simplified: " << triangles_map_[node_id].size() << " / desired: " << primitives_per_node_ << std::endl;
            }

            num_nodes_done++;
        };

        uint32_t num_threads = std::min((size_t)24, nodes_todo.size());
        lamure::mesh::parallel_for(num_threads, nodes_todo.size(), lambda_simplify);
    }

    std::cout << "Upsweep done." << std::endl;

    for(uint32_t node_id = 0; node_id < nodes.size(); ++node_id)
    {
        auto& node = nodes[node_id];

        if(triangles_map_[node_id].size() > primitives_per_node_)
        {
            std::cout << "WARNING: (" << node_id << ": " << triangles_map_[node_id].size() << ") removing \
      " << triangles_map_[node_id].size() - primitives_per_node_
                      << " triangles manually to stay on budget: " << primitives_per_node_ << std::endl;
        }

        // if we have too many, remove some
        while(triangles_map_[node_id].size() > primitives_per_node_)
        {
            triangles_map_[node_id].pop_back();
        }

        visibility_.push_back(node_visibility::NODE_VISIBLE);

        float avg_primitive_extent = 0;
        float max_primitive_extent_deviation = 0;

        // recompute bounding box
        node.min_ = scm::math::vec3f(std::numeric_limits<float>::max());
        node.max_ = scm::math::vec3f(std::numeric_limits<float>::lowest());

        for(auto& tri : triangles_map_[node_id])
        {
            avg_primitive_extent += tri.get_area();
            max_primitive_extent_deviation = std::max(tri.get_area(), max_primitive_extent_deviation);

            if(tri.v0_.pos_.x < node.min_.x)
                node.min_.x = tri.v0_.pos_.x;
            if(tri.v0_.pos_.y < node.min_.y)
                node.min_.y = tri.v0_.pos_.y;
            if(tri.v0_.pos_.z < node.min_.z)
                node.min_.z = tri.v0_.pos_.z;

            if(tri.v0_.pos_.x > node.max_.x)
                node.max_.x = tri.v0_.pos_.x;
            if(tri.v0_.pos_.y > node.max_.y)
                node.max_.y = tri.v0_.pos_.y;
            if(tri.v0_.pos_.z > node.max_.z)
                node.max_.z = tri.v0_.pos_.z;

            if(tri.v1_.pos_.x < node.min_.x)
                node.min_.x = tri.v1_.pos_.x;
            if(tri.v1_.pos_.y < node.min_.y)
                node.min_.y = tri.v1_.pos_.y;
            if(tri.v1_.pos_.z < node.min_.z)
                node.min_.z = tri.v1_.pos_.z;

            if(tri.v1_.pos_.x > node.max_.x)
                node.max_.x = tri.v1_.pos_.x;
            if(tri.v1_.pos_.y > node.max_.y)
                node.max_.y = tri.v1_.pos_.y;
            if(tri.v1_.pos_.z > node.max_.z)
                node.max_.z = tri.v1_.pos_.z;

            if(tri.v2_.pos_.x < node.min_.x)
                node.min_.x = tri.v2_.pos_.x;
            if(tri.v2_.pos_.y < node.min_.y)
                node.min_.y = tri.v2_.pos_.y;
            if(tri.v2_.pos_.z < node.min_.z)
                node.min_.z = tri.v2_.pos_.z;

            if(tri.v2_.pos_.x > node.max_.x)
                node.max_.x = tri.v2_.pos_.x;
            if(tri.v2_.pos_.y > node.max_.y)
                node.max_.y = tri.v2_.pos_.y;
            if(tri.v2_.pos_.z > node.max_.z)
                node.max_.z = tri.v2_.pos_.z;

            auto normal = tri.get_normal();
            tri.v0_.nml_ = normal;
            tri.v1_.nml_ = normal;
            tri.v2_.nml_ = normal;
        }

        bounding_boxes_.push_back(scm::gl::boxf(node.min_, node.max_));

        centroids_.push_back(vec3f(node.min_ + node.max_) * 0.5f);

        avg_primitive_extent /= (float)triangles_map_[node_id].size();
        avg_primitive_extent_.push_back(std::max((1.f / (get_depth_of_node(node_id) + 1)) * 0.1f, 10.f * avg_primitive_extent));
        max_primitive_extent_deviation_.push_back(max_primitive_extent_deviation);

        // if the number of triangles was not divisible by two, add another tri for padding
        while(triangles_map_[node_id].size() < primitives_per_node_)
        {
            triangles_map_[node_id].push_back(Triangle_Chartid());
        }
    }

    primitives_per_node_ *= 3;

    num_nodes_ = nodes.size();

    nodes.clear();
}

void bvh::simplify(std::vector<Triangle_Chartid>& left_child_tris, std::vector<Triangle_Chartid>& right_child_tris, std::vector<Triangle_Chartid>& output_tris, bool contrain_edges)
{
    // concatenate tri sets
    std::vector<Triangle_Chartid> combined_set;
    std::copy(left_child_tris.begin(), left_child_tris.end(), std::back_inserter(combined_set));
    std::copy(right_child_tris.begin(), right_child_tris.end(), std::back_inserter(combined_set));

    // create a mesh from vectors
    Polyhedron polyMesh;
    polyhedron_builder<HalfedgeDS> builder(combined_set);
    polyMesh.delegate(builder);

    if(polyMesh.is_valid(false) && CGAL::is_triangle_mesh(polyMesh))
    {
    }
    else
    {
        std::cout << "WARNING! Triangle mesh invalid! (No triangles saved for this node)" << std::endl;
        return;
    }

    // if mesh is valid, add some info about the faces
    uint32_t i = 0;
    for(Polyhedron::Facet_iterator f = polyMesh.facets_begin(); f != polyMesh.facets_end(); ++f)
    {
        // add tex coords to face
        TexCoord tc0(combined_set[i].v0_.tex_.x, combined_set[i].v0_.tex_.y);
        TexCoord tc1(combined_set[i].v1_.tex_.x, combined_set[i].v1_.tex_.y);
        TexCoord tc2(combined_set[i].v2_.tex_.x, combined_set[i].v2_.tex_.y);
        f->add_tex_coords(tc0, tc1, tc2);

        // add face id that is position of triangle in input triangle list
        f->face_id = i;
        // transfer chart
        f->chart_id = combined_set[i].chart_id;
        // transfer tex id
        f->tex_id = combined_set[i].tex_id;
        f->area_id = combined_set[i].area_id;
        f->tri_id = combined_set[i].tri_id;

        i++;
    }
    i = 0;
    for(lamure::mesh::Polyhedron::Halfedge_iterator e = polyMesh.edges_begin(); e != polyMesh.edges_end(); ++e)
    {
        e->edge_id = i;
        i++;
    }

    // simplification with borders constrained
    Border_is_constrained_edge_map bem(polyMesh);

    typedef SMS::Bounded_normal_change_placement<SMS::LindstromTurk_placement<Polyhedron>> Placement;

    if(contrain_edges)
    {
        SMS::Count_ratio_stop_predicate<Polyhedron> stop(0.5f);

        SMS::edge_collapse(polyMesh,
                           stop,
                           CGAL::parameters::vertex_index_map(get(CGAL::vertex_external_index, polyMesh))
                               .halfedge_index_map(get(CGAL::halfedge_external_index, polyMesh))
                               .edge_is_constrained_map(bem)
                               .get_cost(SMS::LindstromTurk_cost<Polyhedron>())
                               .get_placement(Placement()));
    }
    else
    {
        // without border constraint

        SMS::Count_ratio_stop_predicate<Polyhedron> stop(0.5f);
        SMS::edge_collapse(polyMesh,
                           stop,
                           CGAL::parameters::vertex_index_map(get(CGAL::vertex_external_index, polyMesh))
                               .halfedge_index_map(get(CGAL::halfedge_external_index, polyMesh))
                               .get_cost(SMS::Edge_length_cost<Polyhedron>())
                               //.get_cost (SMS::LindstromTurk_cost<Polyhedron>())
                               .get_placement(SMS::Midpoint_placement<Polyhedron>()));
    }

    output_tris.clear();

    // convert back to triangle soup
    uint32_t num_vertices_simplified = 0;
    for(Polyhedron::Facet_iterator f = polyMesh.facets_begin(); f != polyMesh.facets_end(); ++f)
    {
        Polyhedron::Halfedge_around_facet_circulator c = f->facet_begin();

        Triangle_Chartid tri;
        tri.chart_id = f->chart_id;
        tri.tex_id = f->tex_id;
        tri.area_id = f->area_id;
        tri.tri_id = f->tri_id;

        for(int i = 0; i < 3; ++i, ++c)
        {
            // TODO take care of / generate normals
            // maybe generate because per face normals will be rendered incorrect during simplification

            switch(i)
            {
            case 0:
                tri.v0_.pos_ = vec3f(c->vertex()->point()[0], c->vertex()->point()[1], c->vertex()->point()[2]);
                tri.v0_.tex_ = vec2f(c->vertex()->point().get_u(), c->vertex()->point().get_v());
                break;

            case 1:
                tri.v1_.pos_ = vec3f(c->vertex()->point()[0], c->vertex()->point()[1], c->vertex()->point()[2]);
                tri.v1_.tex_ = vec2f(c->vertex()->point().get_u(), c->vertex()->point().get_v());
                break;

            case 2:
                tri.v2_.pos_ = vec3f(c->vertex()->point()[0], c->vertex()->point()[1], c->vertex()->point()[2]);
                tri.v2_.tex_ = vec2f(c->vertex()->point().get_u(), c->vertex()->point().get_v());
                break;

            default:
                break;
            }

            ++num_vertices_simplified;
        }

        output_tris.push_back(tri);
    }
}

void bvh::write_lod_file(const std::string& lod_filename)
{
    // convert triangle map with chart id to trianglke map without chart id
    std::map<uint32_t, std::vector<triangle_t>> simple_triangles_map;

    for(auto& node_tri_list : triangles_map_) // for each node
    {
        std::vector<triangle_t> tris; // container for new tris

        for(auto& t : node_tri_list.second) // for each triangle
        {
            lamure::mesh::triangle_t new_tri = t.get_basic_triangle();

            tris.push_back(new_tri);
        }
        simple_triangles_map[node_tri_list.first] = tris;
    }

    auto lod = std::make_shared<lamure::ren::lod_stream>();
    lod->open_for_writing(lod_filename);

    for(uint32_t node_id = 0; node_id < num_nodes_; ++node_id)
    {
        size_t length_in_bytes = primitives_per_node_ * sizeof(vertex);
        size_t start_in_file = node_id * length_in_bytes;
        lod->write((char*)&simple_triangles_map[node_id][0], start_in_file, length_in_bytes);
    }

    lod->close();
}

} // namespace mesh
} // namespace lamure
