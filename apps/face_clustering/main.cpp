#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

#include <CGAL/Simple_cartesian.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/Polyhedron_items_with_id_3.h>


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

#include <CGAL/boost/graph/Dual.h>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/trim.hpp>

#include "Utils.h"
#include "OBJ_printer.h"

typedef CGAL::Simple_cartesian<double> Kernel;

typedef CGAL::Polyhedron_3<Kernel,CGAL::Polyhedron_items_with_id_3> Polyhedron;
typedef Polyhedron::HalfedgeDS HalfedgeDS;
// typedef CGAL::Dual<Polyhedron> Dual;

typedef Polyhedron::Facet_iterator Facet_iterator;
typedef Polyhedron::Facet_handle Facet_handle;
typedef Polyhedron::Facet Facet; 


typedef Polyhedron::Halfedge Halfedge;
typedef Polyhedron::Edge_iterator Edge_iterator;


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

    typedef CGAL::Point_3<Kernel> Point;

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
std::map<uint32_t, int32_t> chart_id_map;

// struct to hold a vector of facets that make a chart
struct Chart
{
  std::vector<uint32_t> facets;
  bool active;

  Chart(uint32_t f){
    add_face(f);
    active = true;
  }
  void add_face(uint32_t f){
    facets.push_back(f);
  }
  //concatenate face lists
  void merge_with(Chart &mc){
    facets.insert(facets.end(), mc.facets.begin(), mc.facets.end());
  }
  uint32_t num_faces(){
    return facets.size();
  }

};

double cost_of_join(Chart &c1, Chart &c2){
  return c1.num_faces() + c2.num_faces();
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

void 
create_charts (Polyhedron &P){

  std::stringstream report;

  //each face begins as its own chart
  //ad face ids in same loop
  std::vector<Chart> charts;
  for ( Facet_iterator fb = P.facets_begin(); fb != P.facets_end(); ++fb){
    fb->id() = charts.size();  
    Chart c(fb->id());
    charts.push_back(c);

    std::cout << "creating chart " << charts.size()-1 << " with face id " << fb->id() << std::endl;
  }

  const uint32_t initial_charts = charts.size();
  const uint32_t chart_target = 20;
  const uint32_t desired_merges = initial_charts - chart_target;
  uint32_t chart_merges = 0;

  //create possible join list
  std::list<JoinOperation> joins;
  std::list<JoinOperation>::iterator it;
  for( Edge_iterator eb = P.edges_begin(), ee = P.edges_end(); eb != ee; ++ eb){

    //disallow join if halfedge is a boundary edge
    if ( !(eb->is_border()) && !(eb->opposite()->is_border()) )
    {
          uint32_t face1 = eb->facet()->id();
          uint32_t face2 = eb->opposite()->facet()->id();

          JoinOperation join (face1,face2,2.0);
          joins.push_back(join);

          std::cout << "create join between faces " << face1 << " and " << face2  << std::endl;
    }
  } 

  //make joins until target is reached
  // while ((initial_charts-chart_merges) > chart_target){
  int prev_percent = -1;

  while (chart_merges < desired_merges && !joins.empty()){

    int percent = (int)(((float)chart_merges / (float)desired_merges) * 100);
    if (percent != prev_percent) {
      prev_percent = percent;
      std::cout << percent << " percent merged\n";
    }

    //sort joins by cost
    // std::sort (joins.begin(), joins.end(), sort_joins);
    //TODO faster way than sorting the whole list each time - change the placing only of affected items
    joins.sort(sort_joins);

    //implement the join with lowest cost
    JoinOperation join_todo = joins.front();
    joins.pop_front();

    //merge faces from chart2 into chart 1
    charts[join_todo.chart1_id].merge_with(charts[join_todo.chart2_id]);

    if (charts[join_todo.chart2_id].active == false)
    {
      //report << "chart " << join_todo.chart2_id << " was already inactive at merge " << chart_merges << std::endl;
      continue;
    }

    charts[join_todo.chart2_id].active = false;
    
    int current_item = 0;
    std::vector<int> to_erase;

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

        //update cost with new cost
        it->cost = cost_of_join(charts[it->chart1_id], charts[it->chart2_id]);

        //check for joins within a chart
        if (it->chart1_id == it->chart2_id)
        {
          report << "Join found within a chart: " << it->chart1_id << std::endl;
          to_erase.push_back(current_item);
          
        }
      }
      current_item++;
    }

    for (auto id : to_erase) {
      std::list<JoinOperation>::iterator it = joins.begin();
      std::advance(it, id);
      joins.erase(it);
    }

    

    chart_merges++;
    //std::cout << chart_merges << " merges\n";
    
  }


  //reporting//testing

  std::cout << "--------------------\nCharts:\n----------------------\n";

  uint32_t total_faces = 0;
  uint32_t total_active_charts = 0;
  for (uint32_t i = 0; i < charts.size(); ++i)
  {
    if (charts[i].active)
    {
      uint32_t num_faces = charts[i].num_faces();
      total_faces += num_faces;
      total_active_charts++;
      std::cout << "Chart " << i << " : " << num_faces << " faces\n";
    }
  }
  std::cout << "Total number of faces in charts = " << total_faces << std::endl;
  std::cout << "Initial charts = " << initial_charts << std::endl;
  std::cout << "Total number merges = " << chart_merges << std::endl;
  std::cout << "Total active charts = " << total_active_charts << std::endl;


  std::cout << "--------------------\nReport:\n----------------------\n";
  std::cout << report.str();

  //populate LUT for face to chart mapping
  for (uint32_t id = 0; id < charts.size(); ++id) {
    auto& chart = charts[id];
    if (chart.active) {
      for (auto& f : chart.facets) {
        chart_id_map[f] = id;
      }
    }
  }

}

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


  //split the mesh into charts
  //chart configuration can be accessed
  create_charts(polyMesh);

  std::string out_filename = "data/charts.obj";
  std::ofstream ofs( out_filename );

  OBJ_printer::print_polyhedron_wavefront_with_tex( ofs, polyMesh,chart_id_map);

  ofs.close();
  std::cout << "simplified mesh was written to " << out_filename << std::endl;


  return EXIT_SUCCESS ; 
}