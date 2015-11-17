// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include "tree_modifier.h"

#include <iostream>
#include <scm/gl_core/math/mat4_gl.h>

#include <lamure/pre/serialized_surfel.h>
#include <lamure/pre/node_serializer.h>
#include <lamure/types.h>
#include <lamure/utils.h>
#include <unordered_map>
#include <set>

#include "HistogramMatcher.h"

#define SIMPLE_HIST_MATCHING 1

namespace {

// Obtain a matrix for transforming points of bvh_a to the space of bvh_b
lamure::mat4r
get_frame_transform(const CollisionDetector::Object& bvh_a,
                    const CollisionDetector::Object& bvh_b)
{
    auto inv_bvh_b_mat = scm::math::inverse(bvh_b.second);
    auto inv_hidden_bvh_b_mat = scm::math::inverse(scm::math::make_translation(bvh_b.first->translation()));
    auto bvh_a_mat = bvh_a.second;
    auto hidden_bvh_a_mat = scm::math::make_translation(bvh_a.first->translation());
    
    return inv_hidden_bvh_b_mat * inv_bvh_b_mat * bvh_a_mat * hidden_bvh_a_mat;

    //return scm::math::inverse(bvh_b.second * scm::math::make_translation(bvh_b.first->translation()))
    //       * (bvh_a.second * scm::math::make_translation(bvh_a.first->translation()));
}

}


CollisionDetector::
CollisionDetector(const Object& bvh_l,
                  const Object& bvh_r,
                  int stop_level_a,
                  int stop_level_b)
        : bvh_l_(bvh_l),
          bvh_r_(bvh_r)
{
    //const lamure::mat4r frame_trans = scm::math::inverse(bvh_l_.second) * bvh_r_.second;
    const lamure::mat4r frame_trans = get_frame_transform(bvh_r, bvh_l);

    auto& nd = bvh_r_.first->nodes();
    for(size_t i = 0; i < nd.size(); ++i) {
        lamure::BoundingBox new_box;
        auto& nb = nd[i].bounding_box();

#if 1
        // extend bboxes
        nb.min().z = bvh_r_.first->nodes()[0].bounding_box().min().z;
        nb.max().z = bvh_r_.first->nodes()[0].bounding_box().max().z;
#endif
        new_box.Expand(frame_trans * nb.min());
        new_box.Expand(frame_trans * lamure::vec3r(nb.min().x, nb.min().y, nb.max().z));
        new_box.Expand(frame_trans * lamure::vec3r(nb.min().x, nb.max().y, nb.min().z));
        new_box.Expand(frame_trans * lamure::vec3r(nb.min().x, nb.max().y, nb.max().z));

        new_box.Expand(frame_trans * nb.max());
        new_box.Expand(frame_trans * lamure::vec3r(nb.max().x, nb.max().y, nb.min().z));
        new_box.Expand(frame_trans * lamure::vec3r(nb.max().x, nb.min().y, nb.max().z));
        new_box.Expand(frame_trans * lamure::vec3r(nb.max().x, nb.min().y, nb.min().z));
 
        boxes_.push_back(new_box);
    }

    stop_level_a_ = (stop_level_a >= 0) ? stop_level_a : int(bvh_l_.first->depth()) + 1 + stop_level_a;
    stop_level_b_ = (stop_level_b >= 0) ? stop_level_b : int(bvh_r_.first->depth()) + 1 + stop_level_b;

    stop_level_a_ = std::max(std::min(stop_level_a_, int(bvh_l_.first->depth())), 0);
    stop_level_b_ = std::max(std::min(stop_level_b_, int(bvh_r_.first->depth())), 0);

}

void CollisionDetector::
SearchIntersections(const CallbackFunc& callback) const
{
    Traverse(0, 0, callback);
}

