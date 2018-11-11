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


#include <scm/core.h>
#include <scm/core/math.h>

typedef CGAL::Simple_cartesian<double> Kernel;
typedef CGAL::Polyhedron_3<Kernel> Surface_mesh;
typedef Surface_mesh::HalfedgeDS HalfedgeDS;

typedef boost::graph_traits<Surface_mesh>::vertex_descriptor vertex_descriptor;


typedef CGAL::Surface_mesh<Kernel::Point_3> GMesh;
typedef GMesh::Vertex_index vertex_index;

namespace SMS = CGAL::Surface_mesh_simplification ;

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


// A modifier creating a triangle with the incremental builder.
template<class HDS>
class polyhedron_builder : public CGAL::Modifier_base<HDS> {

public:
  std::vector<double> &coords;
  std::vector<int>    &tris;

  polyhedron_builder( std::vector<double> &_coords, std::vector<int> &_tris ) : coords(_coords), tris(_tris) {}

  void operator()( HDS& hds) {
    typedef typename HDS::Vertex   Vertex;
    typedef typename Vertex::Point Point;
 
    // create a cgal incremental builder
    CGAL::Polyhedron_incremental_builder_3<HDS> B( hds, true);
    B.begin_surface( coords.size()/3, tris.size()/3 );
   
    // add the polyhedron vertices
    for( int i=0; i<(int)coords.size(); i+=3 ){
      B.add_vertex( Point( coords[i+0], coords[i+1], coords[i+2] ) );
    }
   
    // add the polyhedron triangles
    for( int i=0; i<(int)tris.size(); i+=3 ){
      B.begin_facet();
      B.add_vertex_to_facet( tris[i+0] );
      B.add_vertex_to_facet( tris[i+1] );
      B.add_vertex_to_facet( tris[i+2] );
      B.end_facet();
    }
   
    // finish up the surface
    B.end_surface();
    }
};

//custom function to build a mesh of type Surface Mesh
void build_surface_mesh (GMesh &_gmesh, 
              std::vector<double> &_points, 
              std::vector<int> &_tris,
              std::vector<double> &_normals, 
              std::vector<int> &_n_inds
              ){

  std::vector<vertex_index> v_indices;

  //add vertices, record indexes 
  for (int i = 0; i < (int)_points.size(); i+=3){
    vertex_index v = _gmesh.add_vertex(Kernel::Point_3(_points[i+0], _points[i+1], _points[i+2]));
    v_indices.push_back(v);
  }
  //add faces
  for (int i = 0; i < (int)_tris.size(); i+=3){
    _gmesh.add_face(v_indices[_tris[i+0]],v_indices[_tris[i+1]], v_indices[_tris[i+2]]);
  }

  //ref: https://doc.cgal.org/4.7/Surface_mesh/index.html#sectionSurfaceMesh_properties
  GMesh::Property_map<vertex_index, Kernel::Point_3> normal;
  bool created;
  boost::tie(normal, created) = 
    _gmesh.add_property_map<vertex_index, Kernel::Point_3>("v:normal", Kernel::Point_3(0,0,0));
  assert(created);

  //add properties
  uint32_t num_normals = 0;
  BOOST_FOREACH( vertex_index vi, _gmesh.vertices()) { 
    normal[vi] = Kernel::Point_3(_normals[num_normals+0], _normals[num_normals+1], _normals[num_normals+2]);
    std::cout << normal[vi] << std::endl;
    num_normals+=3;
  }

}
 


// reads the first integer from a string in the form
// "334/455/234" by stripping forward slashes and
// scanning the result
int get_first_integer( const char *v ){
 int ival;
 std::string s( v );
 std::replace( s.begin(), s.end(), '/', ' ' );
 sscanf( s.c_str(), "%d", &ival );
 return ival;
}

