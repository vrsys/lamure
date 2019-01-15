#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <chrono>
#include <limits>
#include <float.h>

#include <CGAL/Simple_cartesian.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/Polyhedron_items_with_id_3.h>



#include <CGAL/IO/print_wavefront.h>

// Simplification function
#include <CGAL/Surface_mesh_simplification/edge_collapse.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/property_map.h>


#include <CGAL/Polygon_mesh_processing/measure.h>
// Stop-condition policy
#include <CGAL/Surface_mesh_simplification/Policies/Edge_collapse/Count_stop_predicate.h>
#include <CGAL/Surface_mesh_simplification/Policies/Edge_collapse/Edge_length_cost.h>
#include <CGAL/Surface_mesh_simplification/Policies/Edge_collapse/Midpoint_placement.h>
#include <CGAL/Surface_mesh_simplification/Policies/Edge_collapse/Constrained_placement.h>

#include<CGAL/Polyhedron_incremental_builder_3.h>

#include <CGAL/boost/graph/Dual.h>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/trim.hpp>

#include "Utils.h"
#include "OBJ_printer.h"
#include "SymMat.h"

#include "eig.h"



#define SEPARATE_CHART_FILE false

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
struct Custom_items : public CGAL::Polyhedron_items_with_id_3 {
    template <class Refs, class Traits>
    struct Vertex_wrapper {
      typedef XtndVertex<Refs,CGAL::Tag_true, XtndPoint<Traits>> Vertex;
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

typedef Polyhedron::Halfedge_around_facet_circulator Halfedge_facet_circulator;


typedef Polyhedron::Halfedge Halfedge;
typedef Polyhedron::Edge_iterator Edge_iterator;


typedef boost::graph_traits<Polyhedron>::vertex_descriptor vertex_descriptor;
typedef boost::graph_traits<Polyhedron>::face_descriptor   face_descriptor;
typedef boost::graph_traits<Polyhedron>::face_iterator face_iterator;


namespace SMS = CGAL::Surface_mesh_simplification ;


template<class HDS>
class polyhedron_builder : public CGAL::Modifier_base<HDS> {

public:
  std::vector<double> &vertices;
  std::vector<int>    &tris;
  std::vector<double> &tcoords;
  std::vector<int>    &tindices;

  bool CHECK_VERTICES;

  polyhedron_builder( std::vector<double> &_vertices,
                      std::vector<int> &_tris,
                      std::vector<double> &_tcoords,
                      std::vector<int> &_tindices,
                      bool _CHECK_VERTICES) 
                      : vertices(_vertices), tris(_tris), tcoords(_tcoords), tindices(_tindices), CHECK_VERTICES(_CHECK_VERTICES)  {}


