#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <chrono>
#include <limits>
#include <float.h>

#include <CGAL/Polygon_mesh_processing/measure.h>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/trim.hpp>

#include "Utils.h"
#include "OBJ_printer.h"
#include "SymMat.h"
#include "cluster_settings.h"
#include "ErrorQuadric.h"
#include "Chart.h"
#include "JoinOperation.h"
#include "polyhedron_builder.h"
#include "eig.h"

#include "CGAL_typedefs.h"



#define SEPARATE_CHART_FILE false


Vector normalise(Vector v) {return v / std::sqrt(v.squared_length());}


//key: face_id, value: chart_id
std::map<uint32_t, uint32_t> chart_id_map;



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