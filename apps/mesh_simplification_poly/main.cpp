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

// // //define a new vertex type - inheriting from base type CGAL::HalfedgeDS_vertex_base
//ref https://doc.cgal.org/4.7/Polyhedron/index.html#title11
template <class Refs, class T, class P, class Traits>
struct XtndVertex : public CGAL::HalfedgeDS_vertex_base<Refs, T, P>  {
    
    // typedef typename Traits::Vector_3 Point;
    // typedef typename Traits::Vector_3 Normal;
    // Normal normal;
    // Point pos;

  // XtndVertex(Point p) : CGAL::HalfedgeDS_vertex_base(p){}
  using CGAL::HalfedgeDS_vertex_base<Refs, T, P>::HalfedgeDS_vertex_base;

};

// namespace Traits{
template <class Traits>
struct XtndPoint : public Traits::Point_3 {

  XtndPoint() : Traits::Point_3() {}

  XtndPoint(double a, double b, double c, double d, double e, double f) 
    : Traits::Point_3(a, b, c),
      normal(d, e, f) {}
  //XtndPoint(Traits const& a, Traits const& b, Traits const& c) 
  //  : Traits::Point_3(a, b, c) {}
  // typedef typename Traits::Point_3 Point;
  typedef typename Traits::Vector_3 Normal;
  Normal normal;

  // using Traits::Point_3

};
// }

// A new items type using extended vertex
struct Custom_items : public CGAL::Polyhedron_items_3 {
    template <class Refs, class Traits>
    struct Vertex_wrapper {


      // typedef typename Traits::XtndPoint XtndPoint;
      // typedef typename Traits::Vector_3 Normal;

      typedef XtndVertex<Refs,CGAL::Tag_true, XtndPoint<Traits>,  Traits> Vertex;
    };
};



// //from http://cgal-discuss.949826.n4.nabble.com/Extended-polyhedra-amp-nef-polyhedra-td4656726.html
// // struct Custom_items : public CGAL::Polyhedron_items_3 { 

// //   // wrap vertex
// //   template <class Refs, class Traits>
// //   struct Vertex_wrapper
// //   {
// //       typedef typename Traits::Point_3  Point;
// //       typedef typename Traits::Vector_3 Normal;
// //       typedef XtndVertex<Refs,CGAL::Tag_true,Point,Normal> Vertex;
// //   }; 
// };


typedef CGAL::Simple_cartesian<double> Kernel;
typedef CGAL::Polyhedron_3<Kernel, Custom_items> Polyhedron;
// typedef CGAL::Polyhedron_3<Kernel> Polyhedron;
typedef Polyhedron::HalfedgeDS HalfedgeDS;


namespace SMS = CGAL::Surface_mesh_simplification ;


// struct GVertex
// {
//   Kernel::Point_3 v_pos;
//   Kernel::Point_2 t_coord;
//   Kernel::Point_3 normal;
// };

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

//parses a face string like "f  2//1  8//1  4//1 " into 3 given arrays
void parse_face_string (std::string face_string, int (&index)[3], int (&coord)[3], int (&normal)[3]){

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
      int idx = 0;
      //parse value from string
      if (inds[j] != ""){
        idx = stoi(inds[j]);
      }
      if (j == 0){index[i] = idx;}
      else if (j == 1){coord[i] = idx;}
      else if (j == 2){normal[i] = idx;}
      
    }
  }
}

// load obj function from vt_obj_loader/Utils.h
void load_obj(const std::string& filename, 
              std::vector<double> &v, 
              std::vector<int> &vindices ){

  // std::vector<double> v;
  // std::vector<int> vindices;
  std::vector<double> n;
  std::vector<int> nindices;
  std::vector<double> t;
  std::vector<int> tindices;

  FILE *file = fopen(filename.c_str(), "r");

  if (0 != file) {

    while (true) {
      char line[128];
      int32_t l = fscanf(file, "%s", line);

      if (l == EOF) break;
      if (strcmp(line, "v") == 0) {
        double vx, vy, vz;
        fscanf(file, "%lf %lf %lf\n", &vx, &vy, &vz);
        // v.push_back(Kernel::Point_3(vx,vy,vz));
        v.insert(v.end(), {vx, vy, vz});
      } 
      else if (strcmp(line, "vn") == 0) {
        float nx, ny, nz;
        fscanf(file, "%f %f %f\n", &nx, &ny, &nz);
        // n.push_back(Kernel::Point_3(nx,ny,nz));
        n.insert(n.end(), {nx,ny, nz});
      } 
      else if (strcmp(line, "vt") == 0) {
        float tx, ty;
        fscanf(file, "%f %f\n", &tx, &ty);
        // t.push_back(Kernel::Point_2(tx,ty));
        t.insert(t.end(), {tx, ty});
      } 
      else if (strcmp(line, "f") == 0) {
        fgets(line, 128, file);
        std::string face_string = line; 
        int index[3];
        int coord[3];
        int normal[3];

        parse_face_string(face_string, index, coord, normal);

        vindices.insert(vindices.end(), {index[0]-1, index[1]-1, index[2]-1});
        tindices.insert(tindices.end(), {coord[0]-1, coord[1]-1, coord[2]-1});
        nindices.insert(nindices.end(), {normal[0]-1, normal[1]-1, normal[2]-1});
      }
    }

    fclose(file);

    std::cout << "positions: " << v.size()/3 << std::endl;
    std::cout << "normals: " << n.size()/3 << std::endl;
    std::cout << "coords: " << t.size()/2 << std::endl;
    std::cout << "faces: " << vindices.size()/3 << std::endl;

  }

}

