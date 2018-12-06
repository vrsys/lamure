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
// #include "OBJ_printer_test.h"

typedef CGAL::Simple_cartesian<double> Kernel;

typedef Kernel::Vector_3 Vector;
typedef CGAL::Point_3<Kernel> Point;

typedef CGAL::Polyhedron_3<Kernel,CGAL::Polyhedron_items_with_id_3> Polyhedron;
typedef Polyhedron::HalfedgeDS HalfedgeDS;
// typedef CGAL::Dual<Polyhedron> Dual;

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
  


//key: face_id, value: chart_id
std::map<uint32_t, uint32_t> chart_id_map;

// struct to hold a vector of facets that make a chart
struct Chart
{
  std::vector<Facet> facets;
  bool active;
  Vector avg_normal;
  double area;
  double error;

  Chart(const Facet f,const Vector normal,const double _area){
    facets.push_back(f);
    active = true;
    area = _area;
    avg_normal = normal;
    error = 0;
  }

  //concatenate face lists
  void merge_with(Chart &mc, const double cost_of_join){

    facets.insert(facets.end(), mc.facets.begin(), mc.facets.end());

    Vector n = (avg_normal * area) + (mc.avg_normal * mc.area); //create new chart normal
    avg_normal = n / std::sqrt(n.squared_length()); //normalise normal
    area += mc.area;

    error += (mc.error + cost_of_join);

  }

  static double get_compactness_of_merged_charts(Chart& c1, Chart& c2){
    double area = c1.area + c2.area;
    std::vector<Facet> combined_facets (c1.facets);
    combined_facets.insert(combined_facets.end(), c2.facets.begin(), c2.facets.end());
    double perimeter = get_perimeter(combined_facets);

    return perimeter / area;
  }

  //calculate perimeter of chart
  static double get_perimeter(std::vector<Facet> chart_facets){
    double accum_perimeter = 0;

    //for each face
    for (auto& face : chart_facets)
    {

        Halfedge_facet_circulator he = face.facet_begin();
        CGAL_assertion( CGAL::circulator_size(he) >= 3);
        //for 3 adjacent faces
        do {
            uint32_t adj_face_id = he->opposite()->facet()->id();

            //check if they are in this chart
            bool found_in_this_chart = false;
            for (auto& chart_member : chart_facets){
              if (chart_member.id() == adj_face_id)
              {
                found_in_this_chart = true;
              }
            }
            //if not, add edge length to perimeter total, record as neighbour
            if (!found_in_this_chart)
            {
              accum_perimeter += edge_length(he);
            }

        } while ( ++he != face.facet_begin());
    }

    return accum_perimeter;
  }

  static double edge_length(Halfedge_facet_circulator he){
    const Point& p = he->opposite()->vertex()->point();
    const Point& q = he->vertex()->point();
    return CGAL::sqrt(CGAL::squared_distance(p, q));
  }

};

double cost_of_join(Chart &c1, Chart &c2){

  const double accum_error_factor = 1.0;
  const double angle_error_factor = 10.0;
  const double compactness_factor = 10000.0;

  //define error as angle between normal directions of charts
  double dot_product = c1.avg_normal * c2.avg_normal;
  double error = angle_error_factor * acos(dot_product);

  double compactness = Chart::get_compactness_of_merged_charts(c1,c2);
  
  // std::cout << "error output : angle: " << error << " compactness: " << compactness << " pre-existing: " << (c1.error + c2.error);

  error += (accum_error_factor * (c1.error + c2.error));
  error += (compactness_factor * compactness);

  // std::cout << " total: " << error << std::endl; 
  return error;
}

struct JoinOperation {

  uint32_t chart1_id;
  uint32_t chart2_id;
  double cost;

  JoinOperation(uint32_t _c1, uint32_t _c2) : chart1_id(_c1), chart2_id(_c2){
    cost = 0;
  }
  JoinOperation(uint32_t _c1, uint32_t _c2, double _cost) : chart1_id(_c1), chart2_id(_c2), cost(_cost){}

};