void CollisionDetector::
Traverse(lamure::NodeIdType a, lamure::NodeIdType b, const CallbackFunc& callback) const
{
    
    const auto& tr_a = bvh_l_.first;
    const auto& tr_b = bvh_r_.first;

    if (!tr_a->nodes()[a].bounding_box().Intersects(boxes_[b]))
        return;

    const bool is_leaf_a = int(tr_a->nodes()[a].depth()) >= stop_level_a_;
    const bool is_leaf_b = int(tr_b->nodes()[b].depth()) >= stop_level_b_;

    if (is_leaf_a && is_leaf_b) {
        if (callback)
            callback(a, b);
    }
    else {
        std::vector<lamure::NodeIdType> children_a, children_b;

        if (is_leaf_a)
            children_a.push_back(a);
        else {
            for (unsigned i = 0; i < tr_a->fan_factor(); ++i)
                children_a.push_back(tr_a->GetChildId(a, i));
        }
        if (is_leaf_b)
            children_b.push_back(b);
        else {
            for (unsigned i = 0; i < tr_b->fan_factor(); ++i)
                children_b.push_back(tr_b->GetChildId(b, i));
        }

        for (const auto& ca: children_a)
            for (const auto& cb: children_b)
                Traverse(ca, cb, callback);
    }
}

