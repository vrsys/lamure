// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>
#include <string>
#include <stack>

using namespace std;

#include <vcg/complex/complex.h>
#include <vcg/complex/algorithms/update/topology.h>
#include <vcg/complex/algorithms/update/bounding.h>
#include <vcg/complex/algorithms/update/flag.h>
#include <vcg/complex/algorithms/clean.h>
#include <vcg/space/intersection/triangle_triangle3.h>
#include <vcg/math/histogram.h>
#include <wrap/io_trimesh/import.h>
#include <wrap/io_trimesh/export.h>
#include <vcg/simplex/face/pos.h>
#include <vcg/complex/algorithms/inertia.h>
#include <vcg/space/index/grid_static_ptr.h>
#include <wrap/ply/plylib.h>
#include <wrap/io_trimesh/import_obj.h>

using namespace vcg;

class CVertex;
class CFace;

struct MyTypes : public UsedTypes<Use<CVertex>::AsVertexType, Use<CFace>::AsFaceType>
{
};

class CVertex : public Vertex<MyTypes, vertex::VFAdj, vertex::Coord3f, vertex::TexCoord2f, vertex::BitFlags, vertex::Normal3f>
{
};
class CFace : public Face<MyTypes, face::FFAdj, face::VFAdj, face::VertexRef, face::Normal3f, face::WedgeTexCoord2f, face::BitFlags, face::Mark>
{
};
class CMesh : public vcg::tri::TriMesh<vector<CVertex>, vector<CFace>>
{
};

char* get_cmd_option(char** begin, char** end, const std::string& option)
{
    char** it = std::find(begin, end, option);
    if(it != end && ++it != end)
        return *it;
    return 0;
}

bool cmd_option_exists(char** begin, char** end, const std::string& option) { return std::find(begin, end, option) != end; }

int main(int argc, char** argv)
{
    if(argc < 3)
    {
        std::cout << "Usage: " << argv[0] << " input.obj output.obj\n" << std::endl;
        return -1;
    }

    const std::string model = argv[1];
    const std::string model_fixed = argv[2];

    CMesh m;

    int load_mask;
    bool mask_load_success = vcg::tri::io::ImporterOBJ<CMesh>::LoadMask(model.c_str(), load_mask);
    int load_error = vcg::tri::io::ImporterOBJ<CMesh>::Open(m, model.c_str(), load_mask);

    if(!mask_load_success || load_error != vcg::tri::io::ImporterOBJ<CMesh>::OBJError::E_NOERROR)
    {
        if(vcg::tri::io::ImporterOBJ<CMesh>::ErrorCritical(load_error))
        {
            throw std::runtime_error("Failed to load the model: " + std::string(vcg::tri::io::ImporterOBJ<CMesh>::ErrorMsg(load_error)));
        }
        else
        {
            std::cerr << std::string(vcg::tri::io::ImporterOBJ<CMesh>::ErrorMsg(load_error)) << std::endl;
        }
    }

    vcg::tri::UpdateTopology<CMesh>::FaceFace(m);
    vcg::tri::UpdateTopology<CMesh>::VertexFace(m);

    int non_manifold_vertices = vcg::tri::Clean<CMesh>::SplitNonManifoldVertex(m, 0.25f);

    vcg::tri::UpdateTopology<CMesh>::FaceFace(m);
    vcg::tri::UpdateTopology<CMesh>::VertexFace(m);

    std::cout << "Non-manifold vertices split: " << non_manifold_vertices << std::endl;

    int non_manifold_faces = vcg::tri::Clean<CMesh>::RemoveNonManifoldFace(m);

    vcg::tri::UpdateTopology<CMesh>::FaceFace(m);
    vcg::tri::UpdateTopology<CMesh>::VertexFace(m);

    std::cout << "Non-manifold faces removed: " << non_manifold_faces << std::endl;

    int duplicate_vertices_removed = vcg::tri::Clean<CMesh>::RemoveDuplicateVertex(m, true);
    int faces_out_of_range_area_removed = vcg::tri::Clean<CMesh>::RemoveFaceOutOfRangeArea(m);
    int unreferenced_vertices_removed = vcg::tri::Clean<CMesh>::RemoveUnreferencedVertex(m);
    int degenerate_edge_removed = vcg::tri::Clean<CMesh>::RemoveDegenerateEdge(m);
    int degenerate_face_removed = vcg::tri::Clean<CMesh>::RemoveDegenerateFace(m);

    vcg::tri::UpdateTopology<CMesh>::FaceFace(m);
    vcg::tri::UpdateTopology<CMesh>::VertexFace(m);

    std::cout << "Duplicate vertices removed: " << duplicate_vertices_removed << std::endl;
    std::cout << "Faces out of range area removed: " << faces_out_of_range_area_removed << std::endl;
    std::cout << "Unreferenced vertices removed: " << unreferenced_vertices_removed << std::endl;
    std::cout << "Degenerate edges removed: " << degenerate_edge_removed << std::endl;
    std::cout << "Degenerate faces removed: " << degenerate_face_removed << std::endl;

    int save_error = vcg::tri::io::ExporterOBJ<CMesh>::Save(m, model_fixed.c_str(), load_mask);

    if(save_error != vcg::tri::io::ExporterOBJ<CMesh>::SaveError::E_NOERROR)
    {
        throw std::runtime_error("Failed to save the model: " + std::string(vcg::tri::io::ExporterOBJ<CMesh>::ErrorMsg(save_error)));
    }

    return 0;
}
