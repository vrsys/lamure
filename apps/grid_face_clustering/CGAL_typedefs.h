
#include <CGAL/Simple_cartesian.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/Polyhedron_items_with_id_3.h>

#ifndef CGAL_TYPEDEFSH
#define CGAL_TYPEDEFSH

template <class Refs, class T, class P>
struct XtndVertex : public CGAL::HalfedgeDS_vertex_base<Refs, T, P>  {
// struct XtndVertex : public CGAL::HalfedgeDS_vertex_max_base_with_id<Refs, T, P>  {
    
  using CGAL::HalfedgeDS_vertex_base<Refs, T, P>::HalfedgeDS_vertex_base;
  // using CGAL::HalfedgeDS_vertex_max_base_with_id<Refs, T, P>::HalfedgeDS_vertex_max_base_with_id;

  uint32_t id;
  // uint32_t& id() {return &id;}

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
struct Custom_items : public CGAL::Polyhedron_items_with_id_3 {
    template <class Refs, class Traits>
    struct Vertex_wrapper {
      typedef XtndVertex<Refs,CGAL::Tag_true, XtndPoint<Traits>> Vertex;
      // typedef CGA:HalfedgeDS_vertex_max_base_with_id<Refs,CGAL::Tag_true, XtndPoint<Traits>> Vertex;
    };
};

typedef CGAL::Simple_cartesian<double> Kernel;

typedef Kernel::Vector_3 Vector;
typedef CGAL::Point_3<Kernel> Point;


typedef CGAL::Polyhedron_3<Kernel,Custom_items> Polyhedron;
typedef Polyhedron::HalfedgeDS HalfedgeDS;

typedef Polyhedron::Facet_iterator Facet_iterator;
typedef Polyhedron::Facet_handle Facet_handle;
typedef Polyhedron::Facet Facet; 

typedef Polyhedron::Vertex_handle Vertex_handle;

typedef Polyhedron::Halfedge_around_facet_circulator Halfedge_facet_circulator;


typedef Polyhedron::Halfedge Halfedge;
typedef Polyhedron::Edge_iterator Edge_iterator;


typedef boost::graph_traits<Polyhedron>::vertex_descriptor vertex_descriptor;
typedef boost::graph_traits<Polyhedron>::face_descriptor   face_descriptor;
typedef boost::graph_traits<Polyhedron>::face_iterator face_iterator;


#endif