void TreeModifier::
ComplementOnFirstTree(int relax_levels)
{
    using namespace lamure;

    CollisionDetector cdet(bvhs_[0], bvhs_[1], -1, -relax_levels - 1);

    //attempt trivial reject
    
    {
    const auto& _bb0 = bvhs_[0].first->nodes()[0].bounding_box();
    const auto& _bb1 = cdet.boxes()[0];

    std::cout << "bounding box frame:" << std::endl;
    std::cout << "bb0 min: " << _bb0.min() << std::endl;
    std::cout << "bb0 max: " << _bb0.max() << std::endl;
    std::cout << "bb1 min: " << _bb1.min() << std::endl;
    std::cout << "bb1 max: " << _bb1.max() << std::endl;

    if (_bb0.max().x < _bb1.min().x) {
       std::cout << "trivial reject" << std::endl;
       return;
    }
    if (_bb0.min().x > _bb1.max().x) {
       std::cout << "trivial reject" << std::endl;
       return;
    }
    if (_bb0.max().y < _bb1.min().y) {
       std::cout << "trivial reject" << std::endl;
       return;
    }
    if (_bb0.min().y > _bb1.max().y) {
       std::cout << "trivial reject" << std::endl;
       return;
    }
    if (_bb0.max().z < _bb1.min().z) {
       std::cout << "trivial reject" << std::endl;
       return;
    }
    if (_bb0.min().z > _bb1.max().z) {
       std::cout << "trivial reject" << std::endl;
       return;
    }
    }

    std::cout << "no rejection possible... " << std::endl;
    //return;
    

    const auto& tr_a = bvhs_[0].first;
    const auto& tr_b = bvhs_[1].first;

    const lamure::mat4r frame_trans = get_frame_transform(bvhs_[0], bvhs_[1]);

    pre::NodeSerializer ser_a(tr_a->max_surfels_per_node(), 0);
    ser_a.Open(AddToPath(tr_a->base_path(), ".lod").string(), true);

    pre::SurfelVector surfels;
    size_t pairs = 0, total_discarded = 0;

    std::cout << "processing..." << std::flush;

    /*cdet.SearchIntersections([&](lamure::NodeIdType a, lamure::NodeIdType b) {
        lamure::NodeIdType nid_b = b;
        for (int i = 0; i < relax_levels; ++i)
            nid_b = tr_b->GetParentId(nid_b);

        const auto& bbox = tr_b->nodes()[nid_b].bounding_box();

        lamure::NodeIdType nid_a = a;
        while (true) {
            ser_a.ReadNodeImmediate(surfels, nid_a);
            bool discarded = false;
            for (auto& s: surfels) {
                // check surfel in A intersects with one of the AABB of B
                if (bbox.Contains(frame_trans * s.pos()) && s.radius() > 0.f) {
                    discarded = true;
                    s.radius() = 0.0;
                    ++total_discarded;
                }
            }
            if (discarded)
                ser_a.WriteNodeImmediate(surfels, nid_a);
            if (nid_a == 0)
                break;
            nid_a = tr_a->GetParentId(nid_a);
        }
        std::cout << "\r" << ++pairs << " collision pairs processed" << std::flush;
    });*/

    std::unordered_map<lamure::NodeIdType, std::set<lamure::NodeIdType>> collision_info;

    cdet.SearchIntersections([&](lamure::NodeIdType a, lamure::NodeIdType b) {
        lamure::NodeIdType nid_b = b;
        
        //std::cout << "node id " << nid_b << std::endl;
        
        // traverse to top
        lamure::NodeIdType nid_a = a;
        while (true) {
            collision_info[nid_a].insert(nid_b);
            if (nid_a == 0) break;
            nid_a = tr_a->GetParentId(nid_a);
        }
        // traverse to bottom
        if (++pairs % 1500 == 0)
            std::cout << "\r" << pairs << " collision pairs processed" << std::flush;
    });

    std::cout << "\r" << pairs << " collision pairs processed" << std::endl;

    pairs = 0;

    
    for (const auto& col: collision_info) {
        lamure::NodeIdType a = col.first;
        bool changed = false;
        ser_a.ReadNodeImmediate(surfels, a);

/*

        #pragma omp parallel for
        for (size_t spos = 0; spos < surfels.size(); ++spos) {
//        for (auto& s: surfels) {
          auto& s = surfels[spos];
//            for (const auto& b : col.second) {
          if (s.radius() > 0.f) {
            for (lamure::NodeIdType b = tr_b->first_leaf(); b < tr_b->nodes().size(); ++b) {
              auto bbox = tr_b->nodes()[b].bounding_box();
              bbox.min().z = tr_b->nodes()[0].bounding_box().min().z;
              bbox.max().z = tr_b->nodes()[0].bounding_box().max().z;
              if (bbox.Contains(frame_trans * s.pos())) {
                changed = true;
                s.radius() = 0.0;
                #pragma omp atomic
                ++total_discarded;
                break;
              }
            }
          }
        }
*/
        #pragma omp parallel for
        for (size_t spos = 0; spos < surfels.size(); ++spos) {
          auto& s = surfels[spos];
          if (s.radius() > 0.0) {
            for (const auto& b : col.second) {
              auto bbox = tr_b->nodes()[b].bounding_box();
              if (bbox.Contains(frame_trans * s.pos())) {
                changed = true;
                s.radius() = 0.0;
                #pragma omp atomic
                ++total_discarded;
                break;
              }
            }
          }
        }

        if (changed)
            ser_a.WriteNodeImmediate(surfels, a);

        if (++pairs % 500 == 0)
            std::cout << "\r" << pairs << " / " << collision_info.size() << " nodes processed; discarded: " << total_discarded << std::flush;
    }
    std::cout << "\r" << pairs << " nodes processed" << std::flush;

    ser_a.Close();
    std::cout << std::endl << "Surfels discarded: " << total_discarded << std::endl;
}

