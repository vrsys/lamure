#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

#include <CGAL/Simple_cartesian.h>
#include <CGAL/Polyhedron_3.h>

//for obj export
#include <CGAL/IO/print_wavefront.h>

// Simplification function
#include <CGAL/Surface_mesh_simplification/edge_collapse.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/property_map.h>

// Stop-condition policy
#include <CGAL/Surface_mesh_simplification/Policies/Edge_collapse/Count_stop_predicate.h>
#include <CGAL/Surface_mesh_simplification/Policies/Edge_collapse/Edge_length_cost.h>
#include <CGAL/Surface_mesh_simplification/Policies/Edge_collapse/Midpoint_placement.h>

#include<CGAL/Polyhedron_incremental_builder_3.h>


#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/trim.hpp>


#include <scm/core.h>
#include <scm/core/math.h>

typedef CGAL::Simple_cartesian<double> Kernel;
typedef CGAL::Polyhedron_3<Kernel> Surface_mesh;
typedef Surface_mesh::HalfedgeDS HalfedgeDS;

typedef boost::graph_traits<Surface_mesh>::vertex_descriptor vertex_descriptor;


typedef CGAL::Surface_mesh<Kernel::Point_3> GMesh;
typedef GMesh::Vertex_index vertex_index;

namespace SMS = CGAL::Surface_mesh_simplification ;


struct GVertex
{
  Kernel::Point_3 v_pos;
  Kernel::Point_2 t_coord;
  Kernel::Point_3 normal;
};

char* getCmdOption(char ** begin, char ** end, const std::string & option) {
  char ** itr = std::find(begin, end, option);
  if (itr != end && ++itr != end) {
      return *itr;
  }
  return 0;
}

bool cmdOptionExists(char** begin, char** end, const std::string& option) {
  return std::find(begin, end, option) != end;
}


// // A modifier creating a triangle with the incremental builder.
// template<class HDS>
// class polyhedron_builder : public CGAL::Modifier_base<HDS> {

// public:
//   std::vector<double> &coords;
//   std::vector<int>    &tris;

//   polyhedron_builder( std::vector<double> &_coords, std::vector<int> &_tris ) : coords(_coords), tris(_tris) {}

//   void operator()( HDS& hds) {
//     typedef typename HDS::Vertex   Vertex;
//     typedef typename Vertex::Point Point;
 
//     // create a cgal incremental builder
//     CGAL::Polyhedron_incremental_builder_3<HDS> B( hds, true);
//     B.begin_surface( coords.size()/3, tris.size()/3 );
   
//     // add the polyhedron vertices
//     for( int i=0; i<(int)coords.size(); i+=3 ){
//       B.add_vertex( Point( coords[i+0], coords[i+1], coords[i+2] ) );
//     }
   
//     // add the polyhedron triangles
//     for( int i=0; i<(int)tris.size(); i+=3 ){
//       B.begin_facet();
//       B.add_vertex_to_facet( tris[i+0] );
//       B.add_vertex_to_facet( tris[i+1] );
//       B.add_vertex_to_facet( tris[i+2] );
//       B.end_facet();
//     }
   
//     // finish up the surface
//     B.end_surface();
//     }
// };

//custom function to build a mesh of type Surface Mesh
void build_surface_mesh (GMesh &_gmesh, 
              std::vector<GVertex> &_vertices){

  //to store refs to vertices while the mesh is being built
  std::vector<vertex_index> v_indices;

  //add vertex positions, record indexes 
  for (int i = 0; i < (int)_vertices.size(); i+=3){
    vertex_index v1 = _gmesh.add_vertex(_vertices[i].v_pos);
    vertex_index v2 = _gmesh.add_vertex(_vertices[i+1].v_pos);
    vertex_index v3 = _gmesh.add_vertex(_vertices[i+2].v_pos);

    //add face 
    _gmesh.add_face(v1, v2, v3);

    // v_indices.push_back(v);
    v_indices.insert(v_indices.end(), {v1, v2, v3});
  }

  //add faces
  // for (int i = 0; i < (int)_vertices.size(); i+=3){
  //   _gmesh.add_face(v_indices[_tris[i+0]],v_indices[_tris[i+1]], v_indices[_tris[i+2]]);
  // }

  //create a property map for adding normals
  //ref: https://doc.cgal.org/4.7/Surface_mesh/index.html#sectionSurfaceMesh_properties
  GMesh::Property_map<vertex_index, Kernel::Point_3> normal;
  bool created;
  boost::tie(normal, created) = 
    _gmesh.add_property_map<vertex_index, Kernel::Point_3>("v:normal", Kernel::Point_3(0,0,0));
  assert(created);

  //add normals
  for (int i = 0; i < (int)v_indices.size(); ++i)
  {
    normal[v_indices[i]] = _vertices[i].normal;
    std::cout << normal[v_indices[i]] << std::endl;
  }



  // uint32_t num_normals = 0;
  // BOOST_FOREACH( vertex_index vi, _gmesh.vertices()) { 
  //   normal[vi] = Kernel::Point_3(_normals[num_normals+0], _normals[num_normals+1], _normals[num_normals+2]);
  //   std::cout << normal[vi] << std::endl;
  //   num_normals+=3;
  // }

}

