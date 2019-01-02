#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <chrono>
#include <limits>

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


typedef CGAL::Simple_cartesian<double> Kernel;

typedef Kernel::Vector_3 Vector;
typedef CGAL::Point_3<Kernel> Point;



typedef CGAL::Polyhedron_3<Kernel,CGAL::Polyhedron_items_with_id_3> Polyhedron;
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

  polyhedron_builder( std::vector<double> &_vertices,
                      std::vector<int> &_tris) 
                      : vertices(_vertices), tris(_tris) {}


  void operator()( HDS& hds) {


    // create a cgal incremental builder
    CGAL::Polyhedron_incremental_builder_3<HDS> B( hds, true);
    B.begin_surface( vertices.size()/3, tris.size()/3 );
   
    // add the polyhedron vertices
    for( int i=0; i<(int)vertices.size() / 3; ++i ){

      B.add_vertex( Point( vertices[(i*3)], 
                           vertices[(i*3)+1], 
                           vertices[(i*3)+2]));

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

struct ErrorQuadric {

  // std::vector<double> A;
  SymMat A;
  Vector b;
  double c;

  ErrorQuadric() {
    // A = std::vector<double> (5,0);
    b = Vector(0,0,0);
    c = 0;
  }

  ErrorQuadric(Point& p) {
    // A = Utils::calc_symmetric_mat3(p);
    A = SymMat(p);
    b = Vector(p.x(), p.y(), p.z());
    c = 1;
  }

  ErrorQuadric operator+(const ErrorQuadric& e){
    ErrorQuadric eq;
    // for (int i = 0; i < 5; ++i)
    // {
    //   eq.A.push_back(e.A[i] + A[i]);
    // }
    eq.A = A + e.A; 
    eq.b = b + e.b;
    eq.c = c + e.c;

    return eq;
  }
};
  


//key: face_id, value: chart_id
std::map<uint32_t, uint32_t> chart_id_map;

// struct to hold a vector of facets that make a chart
struct Chart
{
  std::vector<Facet> facets;
  std::vector<Vector> normals;
  std::vector<double> areas;
  bool active;
  Vector avg_normal;
  double area;
  // double error;
  double perimeter;

  ErrorQuadric quad;

  Chart(Facet f,const Vector normal,const double _area){
    facets.push_back(f);
    normals.push_back(normal);
    areas.push_back(_area);
    active = true;
    area = _area;
    avg_normal = normal;
    // error = 0;

    perimeter = get_face_perimeter(f);

    quad = createQuad(f);
  }

  //create a combined quadric for vertices
  ErrorQuadric createQuad(Facet &f){

    ErrorQuadric face_quad;
    Halfedge_facet_circulator he = f.facet_begin();
    CGAL_assertion( CGAL::circulator_size(he) >= 3);
    do {
      face_quad = face_quad + ErrorQuadric(he->vertex()->point());
    } while ( ++he != f.facet_begin());

    return face_quad;
  }

  //concatenate face lists
  void merge_with(Chart &mc, const double cost_of_join){

    facets.insert(facets.end(), mc.facets.begin(), mc.facets.end());
    normals.insert(normals.end(), mc.normals.begin(), mc.normals.end());
    areas.insert(areas.end(), mc.areas.begin(), mc.areas.end());

    Vector n = (avg_normal * area) + (mc.avg_normal * mc.area); //create new chart normal
    avg_normal = n / std::sqrt(n.squared_length()); //normalise normal
    area += mc.area;

    // error += (mc.error + cost_of_join);

    quad = quad + mc.quad;

  }

  static double get_fit_error(Chart& c1, Chart& c2){

    ErrorQuadric fit_quad = c1.quad + c2.quad;

    //create Z matrix

    //find eigenvalues / eigenvectors

    // sub into E fit equation http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.32.8100&rep=rep1&type=pdf

  }
  // as defined in Garland et al 2001
  static double get_compactness_of_merged_charts(Chart& c1, Chart& c2){


    double irreg1 = get_irregularity(c1.perimeter, c1.area);
    double irreg2 = get_irregularity(c2.perimeter, c2.area);
    double irreg_new = get_irregularity(get_chart_perimeter(c1,c2), c1.area + c2.area);

    double shape_penalty = ( irreg_new - std::max(irreg1, irreg2) ) / irreg_new;

    return shape_penalty;
  }

    // as defined in Garland et al 2001
  static double get_irregularity(double perimeter, double area){
    return (perimeter*perimeter) / (4 * M_PI * area);
  }

  //adds edge lengths of a face
  //check functionality for non-manifold edges
  static double get_face_perimeter(Facet& f){

    Halfedge_facet_circulator he = f.facet_begin();
    CGAL_assertion( CGAL::circulator_size(he) >= 3);
    double accum_perimeter = 0;

    //for 3 edges (adjacent faces)
    do {
          accum_perimeter += edge_length(he);
    } while ( ++he != f.facet_begin());

    return accum_perimeter;
  }

  //calculate perimeter of chart
  // associate a perimeter with each cluster
  // then calculate by adding perimeters and subtractng double shared edges
  static double get_chart_perimeter(Chart& c1, Chart& c2){

    double perimeter = c1.perimeter + c2.perimeter;

    //for each face in c1
    for (auto& c1_face : c1.facets){

      //for each edge
      Halfedge_facet_circulator he = c1_face.facet_begin();
      CGAL_assertion( CGAL::circulator_size(he) >= 3);

      do {
        // check if opposite appears in c2

        //check this edge is not a border
        if ( !(he->is_border()) && !(he->opposite()->is_border()) ){
          uint32_t adj_face_id = he->opposite()->facet()->id();

          for (auto& c2_face : c2.facets){
            if (c2_face.id() == adj_face_id)
            {
              perimeter -= (2 * edge_length(he));
              break;
            }
          }
        }

      } while ( ++he != c1_face.facet_begin());
    }

    return perimeter;

  }

  static double get_direction_error(Chart& c1, Chart& c2){

    //average error between average normal and individual face normals
    //can be stored as quadric - next step

    //TODO replace avg_normal with plane fit normal
    Vector avg_normal = (c1.avg_normal * c1.area) + (c2.avg_normal * c2.area);
    double e_direction = accum_direction_error_in_chart(c1,avg_normal) + accum_direction_error_in_chart(c2,avg_normal);

    return e_direction / (c1.area + c2.area);
  }
  static double accum_direction_error_in_chart(Chart& chart, const Vector plane_normal){
    double accum_error = 0;
    for (uint32_t i = 0; i < chart.facets.size(); i++){
      double error = 1.0 - (plane_normal * chart.normals[i]);
      accum_error += (error * chart.areas[i]);
    }
    return accum_error;
  }

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

  static double cost_of_join(Chart &c1, Chart &c2){

    // const double accum_error_factor = 1.0;
    // const double angle_error_factor = 10.0;
    // const double compactness_factor = 1.0;

    double error = 0;

    //define error as angle between normal directions of charts
    // double dot_product = c1.avg_normal * c2.avg_normal;
    // double error = angle_error_factor * acos(dot_product);

    double e_fit = Chart::get_fit_error(c1,c2);
    double e_direction = Chart::get_direction_error(c1,c2);
    double e_shape = Chart::get_compactness_of_merged_charts(c1,c2);

    error = e_fit + e_direction + e_shape;

    // error += (accum_error_factor * (c1.error + c2.error));
    // error += (compactness_factor * e_shape);
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
create_charts (Polyhedron &P, const double cost_threshold , const uint32_t chart_threshold){
  std::stringstream report;

  //calculate areas
  std::cout << "Calculating face areas...\n";
  std::map<face_descriptor,double> fareas;
  for(face_descriptor fd: faces(P)){
    fareas[fd] = CGAL::Polygon_mesh_processing::face_area  (fd,P);
  }
  //calculate normals of all faces
  std::cout << "Calculating face normals...\n";
  std::map<face_descriptor,Vector> fnormals;
  std::map<vertex_descriptor,Vector> vnormals;
  CGAL::Polygon_mesh_processing::compute_normals(P,
                                                 boost::make_assoc_property_map(vnormals),
                                                 boost::make_assoc_property_map(fnormals));

  //get boost face iterator
  face_iterator fb_boost, fe_boost;
  boost::tie(fb_boost, fe_boost) = faces(P);

  //each face begins as its own chart
  //add face ids in same loop
  std::cout << "Creating initial charts...\n";
  std::vector<Chart> charts;
  for ( Facet_iterator fb = P.facets_begin(); fb != P.facets_end(); ++fb){
    fb->id() = charts.size();  

    Vector normal = fnormals[*fb_boost];
    double area = fareas[*fb_boost];

    Chart c(*fb, normal, area);
    charts.push_back(c);

    fb_boost++;
  }

  const uint32_t initial_charts = charts.size();
  const uint32_t desired_merges = initial_charts - chart_threshold;
  uint32_t chart_merges = 0;

  //create possible join list
  std::cout << "Creating initial joins...\n";
  std::list<JoinOperation> joins;
  std::list<JoinOperation>::iterator it;
  for( Edge_iterator eb = P.edges_begin(), ee = P.edges_end(); eb != ee; ++ eb){

    //only create join if halfedge is not a boundary edge
    if ( !(eb->is_border()) && !(eb->opposite()->is_border()) )
    {
          uint32_t face1 = eb->facet()->id();
          uint32_t face2 = eb->opposite()->facet()->id();
          JoinOperation join (face1,face2,JoinOperation::cost_of_join(charts[face1],charts[face2]));
          joins.push_back(join);
    }
  } 

  std::cout << joins.size() << " joins\n";

  // join charts until target is reached
  int prev_cost_percent = -1;
  int prev_charts_percent = -1;
  int overall_percent = -1;

  joins.sort(JoinOperation::sort_joins);
  const double lowest_cost = joins.front().cost;

  std::cout << "Processing join queue...\n";
  while (joins.front().cost < cost_threshold  
        &&  !joins.empty()
        &&  (charts.size() - chart_merges) > chart_threshold){

    int percent = (int)(((joins.front().cost - lowest_cost) / (cost_threshold - lowest_cost)) * 100);
    if (percent != prev_cost_percent && percent > overall_percent) {
      prev_cost_percent = percent;
      overall_percent = percent;
      std::cout << percent << " percent complete\n";
    } 


  // //end test
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
    charts[join_todo.chart1_id].merge_with(charts[join_todo.chart2_id], join_todo.cost);

    if (charts[join_todo.chart2_id].active == false)
    {
      report << "chart " << join_todo.chart2_id << " was already inactive at merge " << chart_merges << std::endl;
      continue;
    }

    charts[join_todo.chart2_id].active = false;
    
    int current_item = 0;
    std::list<int> to_erase;
    std::vector<JoinOperation> to_replace;
    // std::list<int> to_sort;

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
          it->cost = JoinOperation::cost_of_join(charts[it->chart1_id], charts[it->chart2_id]);

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

    chart_merges++;

    
  }

  // std::cout << "Printing Joins:\n";  
  // int index = 0;
  // std::list<JoinOperation>::iterator it2;
  // for (it2 = joins.begin(); it2 != joins.end(); ++it2){
  //   std::cout << "Join " << ++index << ", cost " << it2->cost << std::endl;
  // }


  //reporting//testing

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

  auto start_time = std::chrono::system_clock::now();

  // build a polyhedron from the loaded arrays
  Polyhedron polyMesh;
  polyhedron_builder<HalfedgeDS> builder( vertices, tris );
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

  uint32_t active_charts = create_charts(polyMesh, cost_threshold, chart_threshold);


  std::string out_filename = "data/charts.obj";
  std::ofstream ofs( out_filename );
  OBJ_printer::print_polyhedron_wavefront_with_chart_colours( ofs, polyMesh,chart_id_map, active_charts);
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
  ofs.close();
  std::cout << "Log written to " << log_path << std::endl;



  return EXIT_SUCCESS ; 
}