void TreeModifier::
HistMatchSecondTree()
{
    using namespace lamure;

    // init serializers
    std::vector<std::shared_ptr<pre::NodeSerializer>> ser;
    for (auto& tr : bvhs_) {
        ser.push_back(std::make_shared<pre::NodeSerializer>(tr.first->max_surfels_per_node(), 0));
        ser.back()->Open(AddToPath(tr.first->base_path(), ".lod").string(), true);
    }

    std::set<lamure::NodeIdType> collision_info;
    size_t ctr = 0;

    // collect collisions
    for (size_t i = 1; i < bvhs_.size(); ++i) {
        CollisionDetector cdet(bvhs_[0], bvhs_[i], -1, -1);
        cdet.SearchIntersections([&](lamure::NodeIdType a, lamure::NodeIdType b) {
                                 collision_info.insert(a);
                                 ++ctr;
                                 });
    }
    std::cout << ctr << " collision pairs processed" << std::endl;

    HistogramMatcher matcher;
    // get reference colors
    {
        HistogramMatcher::ColorArray ref_colors;
        pre::SurfelVector surfels;
        for (const auto& col: collision_info) {
            ser[0]->ReadNodeImmediate(surfels, col);
            for (auto& s: surfels) {
                if (s != pre::Surfel())
                    ref_colors.AddColor(s.color());
            }
        }
        std::cout << "Collected reference colors from " << collision_info.size() << " nodes" << std::endl;

        matcher.InitReference(ref_colors);
    }
    // matching

    for (size_t tid = 1; tid < bvhs_.size(); ++ tid) {
        auto& tr = bvhs_[tid].first;
        pre::SurfelVector surfels;
        HistogramMatcher::ColorArray colors;

        ctr = 0; 
        for (unsigned i = 0; i <= tr->depth(); ++i) {
            auto ranges = tr->GetNodeRanges(i);
            colors.Clear();
            // get colors
            for (lamure::NodeIdType j = 0; j < ranges.second; ++j) {
                ser[tid]->ReadNodeImmediate(surfels, j + ranges.first);
                for (auto& s: surfels) {
                    if (s != pre::Surfel())
                        colors.AddColor(s.color());
                }
            }
            matcher.Match(colors);

            // write colors back
            size_t surfel_ctr = 0;
            for (lamure::NodeIdType j = 0; j < ranges.second; ++j) {
                ser[tid]->ReadNodeImmediate(surfels, j + ranges.first);
                for (auto& s: surfels) {
                    if (s != pre::Surfel()) {
                        vec3b c(colors.r[surfel_ctr], 
                                colors.g[surfel_ctr], 
                                colors.b[surfel_ctr]);
                        s.color() = c;
                        ++surfel_ctr;
                    }
                }
                ser[tid]->WriteNodeImmediate(surfels, j + ranges.first);
            }
            std::cout << "\r" << ctr++ << " / " << tr->depth() << " levels processed" << std::flush;
        }
        std::cout << ". Tree " << tid << " finished" << std::endl;
    }
}

void TreeModifier::
MultRadii(lamure::real factor)
{
    using namespace lamure;
    pre::SurfelVector surfels;
    for (size_t tid = 0; tid < bvhs_.size(); ++ tid) {
        auto& tr = bvhs_[tid].first;
        size_t ctr{};

        pre::NodeSerializer ser(tr->max_surfels_per_node(), 0);
        ser.Open(AddToPath(tr->base_path(), ".lod").string(), true);

        for (lamure::NodeIdType i = 0; i < tr->nodes().size(); ++i) {
            ser.ReadNodeImmediate(surfels, i);
            for (auto& s: surfels) {
                if (s != pre::Surfel() && s.radius() != 0.f)
                    s.radius() *= factor;
            }
            ser.WriteNodeImmediate(surfels, i);
            tr->nodes()[i].set_avg_surfel_radius(tr->nodes()[i].avg_surfel_radius() * factor);
            if (++ctr % 500 == 0)
                std::cout << "\r" << int(float(ctr)/tr->nodes().size()*100) << "\% processed" << std::flush;
        }

        tr->SerializeTreeToFile(AddToPath(tr->base_path(), ".bvh").string(), false);

        std::cout << ". Tree " << tid << " finished" << std::endl;
    }

}

lamure::real TreeModifier::
ComputeAvgRadius(const lamure::pre::Bvh& bvh, unsigned depth) const
{
    lamure::real avg_surfel = 0.0;
    auto ranges = bvh.GetNodeRanges(depth);

    for (lamure::NodeIdType j = 0; j < ranges.second; ++j)
        avg_surfel += bvh.nodes()[j + ranges.first].avg_surfel_radius();
    avg_surfel /= ranges.second;

    return avg_surfel;
}