// // A modifier creating a triangle with the incremental builder.
//http://jamesgregson.blogspot.com/2012/05/example-code-for-building.html
template<class HDS>
class polyhedron_builder : public CGAL::Modifier_base<HDS> {

public:
  std::vector<double> &vertices;
  std::vector<int>    &tris;


  polyhedron_builder( std::vector<double> &_vertices, std::vector<int> &_tris ) : vertices(_vertices), tris(_tris) {}

  void operator()( HDS& hds) {
    // typedef typename HDS::Vertex   Vertex;
    // typedef typename XtndPoint xPoint;
    // typedef typename Vertex::Normal Normal;
 
    // create a cgal incremental builder
    CGAL::Polyhedron_incremental_builder_3<HDS> B( hds, true);
    B.begin_surface( vertices.size()/3, tris.size()/3 );
   
    // add the polyhedron vertices
    for( int i=0; i<(int)vertices.size(); i+=3 ){
      // B.add_vertex( Point( vertices[i+0], vertices[i+1], vertices[i+2] ) );
      // B.add_vertex( Point( vertices[i+0], vertices[i+1], vertices[i+2] ) );
      // XtndPoint xP(0,0,0);

      B.add_vertex( XtndPoint<Kernel>( vertices[i+0], vertices[i+1], vertices[i+2] ), 0 , 0 , 0 );

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


  //load OBJ into arrays
  std::vector<double> vertices;
  std::vector<int> tris;
  load_obj( obj_filename, vertices, tris);

  if (vertices.size() == 0 ) {
    std::cout << "didnt find any vertices" << std::endl;
    return 1;
  }
  std::cout << "Mesh loaded (" << vertices.size() << " vertices)" << std::endl;


  // build a polyhedron from the loaded arrays
  Polyhedron polyMesh;
  polyhedron_builder<HalfedgeDS> builder( vertices, tris );
  polyMesh.delegate( builder );

  if (polyMesh.is_valid(true)){
    std::cout << "mesh valid\n"; 
  }



  if (!CGAL::is_triangle_mesh(polyMesh)){
    std::cerr << "Input geometry is not triangulated." << std::endl;
    return EXIT_FAILURE;
  }
  // This is a stop predicate (defines when the algorithm terminates).
  // In this example, the simplification stops when the number of undirected edges
  // left in the surface mesh drops below the specified number (1000)
  SMS::Count_stop_predicate<Polyhedron> stop(300);

  std::cout << "Starting simplification" << std::endl;
  
  // This the actual call to the simplification algorithm.
  // The surface mesh and stop conditions are mandatory arguments.
  // The index maps are needed because the vertices and edges
  // of this surface mesh lack an "id()" field.
  int r = SMS::edge_collapse
            (polyMesh
            ,stop
             ,CGAL::parameters::vertex_index_map(get(CGAL::vertex_external_index,polyMesh)) 
                               .halfedge_index_map  (get(CGAL::halfedge_external_index  ,polyMesh)) 
                               .get_cost (SMS::Edge_length_cost <Polyhedron>())
                               .get_placement(SMS::Midpoint_placement<Polyhedron>())
            );
  
  std::cout << "\nFinished...\n" << (polyMesh.size_of_halfedges()/2) << " final edges.\n" ;
        

  //write to file
  std::ofstream ofs(out_filename);
  CGAL::print_polyhedron_wavefront(ofs, polyMesh);
  ofs.close();
  std::cout << "simplified mesh was written to " << out_filename << std::endl;

  
  return EXIT_SUCCESS ;      
}