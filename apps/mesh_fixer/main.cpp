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
class CEdge;

struct MyTypes : public UsedTypes<Use<CVertex>::AsVertexType, Use<CFace>::AsFaceType, Use<CEdge>::AsEdgeType>
{
};

class CVertex : public Vertex<MyTypes, vertex::VFAdj, vertex::VEAdj, vertex::Coord3f, vertex::TexCoord2f, vertex::Mark, vertex::BitFlags, vertex::Normal3f>
{
};
class CFace : public Face<MyTypes, face::FFAdj, face::VFAdj, face::VertexRef, face::Normal3f, face::WedgeTexCoord2f, face::BitFlags, face::Mark>
{
};
class CEdge : public Edge<MyTypes, edge::EEAdj, edge::VEAdj>
{
};

class CMesh : public vcg::tri::TriMesh<vector<CVertex>, vector<CFace>, vector<CEdge>>
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
    vcg::tri::UpdateTopology<CMesh>::VertexEdge(m);
    vcg::tri::UpdateTopology<CMesh>::EdgeEdge(m);

    int non_manifold_vertices = vcg::tri::Clean<CMesh>::SplitNonManifoldVertex(m, 0.25f);

    vcg::tri::UpdateTopology<CMesh>::FaceFace(m);
    vcg::tri::UpdateTopology<CMesh>::VertexFace(m);
    vcg::tri::UpdateTopology<CMesh>::VertexEdge(m);
    vcg::tri::UpdateTopology<CMesh>::EdgeEdge(m);

    std::cout << "Non-manifold vertices split: " << non_manifold_vertices << std::endl;

    int non_manifold_faces = vcg::tri::Clean<CMesh>::RemoveNonManifoldFace(m);

    vcg::tri::UpdateTopology<CMesh>::FaceFace(m);
    vcg::tri::UpdateTopology<CMesh>::VertexFace(m);
    vcg::tri::UpdateTopology<CMesh>::VertexEdge(m);
    vcg::tri::UpdateTopology<CMesh>::EdgeEdge(m);

    std::cout << "Non-manifold faces removed: " << non_manifold_faces << std::endl;

    auto small_connected_removed = vcg::tri::Clean<CMesh>::RemoveSmallConnectedComponentsSize(m, 50000);

    int duplicate_vertices_removed = vcg::tri::Clean<CMesh>::RemoveDuplicateVertex(m, true);
    int faces_out_of_range_area_removed = vcg::tri::Clean<CMesh>::RemoveFaceOutOfRangeArea(m);
    int unreferenced_vertices_removed = vcg::tri::Clean<CMesh>::RemoveUnreferencedVertex(m);
    int degenerate_edge_removed = vcg::tri::Clean<CMesh>::RemoveDegenerateEdge(m);
    int degenerate_face_removed = vcg::tri::Clean<CMesh>::RemoveDegenerateFace(m);
    int zero_area_face_removed = vcg::tri::Clean<CMesh>::RemoveZeroAreaFace(m);

    vcg::tri::UpdateTopology<CMesh>::FaceFace(m);
    vcg::tri::UpdateTopology<CMesh>::VertexFace(m);
    vcg::tri::UpdateTopology<CMesh>::VertexEdge(m);
    vcg::tri::UpdateTopology<CMesh>::EdgeEdge(m);

    small_connected_removed.first += (vcg::tri::Clean<CMesh>::RemoveSmallConnectedComponentsSize(m, 50000)).first;

    int face_folds_removed = vcg::tri::Clean<CMesh>::RemoveFaceFoldByFlip(m);
    int t_vertices_flipped = vcg::tri::Clean<CMesh>::RemoveTVertexByFlip(m);
    int t_vertices_collapsed = vcg::tri::Clean<CMesh>::RemoveTVertexByCollapse(m);

    vcg::tri::UpdateTopology<CMesh>::FaceFace(m);
    vcg::tri::UpdateTopology<CMesh>::VertexFace(m);
    vcg::tri::UpdateTopology<CMesh>::VertexEdge(m);
    vcg::tri::UpdateTopology<CMesh>::EdgeEdge(m);

    small_connected_removed.first += (vcg::tri::Clean<CMesh>::RemoveSmallConnectedComponentsSize(m, 50000)).first;

    vcg::tri::UpdateTopology<CMesh>::TestFaceFace(m);
    vcg::tri::UpdateTopology<CMesh>::TestVertexFace(m);
    vcg::tri::UpdateTopology<CMesh>::TestVertexEdge(m);

    vcg::tri::Allocator<CMesh>::CompactEveryVector(m);
    vcg::tri::RequireCompactness<CMesh>(m);

    vcg::tri::UpdateNormal<CMesh>::NormalizePerVertex(m);
    vcg::tri::UpdateNormal<CMesh>::NormalizePerFace(m);

    std::cout << "Small connected components removed: " << small_connected_removed.second << " out of " << small_connected_removed.first << " total" << std::endl;

    std::cout << "Duplicate vertices removed: " << duplicate_vertices_removed << std::endl;
    std::cout << "Faces out of range area removed: " << faces_out_of_range_area_removed << std::endl;
    std::cout << "Unreferenced vertices removed: " << unreferenced_vertices_removed << std::endl;
    std::cout << "Degenerate edges removed: " << degenerate_edge_removed << std::endl;
    std::cout << "Degenerate faces removed: " << degenerate_face_removed << std::endl;
    std::cout << "Zero area faces removed: " << zero_area_face_removed << std::endl;

    std::cout << "Face folds removed: " << face_folds_removed << std::endl;
    std::cout << "T-vertices flipped: " << t_vertices_flipped << std::endl;
    std::cout << "T-vertices collapsed: " << t_vertices_collapsed << std::endl;

    std::vector<std::pair<int, vcg::tri::Clean<CMesh>::FacePointer>> CCV;
    int components = vcg::tri::Clean<CMesh>::ConnectedComponents(m, CCV);

    std::cout << "Connected components: " << components << std::endl;

    if(components > 1)
    {
        std::cerr << "FIXED MESH CONTAINS MORE THAN 1 COMPONENT" << std::endl;

        for(int i = 0; i < CCV.size(); i++)
        {
            std::cerr << "Size of component: " << CCV[i].first << std::endl;
        }
    }

    int save_error = vcg::tri::io::ExporterOBJ<CMesh>::Save(m, model_fixed.c_str(), load_mask);

    if(save_error != vcg::tri::io::ExporterOBJ<CMesh>::SaveError::E_NOERROR)
    {
        throw std::runtime_error("Failed to save the model: " + std::string(vcg::tri::io::ExporterOBJ<CMesh>::ErrorMsg(save_error)));
    }

    return 0;
}
