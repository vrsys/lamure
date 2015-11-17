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

class CollisionDetector
{
public:
    typedef std::pair<lamure::pre::SharedBvh, lamure::mat4r> Object;
    typedef std::vector<Object> ObjectArray;
    typedef std::vector<lamure::BoundingBox> AABBArray;

    typedef std::function<void(lamure::NodeIdType a, lamure::NodeIdType b)> CallbackFunc;

    CollisionDetector(const Object& bvh_l,
                      const Object& bvh_r,
                      int stop_level_a,
                      int stop_level_b);

    void SearchIntersections(const CallbackFunc& callback) const;
    const AABBArray& boxes() const { return boxes_; };

private:
    const Object& bvh_l_;
    const Object& bvh_r_;
    AABBArray boxes_;

    void Traverse(lamure::NodeIdType a, lamure::NodeIdType b, 
                  const CallbackFunc& callback) const;

    int stop_level_a_;
    int stop_level_b_;
};


class TreeModifier
{
public:
    TreeModifier(CollisionDetector::ObjectArray& bvhs)
        : bvhs_(bvhs) {}

    void ComplementOnFirstTree(int relax_levels);

    void HistMatchSecondTree();

    void MultRadii(lamure::real factor);

private:

    lamure::real ComputeAvgRadius(const lamure::pre::Bvh& bvh, unsigned depth) const;


    CollisionDetector::ObjectArray& bvhs_;
};

#endif // TREE_MODIFIER_H_

