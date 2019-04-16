
#ifndef LAMURE_MESH_POLYHEDRON_H_
#define LAMURE_MESH_POLYHEDRON_H_

#include <lamure/types.h>
#include <lamure/mesh/triangle.h>
#include <lamure/mesh/triangle_chartid.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <limits>

#include <CGAL/Simple_cartesian.h>
#include <CGAL/Polyhedron_3.h>

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

namespace lamure {
namespace mesh {


/*

CUSTOM DATA STRUCTURES ===================================================

*/

typedef CGAL::Simple_cartesian<double> Kernel;
typedef Kernel::Vector_2 TexCoord;
typedef Kernel::Vector_3 Vec3;
typedef Kernel::Point_3 Point;

template <class Refs, class Traits>
struct ChartFace : public CGAL::HalfedgeDS_face_base<Refs> {
  int chart_id;
  int face_id;
  int tex_id;
  int area_id;

  // typedef typename Traits::Vector_2 TexCoord;
  TexCoord t_coords[3];

  //save tex coords, assume order is the same as the added order of vertices
  void add_tex_coords(TexCoord tc0, TexCoord tc1, TexCoord tc2){
    t_coords[0] = tc0;
    t_coords[1] = tc1;
    t_coords[2] = tc2;
  }
};

//declare a halfedge which also includes an id
template<class Refs>
struct Edge_with_id : public CGAL::HalfedgeDS_halfedge_base<Refs> {
  int edge_id;
};

// // //define a new vertex type - inheriting from base type CGAL::HalfedgeDS_vertex_base
//ref https://doc.cgal.org/4.7/Polyhedron/index.html#title11
template <class Refs, class T, class P>
struct XtndVertex : public CGAL::HalfedgeDS_vertex_base<Refs, T, P>  {
    
  using CGAL::HalfedgeDS_vertex_base<Refs, T, P>::HalfedgeDS_vertex_base;

};

struct UV_Candidate {
  TexCoord tex_;
  Vec3 v_prev;
  Vec3 v_next;
};

//extended point class
template <class Traits>
struct XtndPoint : public Traits::Point_3 {

  typedef typename Traits::Point_3 Point;

  XtndPoint() : Traits::Point_3() {}

  XtndPoint(double x, double y, double z) 
    : Traits::Point_3(x, y, z),
      texCoord(0.0, 0.0) {}

  XtndPoint(double x, double y, double z, double u, double v ) 
  : Traits::Point_3(x, y, z),
    texCoord(u, v) {}

  // typedef typename Traits::Vector_2 TexCoord;
  TexCoord texCoord;

  double get_u () const {return texCoord.hx();}
  double get_v () const {return texCoord.hy();}

  std::vector<UV_Candidate> uv_candidates;

  void add_uv_candidate(UV_Candidate uvc){
    uv_candidates.push_back(uvc);
  }


  static bool float_safe_equals(Point p1, Point p2){
    double eps = std::numeric_limits<double>::epsilon();
    // double eps = 0.0000001;

    if (std::abs(p1.x() - p2.x()) > eps
      || std::abs(p1.y() - p2.y()) > eps
      || std::abs(p1.z() - p2.z()) > eps)
    {
      return false;
    }
    return true;
  }
};



template <class Traits>
std::ostream& operator << (std::ostream& os, const XtndPoint<Traits> &pnt)  
{  
  os << pnt.x() << ", " << pnt.y() << ", " << pnt.z();  
  return os;  
} 

// A new items type using extended vertex
//TODO change XtndVertex back to original vertex class??
struct Custom_items : public CGAL::Polyhedron_items_3 {
    template <class Refs, class Traits>
    struct Vertex_wrapper {
      typedef XtndVertex<Refs,CGAL::Tag_true, XtndPoint<Traits>> Vertex;
    };

    template < class Refs, class Traits>
    struct Halfedge_wrapper {
    typedef Edge_with_id<Refs> Halfedge;
    };

    template <class Refs, class Traits>
    struct Face_wrapper {
      typedef ChartFace<Refs, Traits> Face;
    };
};


typedef CGAL::Polyhedron_3<Kernel, Custom_items> Polyhedron;
typedef Polyhedron::HalfedgeDS HalfedgeDS;
typedef Polyhedron::Halfedge_handle Edge_handle;

typedef Polyhedron::Facet_handle Facet_handle;

namespace SMS = CGAL::Surface_mesh_simplification ;



// struct that allows lableling and constraining of border edges
struct Border_is_constrained_edge_map {
  const Polyhedron* sm_ptr;
  typedef boost::graph_traits<Polyhedron>::edge_descriptor key_type;
  typedef boost::graph_traits<Polyhedron>::halfedge_descriptor HE;
  typedef boost::graph_traits<Polyhedron>::face_descriptor Face;

  typedef bool value_type;
  // typedef value_type reference;
  // typedef boost::readable_property_map_tag category;


  Border_is_constrained_edge_map(const Polyhedron& sm)
  : sm_ptr(&sm)
  {}