  //Returns false for error
  void operator()( HDS& hds) {


    //if ids of vertex positions and texture coordinates do not match, then they are matched manually
    //this only selects the first texture coordinate associated with a vertex position - doesn't support multiple tex coords that share a vertex position 
    bool vertices_match = true;
    if (CHECK_VERTICES)
    {
      //check if vertex indices and tindices are the same
      if ( (vertices.size()/3) != (tcoords.size() / 2)) {std::cerr << "Input Texture coords not matched with input (different quantities)\n";
          vertices_match = false;}
      else {
        for (uint32_t i = 0; i < tindices.size(); ++i)
        {
          if (tindices[i] != tris[i]){
            std::cerr << "Input Texture coords not matched with input (different indices)\n";
            vertices_match = false;
            break;
          }
        }
      }
    }
    if (!vertices_match)
    {

      //if no incoming coords, fil with 0s
      if (tcoords.size()==0)
      {
        for (uint32_t i = 0; i < ((vertices.size()/3)*2); ++i)
        {
          tcoords.push_back(0);
        }
      }
      else {

        //match vertex indexes to texture indexes to create new texture array
        std::vector<double> matched_tex_coords;

        //for each vertex entry
        for (uint32_t i = 0; i < (vertices.size() / 3); ++i)
        {
          //find first use of vertex in tris array
          for (uint32_t t = 0; t < tris.size(); ++t)
          {
            if (tris[t] == (int)i)
            {
              int target_face_vertex = t;
              int matching_tex_coord_idx = tindices[target_face_vertex];
              matched_tex_coords.push_back(tcoords[matching_tex_coord_idx*2]);
              matched_tex_coords.push_back(tcoords[(matching_tex_coord_idx*2) + 1]);
              break;
            }
          }
        }

        tcoords = matched_tex_coords;

        //check each vertex has coords
        if ((vertices.size()/3) == (tcoords.size() / 2)){
          std::cout << "matched texture coords with vertices\n";
        }
        else {
          std::cout << "failed to matched texture coords with vertices (" << (vertices.size()/3) << " vertices, " << (tcoords.size() / 2) << " coords)\n";
        }
      }
    }

    // create a cgal incremental builder
    CGAL::Polyhedron_incremental_builder_3<HDS> B( hds, true);
    B.begin_surface( vertices.size()/3, tris.size()/3 );
   
    // add the polyhedron vertices
    for( int i=0; i<(int)vertices.size() / 3; ++i ){



      B.add_vertex( XtndPoint<Kernel>( vertices[(i*3)], 
                           vertices[(i*3)+1], 
                           vertices[(i*3)+2],
                            tcoords[i*2],
                            tcoords[(i*2)+1]));

    }

    // add the polyhedron triangles
    for( int i=0; i<(int)(tris.size()); i+=3 ){

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

struct CLUSTER_SETTINGS
{
  double e_fit_cf;
  double e_ori_cf;
  double e_shape_cf;

  CLUSTER_SETTINGS(double ef, double eo, double es){
    e_fit_cf = ef;
    e_ori_cf = eo;
    e_shape_cf = es;
  }
};

  
struct ErrorQuadric {

  SymMat A;
  Vector b;
  double c;

  ErrorQuadric() {
    b = Vector(0,0,0);
    c = 0;
  }

  //for p quad
  ErrorQuadric(Point& p) {
    A = SymMat(p);
    b = Vector(p.x(), p.y(), p.z());
    c = 1;
  }

  //for r quad
  ErrorQuadric( const Vector& v){
    A = SymMat(v);
    b = -v;
    c = 1;
  }

  ErrorQuadric operator+(const ErrorQuadric& e){
    ErrorQuadric eq;
    eq.A = A + e.A; 
    eq.b = b + e.b;
    eq.c = c + e.c;

    return eq;
  }

  ErrorQuadric operator*(const double rhs){
    ErrorQuadric result = *this;

    result.A = result.A * rhs;
    result.b = result.b * rhs;
    result.c = result.c * rhs;

    return result;
  }

  //create covariance / 'Z' matrix 
  // A - ( (b*bT) / c)
  void get_covariance_matrix(double cm[3][3]) {

    SymMat rhs = SymMat(b) / c;
    SymMat result = A - rhs;
    result.to_c_mat3(cm);
  }

  std::string print(){

    std::stringstream ss;
    ss << "A:\n" << A.print_mat();
    ss << "b:\n[" << b.x() << " " << b.y() << " " << b.z() << "]" << std::endl;
    ss << "c:\n" << c << std::endl;
    return ss.str();
  }
};

//retrieves eigenvector corresponding to lowest eigenvalue
//which is taken as the normal of the best fitting plane, given an error quadric corresponding to that plane
void get_best_fit_plane( ErrorQuadric eq, Vector& plane_normal, double& scalar_offset) {

  //get covariance matrix (Z matrix) from error quadric
  double Z[3][3];
  eq.get_covariance_matrix(Z);

  //do eigenvalue decomposition
  double eigenvectors[3][3] = {0};
  double eigenvalues[3] = {0};
  eig::eigen_decomposition(Z,eigenvectors,eigenvalues);

  //find min eigenvalue
  double min_ev = DBL_MAX;
  int min_loc;
  for (int i = 0; i < 3; ++i)
  {
    if (eigenvalues[i] < min_ev){
      min_ev = eigenvalues[i];
      min_loc = i;
    }
  }

  plane_normal = Vector(eigenvectors[0][min_loc], eigenvectors[1][min_loc], eigenvectors[2][min_loc]);

  // std::cout << "plane normal: " << plane_normal.x() << ", " << plane_normal.y() << ", " << plane_normal.z() << std::endl;
  // std::cout << "ev1: " << eigenvectors[0][0] << ", " << eigenvectors[1][0] << ", " << eigenvectors[2][0] << std::endl;
  // std::cout << "ev2: " << eigenvectors[0][1] << ", " << eigenvectors[1][1] << ", " << eigenvectors[2][1] << std::endl;
  // std::cout << "ev3: " << eigenvectors[0][2] << ", " << eigenvectors[1][2] << ", " << eigenvectors[2][2] << std::endl;


  //d is described specified as:  d =  (-nT*b)   /     c
  scalar_offset = (-plane_normal * eq.b) / eq.c;

}

Vector normalise(Vector v) {return v / std::sqrt(v.squared_length());}


//key: face_id, value: chart_id
std::map<uint32_t, uint32_t> chart_id_map;

// struct to hold a vector of facets that make a chart
struct Chart
{
  uint32_t id;
  std::vector<Facet> facets;
  std::vector<Vector> normals;
  std::vector<double> areas;
  bool active;
  Vector avg_normal; 
  double area;
  double perimeter;

  bool has_border_edge;

  ErrorQuadric P_quad; // point error quad
  ErrorQuadric R_quad; // orientation error quad

  Chart(uint32_t _id, Facet f,const Vector normal,const double _area){
    facets.push_back(f);
    normals.push_back(normal);
    areas.push_back(_area);
    active = true;
    area = _area;
    id = _id;
    avg_normal = normal;

    bool found_mesh_border = false;
    perimeter = get_face_perimeter(f, found_mesh_border);
    has_border_edge = found_mesh_border;

    P_quad = createPQuad(f);
    R_quad = ErrorQuadric(normalise(normal));
  }

  //create a combined quadric for 3 vertices of a face
  ErrorQuadric createPQuad(Facet &f){

    ErrorQuadric face_quad;
    Halfedge_facet_circulator he = f.facet_begin();
    CGAL_assertion( CGAL::circulator_size(he) >= 3);
    do {
      face_quad = face_quad + ErrorQuadric(he->vertex()->point());
    } while ( ++he != f.facet_begin());

    return face_quad;
  }


  //concatenate data from mc (merge chart) on to data from this chart
  void merge_with(Chart &mc, const double cost_of_join){


    perimeter = get_chart_perimeter(*this, mc);

    facets.insert(facets.end(), mc.facets.begin(), mc.facets.end());
    normals.insert(normals.end(), mc.normals.begin(), mc.normals.end());
    areas.insert(areas.end(), mc.areas.begin(), mc.areas.end());

    P_quad = P_quad + mc.P_quad;
    R_quad = (R_quad*area) + (mc.R_quad*mc.area);

    area += mc.area;

    //recalculate average vector
    Vector v(0,0,0);
    for (uint32_t i = 0; i < normals.size(); ++i)
    {
      v += (normals[i] * areas[i]);
    }
    avg_normal = v / area;


  }

  //returns error of points in a joined chart , averga e distance from best fitting plane
  //returns plane normal for e_dir error term
  static double get_fit_error(Chart& c1, Chart& c2, Vector& fit_plane_normal){

    ErrorQuadric fit_quad = c1.P_quad + c2.P_quad;

    Vector plane_normal;
    double scalar_offset;
    get_best_fit_plane( fit_quad, plane_normal, scalar_offset);

    double e_fit = (plane_normal * (fit_quad.A * plane_normal))
                + (2 * (fit_quad.b * (scalar_offset * plane_normal)))
                + (fit_quad.c * scalar_offset * scalar_offset);


    e_fit = e_fit / ((c1.facets.size() * 3) + (c2.facets.size() * 3));
    //divide by number of points in combined chart
    //TODO does it need to be calculated exactly or just faces * 3 ?

    //check against sign of average normal to get sign of plane_normal
    Vector avg_normal = normalise((c1.avg_normal * c1.area) + (c2.avg_normal * c2.area));
    double angle_between_vectors = acos(avg_normal * plane_normal);

    // std::cout << "avg normal: " << avg_normal.x() << ", " << avg_normal.y() << ", " << avg_normal.z() << std::endl;
    // std::cout << "angle between = " << angle_between_vectors << std::endl;

    if (angle_between_vectors > (0.5 * M_PI))
    {
      fit_plane_normal = -plane_normal;
    }
    else {
      fit_plane_normal = plane_normal;
    }

    //test - scale by area of new plane
    // e_fit /= (c1.area + c2.area);

    // std::cout << "plane normal: " << plane_normal.x() << ", " << plane_normal.y() << ", " << plane_normal.z() << std::endl;

    return e_fit;

  }
  // as defined in Garland et al 2001
  static double get_compactness_of_merged_charts(Chart& c1, Chart& c2){


    double irreg1 = get_irregularity(c1.perimeter, c1.area);
    double irreg2 = get_irregularity(c2.perimeter, c2.area);
    double irreg_new = get_irregularity(get_chart_perimeter(c1,c2), c1.area + c2.area);

    double shape_penalty = ( irreg_new - std::max(irreg1, irreg2) ) / irreg_new;

    shape_penalty /= (c1.area + c2.area);


    // std::cout << "Shape penalty = " << shape_penalty << std::endl;

    return shape_penalty;
  }

    // as defined in Garland et al 2001
  static double get_irregularity(double perimeter, double area){

    // std::cout << "P = " << perimeter << ", area = " << area << std::endl;
    // std::cout << "Irreg = " << (perimeter*perimeter) / (4 * M_PI * area) << std::endl;


    return (perimeter*perimeter) / (4 * M_PI * area);
  }

  //adds edge lengths of a face
  //check functionality for non-manifold edges
  static double get_face_perimeter(Facet& f, bool& found_mesh_border){

    Halfedge_facet_circulator he = f.facet_begin();
    CGAL_assertion( CGAL::circulator_size(he) >= 3);
    double accum_perimeter = 0;

    //for 3 edges (adjacent faces)
    do {

      //check for border
      if ( !(he->is_border()) && !(he->opposite()->is_border()) ){
        found_mesh_border = true;
      }

          accum_perimeter += edge_length(he);
    } while ( ++he != f.facet_begin());

    return accum_perimeter;
  }

  //calculate perimeter of chart
  // associate a perimeter with each cluster
  // then calculate by adding perimeters and subtractng double shared edges
  static double get_chart_perimeter(Chart& c1, Chart& c2){

    // std::cout << "perimeter of charts [" << c1.id << ", " << c2.id << "]" << std::endl;

    double perimeter = c1.perimeter + c2.perimeter;

    // std::cout << "combined P before processing: " << c1.perimeter << " + " << c2.perimeter << " = " << perimeter << std::endl;

    //for each face in c1
    for (auto& c1_face : c1.facets){

      // std::cout << "for face " << c1_face.id() << std::endl;

      //for each edge
      Halfedge_facet_circulator he = c1_face.facet_begin();
      CGAL_assertion( CGAL::circulator_size(he) >= 3);

      do {
        // check if opposite appears in c2

        //check this edge is not a border
        if ( !(he->is_border()) && !(he->opposite()->is_border()) ){
          uint32_t adj_face_id = he->opposite()->facet()->id();

          // std::cout << "found adjacent face: " << adj_face_id << std::endl;

          for (auto& c2_face : c2.facets){
            if (c2_face.id() == adj_face_id)
            {
              perimeter -= (2 * edge_length(he));

              // std::cout << "- " << "found in chart " << c2.id << ", subtracted " << (2 * edge_length(he)) << std::endl;

              break;
            }
          }
        }
        else {
          // std::cout << "border edge\n";
        }

      } while ( ++he != c1_face.facet_begin());
    }

    // std::cout << "after = " << perimeter << std::endl;

    return perimeter;

  }

  static double get_direction_error(Chart& c1, Chart& c2, Vector &plane_normal){

    // std::cout << "----\nplane normal: " << plane_normal.x() << ", " << plane_normal.y() << ", " << plane_normal.z() << std::endl;

    // std::cout << "eq1:\n" << c1.R_quad.print() << "\neq2:\n" << c2.R_quad.print() << std::endl;
    // std::cout << "area 1: " << c1.area << " area 2: " << c2.area << std::endl;

    ErrorQuadric combinedQuad = (c1.R_quad*c1.area) + (c2.R_quad*c2.area);

    // std::cout << "combined:\n" << combinedQuad.print() << std::endl;

    double e_direction = evaluate_direction_quadric(combinedQuad,plane_normal);

    // std::cout << "E direction = " << e_direction << std::endl;

    return e_direction / (c1.area + c2.area);
  }

  static double evaluate_direction_quadric(ErrorQuadric &eq, Vector &plane_normal){


    // std::cout << "Term 1 = " << (plane_normal * (eq.A * plane_normal)) << std::endl;
    // std::cout << "Term 2 = " <<(2 * (eq.b  * plane_normal))<< std::endl;
    // std::cout << "Term 3 = " << eq.c << std::endl;

    return (plane_normal * (eq.A * plane_normal))
                        + (2 * (eq.b  * plane_normal))
                        + eq.c;
  }

  // static double accum_direction_error_in_chart(Chart& chart, const Vector plane_normal){
  //   double accum_error = 0;
  //   for (uint32_t i = 0; i < chart.facets.size(); i++){
  //     double error = 1.0 - (plane_normal * chart.normals[i]);
  //     accum_error += (error * chart.areas[i]);
  //   }
  //   return accum_error;
  // }

  static double edge_length(Halfedge_facet_circulator he){
    const Point& p = he->opposite()->vertex()->point();
    const Point& q = he->vertex()->point();

    return CGAL::sqrt(CGAL::squared_distance(p, q));
  }

};



struct JoinOperation {

  uint32_t chart1_id;
  uint32_t chart2_id;
  double cost;

  JoinOperation(uint32_t _c1, uint32_t _c2) : chart1_id(_c1), chart2_id(_c2){
    cost = 0;
  }
  JoinOperation(uint32_t _c1, uint32_t _c2, double _cost) : chart1_id(_c1), chart2_id(_c2), cost(_cost){}

  //calculates cost of joining these 2 charts
  static double cost_of_join(Chart &c1, Chart &c2 ,CLUSTER_SETTINGS& cluster_settings){

    // std::cout << "-----------------\n";


    double error = 0;

    Vector fit_plane_normal;

    double e_fit =       cluster_settings.e_fit_cf *   Chart::get_fit_error(c1,c2, fit_plane_normal);

    fit_plane_normal = normalise(fit_plane_normal);

    double e_direction = cluster_settings.e_ori_cf *   Chart::get_direction_error(c1,c2,fit_plane_normal); 
    double e_shape =     cluster_settings.e_shape_cf * Chart::get_compactness_of_merged_charts(c1,c2);

    error = e_fit + e_direction + e_shape;

    // std::cout << "Error [" << c1.id << ", " << c2.id << "]: " << error << ", e_fit: " << e_fit << ", e_ori: " << e_direction << ", e_shape: " << e_shape << std::endl;

    return error;
  }

  static bool sort_joins (JoinOperation j1, JoinOperation j2) {
    return (j1.cost < j2.cost);
  }

};


void count_faces_in_active_charts(std::vector<Chart> &charts) {
  uint32_t active_faces = 0;
  for (auto& chart : charts)
  {
    if (chart.active) 
    {
      active_faces += chart.facets.size();
    }
  }
  std::cout << "found " << active_faces << " active faces\n";
}

uint32_t 
create_charts (Polyhedron &P, const double cost_threshold , const uint32_t chart_threshold, CLUSTER_SETTINGS cluster_settings){
  std::stringstream report;

  //calculate areas of each face
  std::cout << "Calculating face areas...\n";
  std::map<face_descriptor,double> fareas;
  for(face_descriptor fd: faces(P)){
    fareas[fd] = CGAL::Polygon_mesh_processing::face_area  (fd,P);
  }
  //calculate normals of each faces
  std::cout << "Calculating face normals...\n";
  std::map<face_descriptor,Vector> fnormals;
  CGAL::Polygon_mesh_processing::compute_face_normals(P,boost::make_assoc_property_map(fnormals));

  //get boost face iterator
  face_iterator fb_boost, fe_boost;
  boost::tie(fb_boost, fe_boost) = faces(P);

  //each face begins as its own chart
  std::cout << "Creating initial charts...\n";
  std::vector<Chart> charts;
  for ( Facet_iterator fb = P.facets_begin(); fb != P.facets_end(); ++fb){
    //assign id to face
    fb->id() = charts.size();  

    // std::cout << "normal " << charts.size() << ": " << fnormals[*fb_boost] << std::endl;

    //init chart instance for face
    Chart c(charts.size(),*fb, fnormals[*fb_boost], fareas[*fb_boost]);
    charts.push_back(c);


    // //check uv coords saved
    // double u = fb->halfedge()->vertex()->point().get_u();
    // double v = fb->halfedge()->vertex()->point().get_v();
    // std::cout << "tex coords:  u " << u << ", v " << v << std::endl;

    fb_boost++;
  }

  //for reporting and calculating when to stop merging
  const uint32_t initial_charts = charts.size();
  const uint32_t desired_merges = initial_charts - chart_threshold;
  uint32_t chart_merges = 0;

  //create possible join list/queue. Each original edge in the mesh becomes a join (if not a boundary edge)
  std::cout << "Creating initial joins...\n";
  std::list<JoinOperation> joins;
  std::list<JoinOperation>::iterator it;

  int edgecount = 0;

  for( Edge_iterator eb = P.edges_begin(), ee = P.edges_end(); eb != ee; ++ eb){

    edgecount++;

    //only create join if halfedge is not a boundary edge
    if ( !(eb->is_border()) && !(eb->opposite()->is_border()) )
    {
          uint32_t face1 = eb->facet()->id();
          uint32_t face2 = eb->opposite()->facet()->id();
          JoinOperation join (face1,face2,JoinOperation::cost_of_join(charts[face1],charts[face2], cluster_settings));
          joins.push_back(join);
    }
  } 

  std::cout << joins.size() << " joins\n" << edgecount << " edges\n";

  // join charts until target is reached
  int prev_cost_percent = -1;
  int prev_charts_percent = -1;
  int overall_percent = -1;

  joins.sort(JoinOperation::sort_joins);
  const double lowest_cost = joins.front().cost;


  //execute lowest join cost and update affected joins.  re-sort.
  std::cout << "Processing join queue...\n";
  while (joins.front().cost < cost_threshold  
        &&  !joins.empty()
        &&  (charts.size() - chart_merges) > chart_threshold){

    //reporting-------------
    int percent = (int)(((joins.front().cost - lowest_cost) / (cost_threshold - lowest_cost)) * 100);
    if (percent != prev_cost_percent && percent > overall_percent) {
      prev_cost_percent = percent;
      overall_percent = percent;
      std::cout << percent << " percent complete\n";
    } 
    percent = (int)(((float)chart_merges / (float)desired_merges) * 100);
    if (percent != prev_charts_percent && percent > overall_percent) {
      prev_charts_percent = percent;
      overall_percent = percent;
      std::cout << percent << " percent complete\n";
    }

    //implement the join with lowest cost
    JoinOperation join_todo = joins.front();
    joins.pop_front();

    // std::cout << "join cost : " << join_todo.cost << std::endl; 

    //merge faces from chart2 into chart 1
    // std::cout << "merging charts " << join_todo.chart1_id << " and " << join_todo.chart2_id << std::endl;
    charts[join_todo.chart1_id].merge_with(charts[join_todo.chart2_id], join_todo.cost);


    //DEactivate chart 2
    if (charts[join_todo.chart2_id].active == false)
    {
      report << "chart " << join_todo.chart2_id << " was already inactive at merge " << chart_merges << std::endl; // should not happen
      continue;
    }
    charts[join_todo.chart2_id].active = false;
    
    int current_item = 0;
    std::list<int> to_erase;
    std::vector<JoinOperation> to_replace;

    //update itremaining joins that include either of the merged charts
    for (it = joins.begin(); it != joins.end(); ++it)
    {
      //if join is affected, update references and cost
      if (it->chart1_id == join_todo.chart1_id 
         || it->chart1_id == join_todo.chart2_id 
         || it->chart2_id == join_todo.chart1_id 
         || it->chart2_id == join_todo.chart2_id )
      {

        //eliminate references to joined chart 2 (it is no longer active)
        // by pointing them to chart 1
        if (it->chart1_id == join_todo.chart2_id){
          it->chart1_id = join_todo.chart1_id;
        }
        if (it->chart2_id == join_todo.chart2_id){
          it->chart2_id = join_todo.chart1_id; 
        }

        //search for duplicates
        if ((it->chart1_id == join_todo.chart1_id && it->chart2_id == join_todo.chart2_id) 
          || (it->chart2_id == join_todo.chart1_id && it->chart1_id == join_todo.chart2_id) ){
          report << "duplicate found : c1 = " << it->chart1_id << ", c2 = " << it->chart2_id << std::endl; 

          to_erase.push_back(current_item);
        }
        //check for joins within a chart
        else if (it->chart1_id == it->chart2_id)
        {
          report << "Join found within a chart: " << it->chart1_id << std::endl;
          to_erase.push_back(current_item);
          
        }
        else {
          //update cost with new cost
          it->cost = JoinOperation::cost_of_join(charts[it->chart1_id], charts[it->chart2_id], cluster_settings);

          //save this join to be deleted and replaced in correct position after deleting duplicates
          to_replace.push_back(*it);
          to_erase.push_back(current_item);
        }
      }
      current_item++;
    }

    //adjust ID to be deleted to account for previously deleted items
    to_erase.sort();
    int num_erased = 0;
    for (auto id : to_erase) {
      std::list<JoinOperation>::iterator it2 = joins.begin();
      std::advance(it2, id - num_erased);
      joins.erase(it2);
      num_erased++;
    }

    // replace joins that were filtered out to be sorted
    if (to_replace.size() > 0)
    {
      std::sort(to_replace.begin(), to_replace.end(), JoinOperation::sort_joins);
      std::list<JoinOperation>::iterator it2;
      uint32_t insert_item = 0;
      for (it2 = joins.begin(); it2 != joins.end(); ++it2){
        //insert items while join list item has bigger cost than element to be inserted
        while (it2->cost > to_replace[insert_item].cost
              && insert_item < to_replace.size()){
          joins.insert(it2, to_replace[insert_item]);
          insert_item++;
        }
        //if all items are in place, we are done
        if (insert_item >= to_replace.size())
        {
          break;
        }
      }
      //add any remaining items
      for (uint32_t i = insert_item; i < to_replace.size(); i++){
        joins.push_back(to_replace[i]);
      }
    }

#if 0
    //CHECK that each join would give a chart with at least 3 neighbours
    //TODO also need to check boundary edges
    to_erase.clear();
    std::vector<std::vector<uint32_t> > neighbour_count (charts.size(), std::vector<uint32_t>(0));
    std::list<JoinOperation>::iterator it2;
    for (it2 = joins.begin(); it2 != joins.end(); ++it2){
      //for chart 1 , add entry in vector for that chart containing id of chart 2
      // and vice versa
      neighbour_count[it2->chart1_id].push_back(it2->chart2_id);
      neighbour_count[it2->chart2_id].push_back(it2->chart1_id);
    }

    uint32_t join_id = 0;
    for (it2 = joins.begin(); it2 != joins.end(); ++it2){
      // combined neighbour count of joins' 2 charts should be at least 5
      // they will both contain each other (accounting for 2 neighbours) and require 3 more

      //merge the vectors for each chart in the join and count unique neighbours
      std::vector<uint32_t> combined_nbrs (neighbour_count[it2->chart1_id]);
      combined_nbrs.insert(combined_nbrs.end(), neighbour_count[it2->chart2_id].begin(), neighbour_count[it2->chart2_id].end());

      //find unique
      std::sort(combined_nbrs.begin(), combined_nbrs.end());
      uint32_t unique = 1;
      for (uint32_t i = 1; i < combined_nbrs.size(); i++){
        if (combined_nbrs[i] != combined_nbrs [i-1])
        {
          unique++;
        }
      }
      if (unique < 5)
      {
        to_erase.push_back(join_id);
      }
      join_id++;
    }
    //erase joins that would result in less than 3 corners
    to_erase.sort();
    num_erased = 0;
    for (auto id : to_erase) {
      std::list<JoinOperation>::iterator it2 = joins.begin();
      std::advance(it2, id - num_erased);
      joins.erase(it2);
      num_erased++;
    }
#endif

    chart_merges++;

    
  }

  // std::cout << "Printing Joins:\n";  
  // int index = 0;
  // std::list<JoinOperation>::iterator it2;
  // for (it2 = joins.begin(); it2 != joins.end(); ++it2){
  //   std::cout << "Join " << ++index << ", cost " << it2->cost << std::endl;
  // }


  //reporting//testing


  std::cout << "front join cost: " << joins.front().cost << ", num joins: " << joins.size() << "chart threshold: " << chart_threshold << std::endl; 

  std::cout << "--------------------\nCharts:\n----------------------\n";

  uint32_t total_faces = 0;
  uint32_t total_active_charts = 0;
  for (uint32_t i = 0; i < charts.size(); ++i)
  {
    if (charts[i].active)
    {
      uint32_t num_faces = charts[i].facets.size();
      total_faces += num_faces;
      total_active_charts++;
      // std::cout << "Chart " << i << " : " << num_faces << " faces" << std::endl;
    }
  }
  std::cout << "Total number of faces in charts = " << total_faces << std::endl;
  std::cout << "Initial charts = " << charts.size() << std::endl;
  std::cout << "Total number merges = " << chart_merges << std::endl;
  std::cout << "Total active charts = " << total_active_charts << std::endl;


  std::cout << "--------------------\nReport:\n----------------------\n";
  // std::cout << report.str();

  //populate LUT for face to chart mapping
  //count charts on the way to apply new chart ids
  uint32_t active_charts = 0;
  for (uint32_t id = 0; id < charts.size(); ++id) {
    auto& chart = charts[id];
    if (chart.active) {
      for (auto& f : chart.facets) {
        chart_id_map[f.id()] = active_charts;
      }
      active_charts++;
    }
  }

  return active_charts;

}



int main( int argc, char** argv ) 
{

  


  std::string obj_filename = "dino.obj";
  if (Utils::cmdOptionExists(argv, argv+argc, "-f")) {
    obj_filename = std::string(Utils::getCmdOption(argv, argv + argc, "-f"));
  }
  else {
    std::cout << "Please provide an obj filename using -f <filename.obj>" << std::endl;
    std::cout << "Optional: -ch specifies chart threshold (=100)" << std::endl;
    std::cout << "Optional: -co specifies cost threshold (=double max)" << std::endl;

    std::cout << "Optional: -ef specifies error fit coefficient (=1)" << std::endl;
    std::cout << "Optional: -eo specifies error orientation coefficient (=1)" << std::endl;
    std::cout << "Optional: -es specifies error shape coefficient (=1)" << std::endl;
    return 1;
  }

  double cost_threshold = std::numeric_limits<double>::max();
  if (Utils::cmdOptionExists(argv, argv+argc, "-co")) {
    cost_threshold = atof(Utils::getCmdOption(argv, argv + argc, "-co"));
  }
  uint32_t chart_threshold = 100;
  if (Utils::cmdOptionExists(argv, argv+argc, "-ch")) {
    chart_threshold = atoi(Utils::getCmdOption(argv, argv + argc, "-ch"));
  }
  double e_fit_cf = 1.0;
  if (Utils::cmdOptionExists(argv, argv+argc, "-ef")) {
    e_fit_cf = atof(Utils::getCmdOption(argv, argv + argc, "-ef"));
  }
  double e_ori_cf = 1.0;
  if (Utils::cmdOptionExists(argv, argv+argc, "-eo")) {
    e_ori_cf = atof(Utils::getCmdOption(argv, argv + argc, "-eo"));
  }
  double e_shape_cf = 1.0;
  if (Utils::cmdOptionExists(argv, argv+argc, "-es")) {
    e_shape_cf = atof(Utils::getCmdOption(argv, argv + argc, "-es"));
  }
  CLUSTER_SETTINGS cluster_settings (e_fit_cf, e_ori_cf, e_shape_cf);

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
  std::cout << "Mesh loaded (" << vertices.size() / 3 << " vertices, " << tris.size() / 3 << " faces, " << t_coords.size() / 2 << " tex coords)" << std::endl;

  auto start_time = std::chrono::system_clock::now();

  // build a polyhedron from the loaded arrays
  Polyhedron polyMesh;
  bool check_vertices = true;
  polyhedron_builder<HalfedgeDS> builder( vertices, tris, t_coords, tindices, check_vertices );


  polyMesh.delegate( builder );

  if (polyMesh.is_valid(false)){
    std::cout << "mesh valid\n"; 
  }


  if (!CGAL::is_triangle_mesh(polyMesh)){
    std::cerr << "Input geometry is not triangulated." << std::endl;
    return EXIT_FAILURE;
  }
  else {
    std::cout << "mesh is triangulated\n";
  }

  uint32_t active_charts = create_charts(polyMesh, cost_threshold, chart_threshold, cluster_settings);


  std::string out_filename = "data/charts.obj";
  std::ofstream ofs( out_filename );
  OBJ_printer::print_polyhedron_wavefront_with_charts( ofs, polyMesh,chart_id_map, active_charts, SEPARATE_CHART_FILE);
  ofs.close();
  std::cout << "simplified mesh was written to " << out_filename << std::endl;

  //Logging
  auto time = std::chrono::system_clock::now();
  std::chrono::duration<double> diff = time - start_time;
  std::time_t now_c = std::chrono::system_clock::to_time_t(time);
  std::string log_path = "../../data/logs/chart_creation_log.txt";
  ofs.open (log_path, std::ofstream::out | std::ofstream::app);
  ofs << "\n-------------------------------------\n";
  
  ofs << "Executed at " << std::put_time(std::localtime(&now_c), "%F %T") << std::endl;
  ofs << "Ran for " << (int)diff.count() / 60 << " m "<< (int)diff.count() % 60 << " s" << std::endl;
  ofs << "Input file: " << obj_filename << "\nOutput file: " << out_filename << std::endl;
  ofs << "Vertices: " << vertices.size()/3 << " , faces: " << tris.size()/3 << std::endl; 
  ofs << "Desired Charts: " << chart_threshold << ", active charts: " << active_charts << std::endl;
  ofs << "Cost threshold: " << cost_threshold << std::endl;
  ofs << "Cluster settings: e_fit: " << cluster_settings.e_fit_cf << ", e_ori: " << cluster_settings.e_ori_cf << ", e_shape" << cluster_settings.e_shape_cf << std::endl;

    ofs.close();
  std::cout << "Log written to " << log_path << std::endl;



  return EXIT_SUCCESS ; 
}