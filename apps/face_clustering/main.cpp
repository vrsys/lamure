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
#include <CGAL/Surface_mesh_simplification/Policies/Edge_collapse/Constrained_placement.h>

#include<CGAL/Polyhedron_incremental_builder_3.h>


#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/trim.hpp>


#include <scm/core.h>
#include <scm/core/math.h>

#include "Utils.h"
#include "OBJ_printer.h"

/*

CUSTOM DATA STRUCTURES ===================================================

*/

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

template <class Refs>
struct XtndFace : public CGAL::HalfedgeDS_face_base<Refs> {
  bool in_left_node;

  void set_in_left_node(bool _in_left_node) {
    in_left_node = _in_left_node;
  }
  bool is_in_left_node() {return in_left_node;}
};

// A new items type using extended vertex
//TODO change XtndVertex back to original vertex class??
struct Custom_items : public CGAL::Polyhedron_items_3 {
    template <class Refs, class Traits>
    struct Vertex_wrapper {
      typedef XtndVertex<Refs,CGAL::Tag_true, XtndPoint<Traits>> Vertex;
    };

    template <class Refs, class Traits>
    struct Face_wrapper {
      typedef XtndFace<Refs> Face;
    };
};


typedef CGAL::Simple_cartesian<double> Kernel;
typedef CGAL::Polyhedron_3<Kernel, Custom_items> Polyhedron;
typedef Polyhedron::HalfedgeDS HalfedgeDS;

//for property map
typedef boost::graph_traits<Polyhedron>::face_descriptor face_descriptor;
typedef boost::graph_traits<Polyhedron>::face_iterator face_iterator;
typedef std::map<face_descriptor,bool> Face_is_left_map;
typedef boost::associative_property_map<Face_is_left_map> Face_is_left_pmap;


namespace SMS = CGAL::Surface_mesh_simplification ;


/*
OTHER STRUCTS ===================================================
*/

//
// BGL property map which indicates whether an edge is marked as non-removable
//
struct Border_is_constrained_edge_map {
  const Polyhedron* sm_ptr;
  const Face_is_left_pmap fm_ptr;
  typedef boost::graph_traits<Polyhedron>::edge_descriptor key_type;
  typedef boost::graph_traits<Polyhedron>::halfedge_descriptor HE;
  typedef boost::graph_traits<Polyhedron>::face_descriptor Face;

  typedef bool value_type;
  typedef value_type reference;
  typedef boost::readable_property_map_tag category;

  int constrained_edges = 0;


  Border_is_constrained_edge_map(const Polyhedron& sm, const Face_is_left_pmap& fmap)
  : sm_ptr(&sm), fm_ptr(fmap)
  {}

  friend bool get(Border_is_constrained_edge_map m, const key_type& edge) {

    //get descriptors for adjacent faces
    HE he1 = halfedge(edge, *m.sm_ptr);
    HE he2 = opposite(he1, *m.sm_ptr);
    Face f1 = face(he1, *m.sm_ptr);
    Face f2 = face(he2, *m.sm_ptr);

    //border is constrained if the faces ar in different sides
    bool constrained = (m.fm_ptr[f1] != m.fm_ptr[f2]);
    if (constrained) {m.constrained_edges++;}
    return constrained;
  }

//not currently working
  // int num_constrained_edges(){
  //   return constrained_edges;
  // }
};

// Placement class
typedef SMS::Constrained_placement<SMS::Midpoint_placement<Polyhedron>, Border_is_constrained_edge_map > Placement;


/*
 ===================================================
*/


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

  //how many of the given triangles are added to the final mesh
  //to enable creation of non-manifold meshes 
  double mesh_proportion = 1.0;

  std::string report = "no report";


  polyhedron_builder( std::vector<double> &_vertices,
                      std::vector<int> &_tris,
                      std::vector<double> &_t_coords ) 
                      : vertices(_vertices), tris(_tris), t_coords(_t_coords) {}

  void set_mesh_proportion(const double _mp){
    mesh_proportion = _mp;
  }

  std::string get_report () {return report;}

  void operator()( HDS& hds) {

    typedef CGAL::Point_3<Kernel> Point;

    std::vector<Point> centroids;
    for( int i=0; i<(int)(tris.size()); i+=3 ){
      Point p = CGAL::centroid(Point(vertices[tris[i]*3],vertices[(tris[i]*3)+1],vertices[(tris[i]*3)+2]),
                             Point(vertices[tris[i+1]*3],vertices[(tris[i+1]*3)+1],vertices[(tris[i+1]*3)+2]),
                             Point(vertices[tris[i+2]*3],vertices[(tris[i+2]*3)+1],vertices[(tris[i+2]*3)+2]));

      centroids.push_back(p);
    }

    double avg_x = (1.0/centroids.size()) * std::accumulate(centroids.begin(), centroids.end(), 0.0, 
                                              [](double a, Point b){
                                                return a + b.x();
                                              });

 
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

    // int total_lefties = 0;
   
    // add the polyhedron triangles
    for( int i=0; i<(int)(tris.size() * mesh_proportion); i+=3 ){

      bool is_left_node = (centroids[i/3].x() < avg_x);

      HalfedgeDS::Face_handle face = B.begin_facet();
      B.add_vertex_to_facet( tris[i+0] );
      B.add_vertex_to_facet( tris[i+1] );
      B.add_vertex_to_facet( tris[i+2] );
      B.end_facet();

      face->set_in_left_node(is_left_node);
      
      // if (is_left_node)
      // {
      //   total_lefties++;
      // }


    }
   
    // finish up the surface
    // B.end_surface();

    // std::stringstream ss;
    // ss << "created mesh with " << tris.size()/3 << " triangles: " << total_lefties << " in left node";
    // report = ss.str();

    }


};


