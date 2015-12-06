// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef TYPES_H_INCLUDED
#define TYPES_H_INCLUDED

#include <lamure/types.h>
#include <scm/core/math.h>

struct sphere {
    scm::math::vec3f center_;
    float radius_;

    sphere(const scm::math::vec3f& center, float radius)
    : center_(center), radius_(radius) {

    }
};

struct surfel {
    float x, y, z;
    uint8_t r, g, b, fake;
    float size;
    float nx, ny, nz;
};


struct NodeSplatId {
    lamure::node_t node_id_;
    unsigned int splat_id_;

    NodeSplatId(lamure::node_t node_id, unsigned int splat_id)
    : node_id_(node_id), splat_id_(splat_id) {

    }
};

struct NodeNoise {
  float noise_;
  lamure::node_t node_id_;
};

bool operator < (const NodeNoise& a, const NodeNoise& b) {
  return a.noise_ > b.noise_;
}








#endif