//parses a face string like "f  2//1  8//1  4//1 " into 3 given arrays
void parse_face_string (std::string face_string, uint32_t (&index)[3], uint32_t (&coord)[3], uint32_t (&normal)[3]){

  //split by space into faces
  std::vector<std::string> faces;
  boost::algorithm::trim(face_string);
  boost::algorithm::split(faces, face_string, boost::algorithm::is_any_of(" "), boost::algorithm::token_compress_on);

  for (int i = 0; i < 3; ++i)
  {
    //split by / for indices
    std::vector<std::string> inds;
    boost::algorithm::split(inds, faces[i], [](char c){return c == '/';}, boost::algorithm::token_compress_off);

    for (int j = 0; j < (int)inds.size(); ++j)
    {
      uint32_t idx = 0;
      //parse value from string
      if (inds[j] != ""){
        idx = (uint32_t)stoi(inds[j]);
      }
      if (j == 0){index[i] = idx;}
      else if (j == 1){coord[i] = idx;}
      else if (j == 2){normal[i] = idx;}
      
    }
  }
}


// load obj function from vt_obj_loader/Utils.h
void load_obj(const std::string& filename, 
              std::vector<GVertex>& vertices){

  std::vector<Kernel::Point_3> v;
  std::vector<uint32_t> vindices;
  std::vector<Kernel::Point_3> n;
  std::vector<uint32_t> nindices;
  std::vector<Kernel::Point_2> t;
  std::vector<uint32_t> tindices;

  FILE *file = fopen(filename.c_str(), "r");

  if (0 != file) {

    while (true) {
      char line[128];
      int32_t l = fscanf(file, "%s", line);

      if (l == EOF) break;
      if (strcmp(line, "v") == 0) {
        double vx, vy, vz;
        fscanf(file, "%lf %lf %lf\n", &vx, &vy, &vz);
        v.push_back(Kernel::Point_3(vx,vy,vz));
      } 
      else if (strcmp(line, "vn") == 0) {
        float nx, ny, nz;
        fscanf(file, "%f %f %f\n", &nx, &ny, &nz);
        n.push_back(Kernel::Point_3(nx,ny,nz));
      } 
      else if (strcmp(line, "vt") == 0) {
        float tx, ty;
        fscanf(file, "%f %f\n", &tx, &ty);
        t.push_back(Kernel::Point_2(tx,ty));
      } 
      else if (strcmp(line, "f") == 0) {
        fgets(line, 128, file);
        std::string face_string = line; 
        uint32_t index[3];
        uint32_t coord[3];
        uint32_t normal[3];

        parse_face_string(face_string, index, coord, normal);

        vindices.insert(vindices.end(), {index[0], index[1], index[2]});
        tindices.insert(tindices.end(), {coord[0], coord[1], coord[2]});
        nindices.insert(nindices.end(), {normal[0], normal[1], normal[2]});
      }
    }

    fclose(file);

    std::cout << "positions: " << v.size() << std::endl;
    std::cout << "normals: " << n.size() << std::endl;
    std::cout << "coords: " << t.size() << std::endl;
    std::cout << "faces: " << vindices.size() << std::endl;

  }

  //read from indices arrays to construct list of vertices in open GL style
  //http://www.opengl-tutorial.org/beginners-tutorials/tutorial-7-model-loading/
  for (int i = 0; i < (int)vindices.size(); ++i)
  {
    Kernel::Point_3 newv;
    Kernel::Point_2 newt;
    Kernel::Point_3 newn;

    //guard against empty arrays 
    if (v.size() == 0) {newv = Kernel::Point_3(0,0,0);}
    else {newv = v[vindices[i]-1];}

    if (t.size() == 0) {newt = Kernel::Point_2(0,0,0);}
    else {newt = t[tindices[i]-1];}

    if (n.size() == 0) {newn = Kernel::Point_3(0,0,0);}
    else {newn = n[nindices[i]-1];}

    GVertex new_Vertex = {newv, newt, newn};
    vertices.push_back(new_Vertex);
  }

}

// void output_obj (GMesh &gmesh, const std::string& filename, std::vector<double>& v, std::vector<int>& vindices) {
//   // https://doc.cgal.org/4.9/Surface_mesh/index.html