  friend bool get(Border_is_constrained_edge_map m, const key_type& edge) {
    return CGAL::is_border(edge, *m.sm_ptr);
  }

};

// Placement class
typedef SMS::Constrained_placement<SMS::Midpoint_placement<Polyhedron>, Border_is_constrained_edge_map > Placement;







// implementatio of a Polyhedron_incremental_builder_3 to create the  polyhedron
//http://jamesgregson.blogspot.com/2012/05/example-code-for-building.html
template<class HDS>
class polyhedron_builder : public CGAL::Modifier_base<HDS> {

public:
  std::vector<Triangle_Chartid> &combined_set;

  //how many of the given triangles are added to the final mesh
  //to enable creation of non-manifold meshes 
  double mesh_proportion = 1.0;


  polyhedron_builder(
    std::vector<Triangle_Chartid>& _combined_set) 
    : combined_set(_combined_set){}

  void set_mesh_proportion(const double _mp){
    mesh_proportion = _mp;
  }

  void operator()(HDS& hds) {
 
    // create a cgal incremental builder
    CGAL::Polyhedron_incremental_builder_3<HDS> B(hds, true);
    uint32_t num_tris = combined_set.size();
    B.begin_surface(3*num_tris, num_tris);


    //create indexed vertex list
    std::vector<XtndPoint<Kernel> > vertices;
    std::vector<uint32_t> tris;
    create_indexed_triangle_list(combined_set, tris, vertices);
    
    //add vertices of surface
    for (uint32_t i = 0; i < vertices.size(); ++i) {
      B.add_vertex(vertices[i]);
    }


    //create faces using vertex index references
    for (uint32_t i = 0; i < tris.size(); i+=3) {

      B.begin_facet();
      B.add_vertex_to_facet(tris[i]);
      B.add_vertex_to_facet(tris[i+1]);
      B.add_vertex_to_facet(tris[i+2]);
      B.end_facet();

    }

    B.end_surface();
  }

  //creates indexed triangles list (indexes and vertices) from triangle list
  void create_indexed_triangle_list(std::vector<lamure::mesh::Triangle_Chartid>& input_triangles,
                                    std::vector<uint32_t>& tris,
                                    std::vector<XtndPoint<Kernel> >& vertices) {

    //for each vertex in each triangle
    for (const lamure::mesh::Triangle_Chartid tri : input_triangles){
      for (int v = 0; v < 3; ++v)
      {
        //get Point value
        const XtndPoint<Kernel> tri_pnt = XtndPoint<Kernel>(
                tri.getVertex(v).pos_.x, 
                tri.getVertex(v).pos_.y, 
                tri.getVertex(v).pos_.z,
                tri.getVertex(v).tex_.x,
                tri.getVertex(v).tex_.y);

        //compare point with all previous vertices
        //go backwards, neighbours are more likely to be added at the end of the list
        uint32_t vertex_id = vertices.size();
        for (int32_t p = (vertices.size() - 1); p >= 0; --p)
        {
          //if a match is found, record index 
          // if (tri_pnt == vertices[p])
          if (XtndPoint<Kernel>::float_safe_equals(tri_pnt, vertices[p]))
          {
            vertex_id = p;
            break;
          }
        }
        //if no match then add to vertices list
        if (vertex_id == vertices.size()){
          vertices.push_back(tri_pnt);
        }
        //store index
        tris.push_back(vertex_id);
      }
    }

    // assign_vertex_candidates(input_triangles, tris, vertices);
  }

  // void assign_vertex_candidates(std::vector<lamure::mesh::Triangle_Chartid>& input_triangles,
  //                                   std::vector<uint32_t>& tris,
  //                                   std::vector<XtndPoint<Kernel> >& vertices) {
  //   //for all vertex indices of triangles
  //   for (int i = 0; i < tris.size(); i++){

  //     //get uv and edge directions from that input_triangle
  //     auto& 
  //   }
  // }

};

struct OBJ_printer
{

  static void write_vertex(std::ostream& out, const double& x, const double& y, const double& z) {
      out << "v " << x << ' ' << y <<  ' ' << z << '\n';
  }

  template <class Polyhedron>
  static void
  print_polyhedron( std::ostream& out, const Polyhedron& P, std::string filename) {

      typedef typename Polyhedron::Vertex_const_iterator                  VCI;
      typedef typename Polyhedron::Facet_const_iterator                   FCI;
      typedef typename Polyhedron::Halfedge_around_facet_const_circulator HFCC;

      // Print header.
      out << "# " << filename << "\n#\n\n";

      //print vertices
      for( VCI vi = P.vertices_begin(); vi != P.vertices_end(); ++vi) {
          write_vertex(out, ::CGAL::to_double( vi->point().x()),
                               ::CGAL::to_double( vi->point().y()),
                               ::CGAL::to_double( vi->point().z()));
      }

      out << std::endl;


      typedef CGAL::Inverse_index< VCI> Index;
      Index index( P.vertices_begin(), P.vertices_end());

      //faces
      for( FCI fi = P.facets_begin(); fi != P.facets_end(); ++fi) {
        HFCC hc = fi->facet_begin();
        HFCC hc_end = hc;
        std::size_t n = circulator_size( hc);
        CGAL_assertion( n >= 3);

        out << "f";
        do {
            out << ' ' << (index[ VCI(hc->vertex())]) + 1;
            ++hc;
        } while( hc != hc_end);
        out << std::endl;
      }
    }
  };



}
}


#endif
