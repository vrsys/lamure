// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef SAMPLER_H_
#define SAMPLER_H_

#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <unordered_set>

#include <vcg/complex/complex.h>
#include <vcg/complex/algorithms/update/normal.h>
#include <wrap/io_trimesh/import.h>

#include "texture.h"


class MyVertex; 
class MyEdge; 
class MyFace;

struct MyUsedTypes : public vcg::UsedTypes<
                        vcg::Use<MyVertex>::AsVertexType,
                        vcg::Use<MyEdge>::AsEdgeType,
                        vcg::Use<MyFace>::AsFaceType>{};

class MyVertex : public vcg::Vertex<MyUsedTypes, 
                                    vcg::vertex::Coord3d, 
                                    vcg::vertex::Normal3f,
                                    vcg::vertex::TexCoord2f,
                                    vcg::vertex::VFAdj,
                                    vcg::vertex::BitFlags>{};

class MyFace : public vcg::Face<MyUsedTypes, 
                                vcg::face::VFAdj, 
                                vcg::face::VertexRef, 
                                vcg::face::WedgeTexCoord2f,
#ifdef USE_WEDGE_NORMALS
                                vcg::face::WedgeNormal,
#endif
                                vcg::face::WedgeNormal,
                                vcg::face::BitFlags> {};

class MyEdge : public vcg::Edge<MyUsedTypes> {};

class MyMesh : public vcg::tri::TriMesh<std::vector<MyVertex>, 
                                        std::vector<MyFace>, 
                                        std::vector<MyEdge>> {};

class sampler { 
public:
    sampler() {};
    virtual ~sampler() {};

    bool load(const std::string& filename);
    bool SampleMesh(const std::string& outputFilename,
      bool flip_x, bool flip_y);

private:

    struct Splat {
        double x, y, z;
        float nx, ny, nz;
        uint8_t r, g, b;
        double d;
    };

    typedef std::vector<Splat> splat_vector;
    typedef vcg::FaceBase<MyUsedTypes>::FacePointer face_pointer;
    typedef std::unordered_set<face_pointer> face_set;

    inline void add_adjacent(face_set& faceList, MyVertex* v) 
    {  
        vcg::face::VFIterator<MyFace> vfi(v);
        for (; !vfi.End(); ++vfi) {
            faceList.insert(vfi.F());
        }
    }

    inline double area(const vcg::Point3d& a, 
                       const vcg::Point3d& b, 
                       const vcg::Point3d& c)
    {
        return 0.5 * vcg::Distance((b-a)^(c-a), 
                                   vcg::Point3d(0.0, 0.0, 0.0));
    }

    inline double area(const vcg::Point2d& a, 
                       const vcg::Point2d& b, 
                       const vcg::Point2d& c)
    {
        return 0.5 *fabs((a.X()-c.X())*(b.Y()-a.Y())-
                         (a.X()-b.X())*(c.Y()-a.Y()));
    }

    inline bool is_point_in_triangle(const vcg::Triangle2<double>& t, 
                                  const vcg::Point2d& p, 
                                  double& uOut, double& vOut)
    {
        const vcg::Point2d v0 = t.P(2) - t.P(0);
        const vcg::Point2d v1 = t.P(1) - t.P(0);
        const vcg::Point2d v2 = p - t.P(0);

        double dot00 = v0.dot(v0);
        double dot01 = v0.dot(v1);
        double dot02 = v0.dot(v2);
        double dot11 = v1.dot(v1);
        double dot12 = v1.dot(v2);

        double invDenom = 1.0 / (dot00 * dot11 - dot01 * dot01);

        double u = (dot11*dot02 - dot01*dot12) * invDenom;
        double v = (dot00*dot12 - dot01*dot02) * invDenom;

        uOut = u;
        vOut = v;

        return (u >= 0.0) && (v >= 0.0) && (u+v <= 1.0);
    }

    void compute_normals();

    bool sample_face(int faceId, bool flip_x, bool flip_y, splat_vector& out);

    bool sample_face(face_pointer facePtr, bool flip_x, bool flip_y, splat_vector& out);

    MyMesh m;
    std::vector<texture> textures;
};


#endif // SAMPLER_H_