void parse_face_string (std::string face_string, int (&index)[3], int (&coord)[3], int (&normal)[3]){
  
  // std::cout << "face_string: " << face_string << std::endl;

  //split into faces
  std::vector<std::string> faces;
  boost::algorithm::split(faces, face_string, boost::algorithm::is_any_of(" "));

  for (int i = 0; i < 3; ++i)
  {

    // std::cout << faces[i] << std::endl;

    //split by / for indices
    std::vector<std::string> inds;
    boost::algorithm::split(inds, faces[i], [](char c){return c == '/';}, boost::algorithm::token_compress_off);

    for (int j = 0; j < inds.size(); ++j)
    {
      
      // std::cout << inds[j] << std::endl;

      int idx = 0;
      if (inds[j] != ""){
        idx = stoi(inds[j]);
      }
      if (j == 0){index[i] = idx;}
      else if (j == 1){coord[i] = idx;}
      else if (j == 2){normal[i] = idx;}


      
    }

    std::cout << index[i] << " : " << coord[i]<< " : "  << normal[i] << "  ||  ";
  }
 std::cout << std::endl;

}

// //original load function from James code
// void load_obj(const std::string& filename, std::vector<double>& coords, std::vector<int>& tris ){
//  double x, y, z;
//  char line[1024], v0[1024], v1[1024], v2[1024];

//  // open the file, return if open fails
//  FILE *fp = fopen(filename.c_str(), "r" );
//  if( !fp ) return;
  
//  // read lines from the file, if the first character of the
//  // line is 'v', we are reading a vertex, otherwise, if the
//  // first character is a 'f' we are reading a facet
//  // while( fgets( line, 1024, fp ) ){
//  while (true) {
//   char line[128];
//   int32_t l = fscanf(fp, "%s", line);

//   if (l == EOF) break;

//   // if ( line)
//     // std::cout << line;
//   if (strcmp(line, "vn") == 0) {
//   } 
//   else if (strcmp(line, "vt") == 0) {
//   } 
//   else if (strcmp(line, "v") == 0) {
//     // sscanf( line, "%*s%lf%lf%lf", &x, &y, &z );
//     fscanf(fp, "%f %f %f\n", &x, &y, &z);
//     coords.push_back( x );
//     coords.push_back( y );
//     coords.push_back( z );
//   } 
//   else if(strcmp(line, "f") == 0){
//     sscanf( line, "%*s%s%s%s", v0, v1, v2 );
//     tris.push_back( get_first_integer( v0 )-1 );
//     tris.push_back( get_first_integer( v1 )-1 );
//     tris.push_back( get_first_integer( v2 )-1 );
//   }
//     // else if( line[])
//  }
//  fclose(fp); 
// }


