#include <iostream>
#include <fstream>
#include <algorithm>

#include <CGAL/Simple_cartesian.h>
#include <CGAL/Polyhedron_3.h>

//for obj export
#include <CGAL/IO/print_wavefront.h>

// Simplification function
#include <CGAL/Surface_mesh_simplification/edge_collapse.h>

// Stop-condition policy
#include <CGAL/Surface_mesh_simplification/Policies/Edge_collapse/Count_stop_predicate.h>
#include <CGAL/Surface_mesh_simplification/Policies/Edge_collapse/Edge_length_cost.h>
#include <CGAL/Surface_mesh_simplification/Policies/Edge_collapse/Midpoint_placement.h>

#include<CGAL/Polyhedron_incremental_builder_3.h>


#include <scm/core.h>
#include <scm/core/math.h>

typedef CGAL::Simple_cartesian<double> Kernel;
typedef CGAL::Polyhedron_3<Kernel> Surface_mesh;
typedef Surface_mesh::HalfedgeDS HalfedgeDS;

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

//original load function from James code
void load_obj(const std::string& filename, std::vector<double>& coords, std::vector<int>& tris ){
 double x, y, z;
 char line[1024], v0[1024], v1[1024], v2[1024];

 // open the file, return if open fails
 FILE *fp = fopen(filename.c_str(), "r" );
 if( !fp ) return;
  
 // read lines from the file, if the first character of the
 // line is 'v', we are reading a vertex, otherwise, if the
 // first character is a 'f' we are reading a facet
 while( fgets( line, 1024, fp ) ){
  if( line[0] == 'v' ){
    sscanf( line, "%*s%lf%lf%lf", &x, &y, &z );
    coords.push_back( x );
    coords.push_back( y );
    coords.push_back( z );
  } 
  else if( line[0] == 'f' ){
    sscanf( line, "%*s%s%s%s", v0, v1, v2 );
    tris.push_back( get_first_integer( v0 )-1 );
    tris.push_back( get_first_integer( v1 )-1 );
    tris.push_back( get_first_integer( v2 )-1 );
  }
  // else if( line[])
 }
 fclose(fp); 
}


//load obj function from vt_obj_loader/Utils.h
// void load_obj(const std::string& filename, std::vector<double>& v, std::vector<int>& vindices ){

//   // std::vector<float> v;
//   // std::vector<uint32_t> vindices;
//   // std::vector<float> n;
//   // std::vector<uint32_t> nindices;
//   // std::vector<float> t;
//   // std::vector<uint32_t> tindices;

//   uint32_t num_tris = 0;

//   FILE *file = fopen(filename.c_str(), "r");

//   if (0 != file) {

//     while (true) {
//       char line[128];
//       int32_t l = fscanf(file, "%s", line);

//       if (l == EOF) break;
//       if (strcmp(line, "v") == 0) {
//         double vx, vy, vz;
//         fscanf(file, "%f %f %f\n", &vx, &vy, &vz);
//         v.insert(v.end(), {vx, vy, vz});
//       } 
//       // else if (strcmp(line, "vn") == 0) {
//       //   float nx, ny, nz;
//       //   fscanf(file, "%f %f %f\n", &nx, &ny, &nz);
//       //   n.insert(n.end(), {nx, ny, nz});
//       // } 
//       // else if (strcmp(line, "vt") == 0) {
//       //   float tx, ty;
//       //   fscanf(file, "%f %f\n", &tx, &ty);
//       //   t.insert(t.end(), {tx, ty});
//       // } 
//       else if (strcmp(line, "f") == 0) {
//         std::string vertex1, vertex2, vertex3;
//         int index[3];
//         int coord[3];
//         int normal[3];
//         fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n",
//            &index[0], &coord[0], &normal[0],
//            &index[1], &coord[1], &normal[1],
//            &index[2], &coord[2], &normal[2]);

//         vindices.insert(vindices.end(), {index[0], index[1], index[2]});
//         // tindices.insert(tindices.end(), {coord[0], coord[1], coord[2]});
//         // nindices.insert(nindices.end(), {normal[0], normal[1], normal[2]});
//       }
//     }

//     fclose(file);

//     std::cout << "positions: " << vindices.size() << std::endl;
//     // std::cout << "normals: " << nindices.size() << std::endl;
//     // std::cout << "coords: " << tindices.size() << std::endl;

//   }

// }


// struct vertex {
//     scm::math::vec3f position_;
//     scm::math::vec2f coords_;
//     scm::math::vec3f normal_;
// };

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


  //load a mesh
  std::vector<double> coords;
  std::vector<int>    tris;
  load_obj( obj_filename, coords, tris );

  if (coords.size() == 0 ) {
    std::cout << "didnt find any vertices" << std::endl;
    return 1;
  }

  std::cout << "Mesh loaded (" << coords.size() << " vertices)" << std::endl;

  Surface_mesh surface_mesh;
  polyhedron_builder<HalfedgeDS> builder( coords, tris );
  surface_mesh.delegate(builder);

  if (!CGAL::is_triangle_mesh(surface_mesh)){
    std::cerr << "Input geometry is not triangulated." << std::endl;
    return EXIT_FAILURE;
  }
  // This is a stop predicate (defines when the algorithm terminates).
  // In this example, the simplification stops when the number of undirected edges
  // left in the surface mesh drops below the specified number (1000)
  SMS::Count_stop_predicate<Surface_mesh> stop(500);

  std::cout << "Starting simplification" << std::endl;
  
  // This the actual call to the simplification algorithm.
  // The surface mesh and stop conditions are mandatory arguments.
  // The index maps are needed because the vertices and edges
  // of this surface mesh lack an "id()" field.
  int r = SMS::edge_collapse
            (surface_mesh
            ,stop
             ,CGAL::parameters::vertex_index_map(get(CGAL::vertex_external_index,surface_mesh)) 
                               .halfedge_index_map  (get(CGAL::halfedge_external_index  ,surface_mesh)) 
                               .get_cost (SMS::Edge_length_cost <Surface_mesh>())
                               .get_placement(SMS::Midpoint_placement<Surface_mesh>())
            );
  
  std::cout << "\nFinished...\n" << r << " edges removed.\n" 
            << (surface_mesh.size_of_halfedges()/2) << " final edges.\n" ;
        
  //write obj file
  std::ofstream ofs(out_filename);
  CGAL::print_polyhedron_wavefront(ofs, surface_mesh);
  ofs.close();

  std::cout << "Simplified mesh was written to " << out_filename << std::endl;

  
  return EXIT_SUCCESS ;      
}