bool sort_joins (JoinOperation j1, JoinOperation j2) {
  return (j1.cost < j2.cost);
}

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
  std::map<face_descriptor,double> fareas;
  for(face_descriptor fd: faces(P)){
    fareas[fd] = CGAL::Polygon_mesh_processing::face_area  (fd,P);
  }
  //calculate normals of all faces
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
  std::vector<Chart> charts;
  for ( Facet_iterator fb = P.facets_begin(); fb != P.facets_end(); ++fb){
    fb->id() = charts.size();  

    Vector normal = fnormals[*fb_boost];
    double area = fareas[*fb_boost];

    Chart c(*fb, normal, area);
    charts.push_back(c);

    fb_boost++;
  }

  // const uint32_t initial_charts = charts.size();
  // const uint32_t chart_target = 20;
  // const uint32_t desired_merges = initial_charts - chart_target;
  uint32_t chart_merges = 0;

  //create possible join list
  std::list<JoinOperation> joins;
  std::list<JoinOperation>::iterator it;
  for( Edge_iterator eb = P.edges_begin(), ee = P.edges_end(); eb != ee; ++ eb){

    //only create join if halfedge is not a boundary edge
    if ( !(eb->is_border()) && !(eb->opposite()->is_border()) )
    {
          uint32_t face1 = eb->facet()->id();
          uint32_t face2 = eb->opposite()->facet()->id();

          JoinOperation join (face1,face2,cost_of_join(charts[face1],charts[face2]));
          joins.push_back(join);


          // std::cout << "join cost : " << cost_of_join(charts[face1],charts[face2]) << std::endl; 
          // std::cout << "create join between faces " << face1 << " and " << face2  << std::endl;
    }
  } 

  // join charts until target is reached
  int prev_percent = -1;

  joins.sort(sort_joins);
  const double lowest_cost = joins.front().cost;

  while (joins.front().cost < cost_threshold  
        &&  !joins.empty()
        &&  (charts.size() - chart_merges) > chart_threshold){

    int percent = (int)(((joins.front().cost - lowest_cost) / (cost_threshold - lowest_cost)) * 100);
    // int percent = (int)(((float)chart_merges / (float)desired_merges) * 100);
    if (percent != prev_percent) {
      prev_percent = percent;
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
          it->cost = cost_of_join(charts[it->chart1_id], charts[it->chart2_id]);

          //save this join to be deleted and replaced in correct position after deleting duplicates
          to_replace.push_back(*it);
          to_erase.push_back(current_item);
        }
      }
      current_item++;
    }

    uint32_t joins_after_first_pass = joins.size();

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
      std::sort(to_replace.begin(), to_replace.end(), sort_joins);
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


    //     //TODO - search for joins that would result in charts with less than 3 corners
    //     // therefore search for joins, where member charts are included in at least 3 joins, not including the shared join
    // //create map of chart ids to a list/map/vector of adjacent charts
    // //populate this on 1 pass through joins
    // //then pass through again and check all merges would leave at least 3 neighbours
    // to_erase.clear();
    // std::map<uint32_t, std::map<uint32_t, bool> > neighbour_count;
    // std::list<JoinOperation>::iterator it2;
    // for (it2 = joins.begin(); it2 != joins.end(); ++it2){
    //   //for chart 1 , add entry in map for that chart containing id of chart 2
    //   // and vice versa
    //   auto& c_map = neighbour_count[it2->chart1_id];
    //   c_map[it2->chart2_id] = true;
    //   c_map = neighbour_count[it2->chart2_id];
    //   c_map[it2->chart1_id] = true;
    // }
    // for (it2 = joins.begin(); it2 != joins.end(); ++it2){
    //   // combined neighbour count of joins' 2 charts should be at least 5
    //   // they will both contain each other (accounting for 2 neighbours) and require 3 more

    //   //if total neighbour count < 5
    //   //    add to to_erase list
    // }

    chart_merges++;

    // joins.sort(sort_joins);
    
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

// void organise_chart_id_map(std::map<uint32_t, uint32_t>& chart_id_map){

//   std::map<uint32_t, uint32_t> new_map;

//   for (auto const& x : chart_id_map)
//   {
//       std::cout << x.first  // string (key)
//                 // << ':' 
//                 // << x.second // string's value 
//                 << std::endl ;
//   }
// }

int main( int argc, char** argv ) 
{

  auto start_time = std::chrono::system_clock::now();


  std::string obj_filename = "dino.obj";
  if (Utils::cmdOptionExists(argv, argv+argc, "-f")) {
    obj_filename = std::string(Utils::getCmdOption(argv, argv + argc, "-f"));
  }
  else {
    std::cout << "Please provide an obj filename using -f <filename.obj>" << std::endl;
    return 1;
  }

  double cost_threshold = std::numeric_limits<double>::max();
  if (Utils::cmdOptionExists(argv, argv+argc, "-co")) {
    cost_threshold = atof(Utils::getCmdOption(argv, argv + argc, "-co"));
  }
  uint32_t chart_threshold = 10;
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
  ofs << "Ran for " << diff.count() << " s" << std::endl;
  ofs << "Input file: " << obj_filename << "\nOutput file: " << out_filename << std::endl;
  ofs << "Vertices: " << vertices.size()/3 << " , faces: " << tris.size()/3 << std::endl; 
  ofs << "Desired Charts: " << chart_threshold << ", active charts: " << active_charts << std::endl;
  ofs << "Cost threshold: " << cost_threshold << std::endl;
  ofs.close();
  std::cout << "Log written to " << log_path << std::endl;



  return EXIT_SUCCESS ; 
}