//     typedef typename HDS::Vertex   Vertex;
//     typedef typename Vertex::Point Point;
  
//   std::cout << "all vertices " << std::endl;
//   // The vertex iterator type is a nested type of the Vertex_range
//   GMesh::Vertex_range::iterator  vb, ve;
//   GMesh::Vertex_range r = gmesh.vertices();
//   // The iterators can be accessed through the C++ range API
//   vb = r.begin(); 
//   ve = r.end();
//   // or the boost Range API
//   vb = boost::begin(r);
//   ve = boost::end(r);
//   // or with boost::tie, as the CGAL range derives from std::pair
//   for(boost::tie(vb, ve) = gmesh.vertices(); vb != ve; ++vb){
//     Point p = gmesh.point(*vb);
//     std::cout << p << std::endl;
//           // std::cout << *vb << std::endl;
//   }
// }


int main( int argc, char** argv ) 
{
  std::string obj_filename = "dino.obj";
  if (cmdOptionExists(argv, argv+argc, "-f")) {
    obj_filename = std::string(getCmdOption(argv, argv + argc, "-f"));
  }
  else {
    std::cout << "Please provide a obj filename using -f <filename.obj>" << std::endl;
    return 1;
  }
  std::string out_filename = "data/simplified_mesh.obj";
  if (cmdOptionExists(argv, argv+argc, "-o")) {
    out_filename = std::string(getCmdOption(argv, argv + argc, "-o"));
  }


  //load OBJ into vertex array
  std::vector<GVertex> vertices;
  load_obj( obj_filename, vertices);

  if (vertices.size() == 0 ) {
    std::cout << "didnt find any vertices" << std::endl;
    return 1;
  }
  std::cout << "Mesh loaded (" << vertices.size() << " vertices)" << std::endl;

  // print loaded obj info
  // std::cout << "points" << std::endl;
  // for (int i = 0; i < points.size(); i+=3)
  // {
  //   std::cout << points[i] << " "  << points[i+1] << " "  << points[i+2] << std::endl;
  // }
  // std::cout << "faces" << std::endl;
  // for (int i = 0; i < tris.size(); i+=3)
  // {
  //   std::cout << tris[i] << " "  << tris[i+1] << " "  << tris[i+2] << std::endl;
  // }
  // std::cout << "normals" << std::endl;
  // for (int i = 0; i < normals.size(); i+=3)
  // {
  //   std::cout << normals[i] << " "  << normals[i+1] << " "  << normals[i+2] << std::endl;
  // }
  // std::cout << "norm indexes" << std::endl;
  // for (int i = 0; i < n_inds.size(); i+=3)
  // {
  //   std::cout << n_inds[i] << " "  << n_inds[i+1] << " "  << n_inds[i+2] << std::endl;
  // }


  //create a mesh from vectors
  GMesh gmesh;
  build_surface_mesh(gmesh, vertices);

  if (gmesh.is_valid(true)){
    std::cout << "mesh valid\n";
  }



  if (!CGAL::is_triangle_mesh(gmesh)){
    std::cerr << "Input geometry is not triangulated." << std::endl;
    return EXIT_FAILURE;
  }
  // This is a stop predicate (defines when the algorithm terminates).
  // In this example, the simplification stops when the number of undirected edges
  // left in the surface mesh drops below the specified number (1000)
  SMS::Count_stop_predicate<Surface_mesh> stop(300);

  std::cout << "Starting simplification" << std::endl;
  
  // This the actual call to the simplification algorithm.
  // The surface mesh and stop conditions are mandatory arguments.
  // The index maps are needed because the vertices and edges
  // of this surface mesh lack an "id()" field.
  // int r = SMS::edge_collapse
  //           (gmesh
  //           ,stop
  //            ,CGAL::parameters::halfedge_index_map  (get(CGAL::halfedge_external_index  ,gmesh)) 
  //            // ,CGAL::parameters::vertex_index_map(get(CGAL::vertex_external_index,gmesh)) 
  //                              // .halfedge_index_map  (get(CGAL::halfedge_external_index  ,gmesh)) 
  //            //                   .get_cost (SMS::Edge_length_cost <Surface_mesh>())
  //            //                   .get_placement(SMS::Midpoint_placement<Surface_mesh>())
  //           );
  
  // std::cout << "\nFinished...\n" << (gmesh.size_of_halfedges()/2) << " final edges.\n" ;
        

  // output_obj(gmesh,"bla", points, tris);

  //write to file
  std::ofstream ofs(out_filename);
  ofs << gmesh;
  // CGAL::print_polyhedron_wavefront(ofs, gmesh);
  ofs.close();
  std::cout << "Simplified mesh was written to " << out_filename << std::endl;

  
  return EXIT_SUCCESS ;      
}