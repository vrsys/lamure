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
get_frame_transform(const collision_detector::object& bvh_a,
                    const collision_detector::object& bvh_b)
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


collision_detector::
collision_detector(const object& bvh_l,
                  const object& bvh_r,
                  int stop_level_a,
                  int stop_level_b)
        : bvh_l_(bvh_l),
          bvh_r_(bvh_r)
{
    //const lamure::mat4r frame_trans = scm::math::inverse(bvh_l_.second) * bvh_r_.second;
    const lamure::mat4r frame_trans = get_frame_transform(bvh_r, bvh_l);

    auto& nd = bvh_r_.first->nodes();
    for(size_t i = 0; i < nd.size(); ++i) {
        lamure::bounding_box new_box;
        auto& nb = nd[i].get_bounding_box();

#if 1
        // extend bboxes
        nb.min().z = bvh_r_.first->nodes()[0].get_bounding_box().min().z;
        nb.max().z = bvh_r_.first->nodes()[0].get_bounding_box().max().z;
#endif
        new_box.expand(frame_trans * nb.min());
        new_box.expand(frame_trans * lamure::vec3r(nb.min().x, nb.min().y, nb.max().z));
        new_box.expand(frame_trans * lamure::vec3r(nb.min().x, nb.max().y, nb.min().z));
        new_box.expand(frame_trans * lamure::vec3r(nb.min().x, nb.max().y, nb.max().z));

        new_box.expand(frame_trans * nb.max());
        new_box.expand(frame_trans * lamure::vec3r(nb.max().x, nb.max().y, nb.min().z));
        new_box.expand(frame_trans * lamure::vec3r(nb.max().x, nb.min().y, nb.max().z));
        new_box.expand(frame_trans * lamure::vec3r(nb.max().x, nb.min().y, nb.min().z));
 
        boxes_.push_back(new_box);
    }

    stop_level_a_ = (stop_level_a >= 0) ? stop_level_a : int(bvh_l_.first->depth()) + 1 + stop_level_a;
    stop_level_b_ = (stop_level_b >= 0) ? stop_level_b : int(bvh_r_.first->depth()) + 1 + stop_level_b;

    stop_level_a_ = std::max(std::min(stop_level_a_, int(bvh_l_.first->depth())), 0);
    stop_level_b_ = std::max(std::min(stop_level_b_, int(bvh_r_.first->depth())), 0);

}

void collision_detector::
search_intersections(const callback_func& callback) const
{
    traverse(0, 0, callback);
}

void collision_detector::
traverse(lamure::node_id_type a, lamure::node_id_type b, const callback_func& callback) const
{
    
    const auto& tr_a = bvh_l_.first;
    const auto& tr_b = bvh_r_.first;

    if (!tr_a->nodes()[a].get_bounding_box().intersects(boxes_[b]))
        return;

    const bool is_leaf_a = int(tr_a->nodes()[a].depth()) >= stop_level_a_;
    const bool is_leaf_b = int(tr_b->nodes()[b].depth()) >= stop_level_b_;

    if (is_leaf_a && is_leaf_b) {
        if (callback)
            callback(a, b);
    }
    else {
        std::vector<lamure::node_id_type> children_a, children_b;

        if (is_leaf_a)
            children_a.push_back(a);
        else {
            for (unsigned i = 0; i < tr_a->fan_factor(); ++i)
                children_a.push_back(tr_a->get_child_id(a, i));
        }
        if (is_leaf_b)
            children_b.push_back(b);
        else {
            for (unsigned i = 0; i < tr_b->fan_factor(); ++i)
                children_b.push_back(tr_b->get_child_id(b, i));
        }

        for (const auto& ca: children_a)
            for (const auto& cb: children_b)
                traverse(ca, cb, callback);
    }
}