// void see_polyhedron(const Polyhedron& P){

//   typedef typename Polyhedron::Vertex_const_iterator VCI;

//   for( VCI vi = P.vertices_begin(); vi != P.vertices_end(); ++vi) {

//     double u = CGAL::to_double( vi->point().get_u());
//     double v = CGAL::to_double( vi->point().get_v());

//     std::cout << "v " << ": " << u << ' ' << v << '\n';
//   }

// }


int main( int argc, char** argv ) 
{
  std::string obj_filename = "dino.obj";
  if (Utils::cmdOptionExists(argv, argv+argc, "-f")) {
    obj_filename = std::string(Utils::getCmdOption(argv, argv + argc, "-f"));
  }
  else {
    std::cout << "Please provide an obj filename using -f <filename.obj>" << std::endl;
    return 1;
  }

  std::string out_filename = "data/simplified_mesh.obj";
  if (Utils::cmdOptionExists(argv, argv+argc, "-o")) {
    out_filename = std::string(Utils::getCmdOption(argv, argv + argc, "-o"));
  }

  int target_edges = 1000;
  if(Utils::cmdOptionExists(argv, argv+argc, "-e")){
    target_edges = stoi(std::string(Utils::getCmdOption(argv, argv + argc, "-e")));
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

  // std::cout << builder.get_report() << std::endl;

  if (polyMesh.is_valid(true)){
    std::cout << "mesh valid\n"; 
  }


  if (!CGAL::is_triangle_mesh(polyMesh)){
    std::cerr << "Input geometry is not triangulated." << std::endl;
    return EXIT_FAILURE;
  }




  int total_lefties = 0;

  //test if faces have been stored
  for( Polyhedron::Facet_iterator fb = polyMesh.facets_begin()
   , fe = polyMesh.facets_end()
   ; fb != fe
   ; ++ fb
   ) {

    if (fb->is_in_left_node()){

      total_lefties++;
    }
  }

  std::cout << "total num faces: " << polyMesh.size_of_facets() << std::endl;
  std::cout << "total lefties found in built mesh = " << total_lefties << std::endl;
    

  // This is a stop predicate (defines when the algorithm terminates).
  // In this example, the simplification stops when the number of undirected edges
  // left in the surface mesh drops below the specified number (1000)
  //target edges set from input
  SMS::Count_stop_predicate<Polyhedron> stop(target_edges);


  //build property map
  Face_is_left_map face_is_left_map;
  Face_is_left_pmap face_is_left_pmap(face_is_left_map);

  typedef CGAL::Point_3<Kernel> Point;
  std::vector<Point> centroids;

  // for each face, associate bools to faces depending on position
  face_iterator fb, fe;
  for(boost::tie(fb, fe)=faces(polyMesh); fb!=fe; ++fb){

    //calculate centroid for each face
    CGAL::Vertex_around_face_iterator<Polyhedron> vbegin, vend;
    boost::tie(vbegin, vend) = vertices_around_face(halfedge(*fb, polyMesh), polyMesh);
    Point p1 = (*vbegin)->point();  ++vbegin;
    Point p2 = (*vbegin)->point();  ++vbegin;
    Point p3 = (*vbegin)->point();  ++vbegin;
    Point centroid = CGAL::centroid(p1,p2,p3);
    centroids.push_back(centroid);
  }
  //get average centroid
  double avg_x = (1.0/centroids.size()) * std::accumulate(centroids.begin(), centroids.end(), 0.0, 
                                          [](double a, Point b){
                                            return a + b.x();
                                          });

  //fill property map with boolean
  int index = 0;
  for(boost::tie(fb, fe)=faces(polyMesh); fb!=fe; ++fb){
    bool is_left = (centroids[index].x() < avg_x);
    face_is_left_pmap[*fb] = is_left;
    index++;
  }


  // map that defines which edges are protected
  Border_is_constrained_edge_map bem(polyMesh, face_is_left_pmap);


  std::cout << "Starting simplification" << std::endl;
  
  // This the actual call to the simplification algorithm.
  // The surface mesh and stop conditions are mandatory arguments.
  // The index maps are needed because the vertices and edges
  // of this surface mesh lack an "id()" field.
  SMS::edge_collapse
            (polyMesh
            ,stop
             ,CGAL::parameters::vertex_index_map(get(CGAL::vertex_external_index,polyMesh)) 
                               .halfedge_index_map  (get(CGAL::halfedge_external_index  ,polyMesh))
                               .edge_is_constrained_map(bem)
                               .get_placement(Placement(bem))
            );

  // SMS::edge_collapse
  //           (polyMesh
  //           ,stop
  //            ,CGAL::parameters::vertex_index_map(get(CGAL::vertex_external_index,polyMesh)) 
  //                              .halfedge_index_map  (get(CGAL::halfedge_external_index  ,polyMesh))
  //                              .get_cost (SMS::Edge_length_cost <Polyhedron>())
  //                              .get_placement(SMS::Midpoint_placement<Polyhedron>())
  //           );


  
  std::cout << "\nFinished...\n" << (polyMesh.size_of_halfedges()/2) << " final edges.\n" ;


  std::cout << "constrained edges : " << bem.num_constrained_edges() << std::endl;
        

  //write to file
  std::ofstream ofs(out_filename);
  OBJ_printer::print_polyhedron_wavefront_with_tex(ofs, polyMesh);
  ofs.close();
  std::cout << "simplified mesh was written to " << out_filename << std::endl;

  
  return EXIT_SUCCESS ;      
}