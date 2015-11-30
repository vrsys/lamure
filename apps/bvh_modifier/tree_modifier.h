// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef TREE_MODIFIER_H_
#define TREE_MODIFIER_H_

#include <lamure/pre/bvh.h>
#include <functional>

class collision_detector
{
public:
    typedef std::pair<lamure::pre::bvh_ptr, lamure::mat4r> object;
    typedef std::vector<object> objectArray;
    typedef std::vector<lamure::bounding_box> aabb_array;

    typedef std::function<void(lamure::node_id_type a, lamure::node_id_type b)> callback_func;

    collision_detector(const object& bvh_l,
                      const object& bvh_r,
                      int stop_level_a,
                      int stop_level_b);

    void search_intersections(const callback_func& callback) const;
    const aabb_array& boxes() const { return boxes_; };

private:
    const object& bvh_l_;
    const object& bvh_r_;
    aabb_array boxes_;

    void traverse(lamure::node_id_type a, lamure::node_id_type b, 
                  const callback_func& callback) const;

    int stop_level_a_;
    int stop_level_b_;
};


class TreeModifier
{
public:
    TreeModifier(collision_detector::objectArray& bvhs)
        : bvhs_(bvhs) {}

    void complementOnFirstTree(int relax_levels);

    void histogrammatchSecondTree();

    void MultRadii(lamure::real factor);

private:

    lamure::real computeAvgRadius(const lamure::pre::bvh& bvh, unsigned depth) const;


    collision_detector::objectArray& bvhs_;
};

#endif // TREE_MODIFIER_H_

