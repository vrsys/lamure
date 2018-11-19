
#ifndef LAMURE_MESH_POLYHEDRON_H_
#define LAMURE_MESH_POLYHEDRON_H_

#include <lamure/types.h>
#include <lamure/mesh/triangle.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

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


// implementatio of a Polyhedron_incremental_builder_3 to create the  polyhedron
//http://jamesgregson.blogspot.com/2012/05/example-code-for-building.html
template<class HDS>
class polyhedron_builder : public CGAL::Modifier_base<HDS> {

public:
  std::vector<triangle_t> &left_tris_;
  std::vector<triangle_t> &right_tris_;

  //how many of the given triangles are added to the final mesh
  //to enable creation of non-manifold meshes 
  double mesh_proportion = 1.0;

  polyhedron_builder(
  	std::vector<triangle_t>& left_child_tris,
    std::vector<triangle_t>& right_child_tris) 
    : left_tris_(left_child_tris), right_tris_(right_child_tris) {}

  void set_mesh_proportion(const double _mp){
    mesh_proportion = _mp;
  }

  void operator()(HDS& hds) {
 
    // create a cgal incremental builder
    CGAL::Polyhedron_incremental_builder_3<HDS> B(hds, true);
    uint32_t num_tris = left_tris_.size()+right_tris_.size();
    B.begin_surface(3*num_tris, num_tris);
    
    //add the polyhedron vertices
    uint32_t vertex_id = 0;
    for (uint32_t i = 0; i < left_tris_.size(); ++i) {
      const triangle_t& tri = left_tris_[i];
      B.add_vertex(XtndPoint<Kernel>(tri.v0_.pos_.x, tri.v0_.pos_.y, tri.v0_.pos_.z));
      B.add_vertex(XtndPoint<Kernel>(tri.v1_.pos_.x, tri.v1_.pos_.y, tri.v1_.pos_.z));
      B.add_vertex(XtndPoint<Kernel>(tri.v2_.pos_.x, tri.v2_.pos_.y, tri.v2_.pos_.z));

      B.begin_facet();
      B.add_vertex_to_facet(vertex_id);
      B.add_vertex_to_facet(vertex_id+1);
      B.add_vertex_to_facet(vertex_id+2);
      B.end_facet();

      vertex_id += 3;
    }

    for (uint32_t i = 0; i < right_tris_.size(); ++i) {
      const triangle_t& tri = right_tris_[i];
      B.add_vertex(XtndPoint<Kernel>(tri.v0_.pos_.x, tri.v0_.pos_.y, tri.v0_.pos_.z));
      B.add_vertex(XtndPoint<Kernel>(tri.v1_.pos_.x, tri.v1_.pos_.y, tri.v1_.pos_.z));
      B.add_vertex(XtndPoint<Kernel>(tri.v2_.pos_.x, tri.v2_.pos_.y, tri.v2_.pos_.z));

      B.begin_facet();
      B.add_vertex_to_facet(vertex_id);
      B.add_vertex_to_facet(vertex_id+1);
      B.add_vertex_to_facet(vertex_id+2);
      B.end_facet();

      vertex_id += 3;
    }
   
    // finish up the surface
    B.end_surface();
  }
};



}
}


#endif