// load obj function from vt_obj_loader/Utils.h
void load_obj(const std::string& filename, 
              std::vector<double>& v,
              std::vector<int>& vindices,
              std::vector<double>& n,  
              std::vector<int>& nindices){

  // std::vector<float> v;
  // std::vector<uint32_t> vindices;
  // std::vector<float> n;
  // std::vector<uint32_t> nindices;
  // std::vector<float> t;
  // std::vector<uint32_t> tindices;

  uint32_t num_tris = 0;

  FILE *file = fopen(filename.c_str(), "r");

  if (0 != file) {

    while (true) {
      char line[128];
      int32_t l = fscanf(file, "%s", line);

      if (l == EOF) break;
      if (strcmp(line, "v") == 0) {
        double vx, vy, vz;
        fscanf(file, "%lf %lf %lf\n", &vx, &vy, &vz);
        v.insert(v.end(), {vx, vy, vz});
      } 
      else if (strcmp(line, "vn") == 0) {
        float nx, ny, nz;
        fscanf(file, "%f %f %f\n", &nx, &ny, &nz);
        n.insert(n.end(), {nx, ny, nz});
      } 
      // else if (strcmp(line, "vt") == 0) {
      //   float tx, ty;
      //   fscanf(file, "%f %f\n", &tx, &ty);
      //   t.insert(t.end(), {tx, ty});
      // } 
      else if (strcmp(line, "f") == 0) {
        fgets(line, 128, file);
        std::string face_string = line; 
        int index[3];
        int coord[3];
        int normal[3];
        // fscanf(file, "%d//%d %d//%d %d//%d\n", //TODO adjust to correctly deal woth missing tex coords
        //    &index[0], &normal[0],
        //    &index[1], &normal[1],
        //    &index[2], &normal[2]);
        // fscanf(file, "%s", &face_string);
        parse_face_string(face_string, index, coord, normal);

        vindices.insert(vindices.end(), {index[0], index[1], index[2]});
        // tindices.insert(tindices.end(), {coord[0], coord[1], coord[2]});
        nindices.insert(nindices.end(), {normal[0], normal[1], normal[2]});
      }
    }

    fclose(file);

    std::cout << "positions: " << vindices.size() << std::endl;
    std::cout << "normals: " << nindices.size() << std::endl;
    // std::cout << "coords: " << tindices.size() << std::endl;

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


  //load a n obj file into vectors
  std::vector<double> points;
  std::vector<int>    tris;
  std::vector<double> normals;
  std::vector<int> n_inds;
  load_obj( obj_filename, points, tris, normals, n_inds);
  if (points.size() == 0 ) {
    std::cout << "didnt find any vertices" << std::endl;
    return 1;
  }
  std::cout << "Mesh loaded (" << points.size()/3 << " vertices)" << std::endl;

  // print loaded obj info

  std::cout << "points" << std::endl;
  for (int i = 0; i < points.size(); i+=3)
  {
    std::cout << points[i] << " "  << points[i+1] << " "  << points[i+2] << std::endl;
  }
  std::cout << "faces" << std::endl;
  for (int i = 0; i < tris.size(); i+=3)
  {
    std::cout << tris[i] << " "  << tris[i+1] << " "  << tris[i+2] << std::endl;
  }
  std::cout << "normals" << std::endl;
  for (int i = 0; i < normals.size(); i+=3)
  {
    std::cout << normals[i] << " "  << normals[i+1] << " "  << normals[i+2] << std::endl;
  }
  std::cout << "norm indexes" << std::endl;
  for (int i = 0; i < n_inds.size(); i+=3)
  {
    std::cout << n_inds[i] << " "  << n_inds[i+1] << " "  << n_inds[i+2] << std::endl;
  }


  //create a mesh from vectors
  // Surface_mesh surface_mesh;
  // polyhedron_builder<HalfedgeDS> builder( points, tris );
  // surface_mesh.delegate(builder);
  GMesh gmesh;
  // build_surface_mesh(gmesh, points, tris, normals, n_inds);

  if (gmesh.is_valid(true)){
    std::cout << "mesh valid\n";
  }

  //trying to add a property map
  // Surface_mesh::Property_map<vertex_descriptor, Vector> vnormals =
  // surface_mesh.add_property_map<vertex_descriptor, Vector>
  //   ("v:normals", CGAL::NULL_VECTOR).first;

  // GMesh::Property_map<vertex_descriptor,std::string> name;


  // if (!CGAL::is_triangle_mesh(surface_mesh)){
  //   std::cerr << "Input geometry is not triangulated." << std::endl;
  //   return EXIT_FAILURE;
  // }
  // // This is a stop predicate (defines when the algorithm terminates).
  // // In this example, the simplification stops when the number of undirected edges
  // // left in the surface mesh drops below the specified number (1000)
  // SMS::Count_stop_predicate<Surface_mesh> stop(500);

  // std::cout << "Starting simplification" << std::endl;
  
  // // This the actual call to the simplification algorithm.
  // // The surface mesh and stop conditions are mandatory arguments.
  // // The index maps are needed because the vertices and edges
  // // of this surface mesh lack an "id()" field.
  // int r = SMS::edge_collapse
  //           (surface_mesh
  //           ,stop
  //            ,CGAL::parameters::vertex_index_map(get(CGAL::vertex_external_index,surface_mesh)) 
  //                              .halfedge_index_map  (get(CGAL::halfedge_external_index  ,surface_mesh)) 
  //                              .get_cost (SMS::Edge_length_cost <Surface_mesh>())
  //                              .get_placement(SMS::Midpoint_placement<Surface_mesh>())
  //           );
  
  // std::cout << "\nFinished...\n" << r << " edges removed.\n" 
  //           << (surface_mesh.size_of_halfedges()/2) << " final edges.\n" ;
        

  // output_obj(gmesh,"bla", points, tris);

  //write obj file
  // std::ofstream ofs(out_filename);
  // // ofs << gmesh;
  // CGAL::print_polyhedron_wavefront(ofs, gmesh);
  // ofs.close();
  // std::cout << "Simplified mesh was written to " << out_filename << std::endl;

  
  return EXIT_SUCCESS ;      
}