void TreeModifier::
complementOnFirstTree(int relax_levels)
{
    using namespace lamure;

    collision_detector cdet(bvhs_[0], bvhs_[1], -1, -relax_levels - 1);

    //attempt trivial reject
    
    {
    const auto& _bb0 = bvhs_[0].first->nodes()[0].get_bounding_box();
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

    pre::node_serializer ser_a(tr_a->max_surfels_per_node(), 0);
    ser_a.open(add_to_path(tr_a->base_path(), ".lod").string(), true);

    pre::surfel_vector surfels;
    size_t pairs = 0, total_discarded = 0;

    std::cout << "processing..." << std::flush;

    /*cdet.search_intersections([&](lamure::node_id_type a, lamure::node_id_type b) {
        lamure::node_id_type nid_b = b;
        for (int i = 0; i < relax_levels; ++i)
            nid_b = tr_b->get_parent_id(nid_b);

        const auto& bbox = tr_b->nodes()[nid_b].get_bounding_box();

        lamure::node_id_type nid_a = a;
        while (true) {
            ser_a.read_node_immediate(surfels, nid_a);
            bool discarded = false;
            for (auto& s: surfels) {
                // check surfel in A intersects with one of the AABB of B
                if (bbox.contains(frame_trans * s.pos()) && s.radius() > 0.f) {
                    discarded = true;
                    s.radius() = 0.0;
                    ++total_discarded;
                }
            }
            if (discarded)
                ser_a.write_node_immediate(surfels, nid_a);
            if (nid_a == 0)
                break;
            nid_a = tr_a->get_parent_id(nid_a);
        }
        std::cout << "\r" << ++pairs << " collision pairs processed" << std::flush;
    });*/

    std::unordered_map<lamure::node_id_type, std::set<lamure::node_id_type>> collision_info;

    cdet.search_intersections([&](lamure::node_id_type a, lamure::node_id_type b) {
        lamure::node_id_type nid_b = b;
        
        //std::cout << "node id " << nid_b << std::endl;
        
        // traverse to top
        lamure::node_id_type nid_a = a;
        while (true) {
            collision_info[nid_a].insert(nid_b);
            if (nid_a == 0) break;
            nid_a = tr_a->get_parent_id(nid_a);
        }
        // traverse to bottom
        if (++pairs % 1500 == 0)
            std::cout << "\r" << pairs << " collision pairs processed" << std::flush;
    });

    std::cout << "\r" << pairs << " collision pairs processed" << std::endl;

    pairs = 0;

    
    for (const auto& col: collision_info) {
        lamure::node_id_type a = col.first;
        bool changed = false;
        ser_a.read_node_immediate(surfels, a);

/*

        #pragma omp parallel for
        for (size_t spos = 0; spos < surfels.size(); ++spos) {
//        for (auto& s: surfels) {
          auto& s = surfels[spos];
//            for (const auto& b : col.second) {
          if (s.radius() > 0.f) {
            for (lamure::node_id_type b = tr_b->first_leaf(); b < tr_b->nodes().size(); ++b) {
              auto bbox = tr_b->nodes()[b].get_bounding_box();
              bbox.min().z = tr_b->nodes()[0].get_bounding_box().min().z;
              bbox.max().z = tr_b->nodes()[0].get_bounding_box().max().z;
              if (bbox.contains(frame_trans * s.pos())) {
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
              auto bbox = tr_b->nodes()[b].get_bounding_box();
              if (bbox.contains(frame_trans * s.pos())) {
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
            ser_a.write_node_immediate(surfels, a);

        if (++pairs % 500 == 0)
            std::cout << "\r" << pairs << " / " << collision_info.size() << " nodes processed; discarded: " << total_discarded << std::flush;
    }
    std::cout << "\r" << pairs << " nodes processed" << std::flush;

    ser_a.close();
    std::cout << std::endl << "surfels discarded: " << total_discarded << std::endl;
}

void TreeModifier::
histogrammatchSecondTree()
{
    using namespace lamure;

    // init serializers
    std::vector<std::shared_ptr<pre::node_serializer>> ser;
    for (auto& tr : bvhs_) {
        ser.push_back(std::make_shared<pre::node_serializer>(tr.first->max_surfels_per_node(), 0));
        ser.back()->open(add_to_path(tr.first->base_path(), ".lod").string(), true);
    }

    std::set<lamure::node_id_type> collision_info;
    size_t ctr = 0;

    // collect collisions
    for (size_t i = 1; i < bvhs_.size(); ++i) {
        collision_detector cdet(bvhs_[0], bvhs_[i], -1, -1);
        cdet.search_intersections([&](lamure::node_id_type a, lamure::node_id_type b) {
                                 collision_info.insert(a);
                                 ++ctr;
                                 });
    }
    std::cout << ctr << " collision pairs processed" << std::endl;

    histogram_matcher matcher;
    // get reference colors
    {
        histogram_matcher::color_array ref_colors;
        pre::surfel_vector surfels;
        for (const auto& col: collision_info) {
            ser[0]->read_node_immediate(surfels, col);
            for (auto& s: surfels) {
                if (s != pre::surfel())
                    ref_colors.add_color(s.color());
            }
        }
        std::cout << "Collected reference colors from " << collision_info.size() << " nodes" << std::endl;

        matcher.init_reference(ref_colors);
    }
    // matching

    for (size_t tid = 1; tid < bvhs_.size(); ++ tid) {
        auto& tr = bvhs_[tid].first;
        pre::surfel_vector surfels;
        histogram_matcher::color_array colors;

        ctr = 0; 
        for (unsigned i = 0; i <= tr->depth(); ++i) {
            auto ranges = tr->get_node_ranges(i);
            colors.clear();
            // get colors
            for (lamure::node_id_type j = 0; j < ranges.second; ++j) {
                ser[tid]->read_node_immediate(surfels, j + ranges.first);
                for (auto& s: surfels) {
                    if (s != pre::surfel())
                        colors.add_color(s.color());
                }
            }
            matcher.match(colors);

            // write colors back
            size_t surfel_ctr = 0;
            for (lamure::node_id_type j = 0; j < ranges.second; ++j) {
                ser[tid]->read_node_immediate(surfels, j + ranges.first);
                for (auto& s: surfels) {
                    if (s != pre::surfel()) {
                        vec3b c(colors.r[surfel_ctr], 
                                colors.g[surfel_ctr], 
                                colors.b[surfel_ctr]);
                        s.color() = c;
                        ++surfel_ctr;
                    }
                }
                ser[tid]->write_node_immediate(surfels, j + ranges.first);
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
    pre::surfel_vector surfels;
    for (size_t tid = 0; tid < bvhs_.size(); ++ tid) {
        auto& tr = bvhs_[tid].first;
        size_t ctr{};

        pre::node_serializer ser(tr->max_surfels_per_node(), 0);
        ser.open(add_to_path(tr->base_path(), ".lod").string(), true);

        for (lamure::node_id_type i = 0; i < tr->nodes().size(); ++i) {
            ser.read_node_immediate(surfels, i);
            for (auto& s: surfels) {
                if (s != pre::surfel() && s.radius() != 0.f)
                    s.radius() *= factor;
            }
            ser.write_node_immediate(surfels, i);
            tr->nodes()[i].set_avg_surfel_radius(tr->nodes()[i].avg_surfel_radius() * factor);
            if (++ctr % 500 == 0)
                std::cout << "\r" << int(float(ctr)/tr->nodes().size()*100) << " % processed" << std::flush;
        }

        tr->serialize_tree_to_file(add_to_path(tr->base_path(), ".bvh").string(), false);

        std::cout << ". Tree " << tid << " finished" << std::endl;
    }

}

lamure::real TreeModifier::
computeAvgRadius(const lamure::pre::bvh& bvh, unsigned depth) const
{
    lamure::real avg_surfel = 0.0;
    auto ranges = bvh.get_node_ranges(depth);

    for (lamure::node_id_type j = 0; j < ranges.second; ++j)
        avg_surfel += bvh.nodes()[j + ranges.first].avg_surfel_radius();
    avg_surfel /= ranges.second;

    return avg_surfel;
}
