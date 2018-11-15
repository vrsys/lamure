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

#include "Utils.h"
#include "OBJ_printer.h"

// // //define a new vertex type - inheriting from base type CGAL::HalfedgeDS_vertex_base
//ref https://doc.cgal.org/4.7/Polyhedron/index.html#title11
template <class Refs, class T, class P>
struct XtndVertex : public CGAL::HalfedgeDS_vertex_base<Refs, T, P>  {
    
  using CGAL::HalfedgeDS_vertex_base<Refs, T, P>::HalfedgeDS_vertex_base;

};

//extended point class
template <class Traits>
struct XtndPoint : public Traits::Point_3 {

  XtndPoint() : Traits::Point_3() {}

  XtndPoint(double x, double y, double z) 
    : Traits::Point_3(x, y, z),
      texCoord(0.0, 0.0) {}

  XtndPoint(double x, double y, double z, double u, double v ) 
  : Traits::Point_3(x, y, z),
    texCoord(u, v) {}

  typedef typename Traits::Vector_2 TexCoord;
  TexCoord texCoord;

  double get_u () const {return texCoord.hx();}
  double get_v () const {return texCoord.hy();}


};

// A new items type using extended vertex
//TODO change XtndVertex back to original vertex class??
struct Custom_items : public CGAL::Polyhedron_items_3 {
    template <class Refs, class Traits>
    struct Vertex_wrapper {

      typedef XtndVertex<Refs,CGAL::Tag_true, XtndPoint<Traits>> Vertex;
    };
};


typedef CGAL::Simple_cartesian<double> Kernel;
typedef CGAL::Polyhedron_3<Kernel, Custom_items> Polyhedron;
typedef Polyhedron::HalfedgeDS HalfedgeDS;


namespace SMS = CGAL::Surface_mesh_simplification ;



//assigns a tex coord to each vertex by matching a coordinate and a tex coord that are
//used for the same vertex in the OBJ file
void build_t_coords(std::vector<double> &v, 
                    std::vector<int> &vindices, 
                    std::vector<double> &t,
                    std::vector<int> &tindices,
                    std::vector<double> &built_t_coords ){

  //for each point in vertex position list
  for (int i = 0; i < (int)v.size(); i+=3)
  {
    //find location of first vertex index reference
    int idx = i/3;
    int tloc = 0;
    std::vector<int>::iterator it;
    it = std::find(vindices.begin(), vindices.end(), idx);
    if (it != vindices.end()){
      tloc = it - vindices.begin();
    }
    else {
      std::cout << "build_t_coords: vertex not found in indices list: " << idx << std::endl;
    }

    //get tex coord corresponding to that index reference
    double u = 0.0, v = 0.0;
    if (tloc < (int)t.size() && tloc < (int)tindices.size())
    {
      int tidx = tindices[tloc];
      u = t[tidx * 2];
      v = t[(tidx * 2) + 1];
    }
    built_t_coords.insert(built_t_coords.end(), {u, v});
  }
}

// implementatio of a Polyhedron_incremental_builder_3 to create the  polyhedron
//http://jamesgregson.blogspot.com/2012/05/example-code-for-building.html
template<class HDS>
class polyhedron_builder : public CGAL::Modifier_base<HDS> {

public:
  std::vector<double> &vertices;
  std::vector<int>    &tris;
  std::vector<double> &t_coords;


  polyhedron_builder( std::vector<double> &_vertices,
                      std::vector<int> &_tris,
                      std::vector<double> &_t_coords ) 
                      : vertices(_vertices), tris(_tris), t_coords(_t_coords) {}

  void operator()( HDS& hds) {
 
    // create a cgal incremental builder
    CGAL::Polyhedron_incremental_builder_3<HDS> B( hds, true);
    B.begin_surface( vertices.size()/3, tris.size()/3 );
   
    // add the polyhedron vertices
    for( int i=0; i<(int)vertices.size() / 3; ++i ){

      //dummy textture coordinates
      B.add_vertex( XtndPoint<Kernel>( vertices[(i*3)], 
                                       vertices[(i*3)+1], 
                                       vertices[(i*3)+2] , 
                                       i,  
                                       i+1
                                       ));

      // B.add_vertex( XtndPoint<Kernel>( vertices[(i*3)], 
      //                            vertices[(i*3)+1], 
      //                            vertices[(i*3)+2] , 
      //                            t_coords[i*2],  
      //                            t_coords[(i*2)+1]));
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


void see_polyhedron(const Polyhedron& P){

  typedef typename Polyhedron::Vertex_const_iterator VCI;

  for( VCI vi = P.vertices_begin(); vi != P.vertices_end(); ++vi) {

    // int idx = vi - P.vertices_begin();
  // for( VCI vi = P.vertices_begin(); vi != P.vertices_end(); ++vi) {


    // writer.write_vertex( ::CGAL::to_double( vi->point().x()),
    //                      ::CGAL::to_double( vi->point().y()),
    //                      ::CGAL::to_double( vi->point().z()));

    double u = CGAL::to_double( vi->point().get_u());
    double v = CGAL::to_double( vi->point().get_v());
    // double z = CGAL::to_double( vi->point().z());

    std::cout << "v " << ": " << u << ' ' << v << '\n';
  }

}


int main( int argc, char** argv ) 
{
  std::string obj_filename = "dino.obj";
  if (Utils::cmdOptionExists(argv, argv+argc, "-f")) {
    obj_filename = std::string(Utils::getCmdOption(argv, argv + argc, "-f"));
  }
  else {
    std::cout << "Please provide a obj filename using -f <filename.obj>" << std::endl;
    return 1;
  }
  std::string out_filename = "data/simplified_mesh.obj";
  if (Utils::cmdOptionExists(argv, argv+argc, "-o")) {
    out_filename = std::string(Utils::getCmdOption(argv, argv + argc, "-o"));
  }


  //load OBJ into arrays
  std::vector<double> vertices;
  std::vector<int> tris;
  std::vector<double> t_coords;
  std::vector<int> tindices;
  Utils::load_obj( obj_filename, vertices, tris, t_coords, tindices);

  if (vertices.size() == 0 ) {
    std::cout << "didnt find any vertices" << std::endl;
    return 1;
  }
  std::cout << "Mesh loaded (" << vertices.size() << " vertices)" << std::endl;


  //decide the best text coord for each vertex position
  std::vector<double> built_t_coords;
  build_t_coords(vertices, tris, t_coords, tindices, built_t_coords);


  // build a polyhedron from the loaded arrays
  Polyhedron polyMesh;
  polyhedron_builder<HalfedgeDS> builder( vertices, tris, built_t_coords );
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
  SMS::Count_stop_predicate<Polyhedron> stop(50);

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

  std::cout << "Final vertices: " << polyMesh.size_of_vertices() << "\n";

  see_polyhedron(polyMesh);
        

  //write to file
  std::ofstream ofs(out_filename);
  // CGAL::print_polyhedron_wavefront(ofs, polyMesh);
  OBJ_printer::print_polyhedron_wavefront_with_tex(ofs, polyMesh);
  ofs.close();
  std::cout << "simplified mesh was written to " << out_filename << std::endl;

  
  return EXIT_SUCCESS ;      
}