#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <chrono>
#include <limits>
#include <float.h>
#include <lamure/mesh/bvh.h>


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
#include "ClusterCreator.h"
#include "GridClusterCreator.h"
#include "ParallelClusterCreator.h"

#include "CGAL_typedefs.h"




int main( int argc, char** argv ) 
{

  std::string obj_filename = "dino.obj";
  if (Utils::cmdOptionExists(argv, argv+argc, "-f")) {
    obj_filename = std::string(Utils::getCmdOption(argv, argv + argc, "-f"));
  }
  else {
    std::cout << "Please provide an obj filename using -f <filename.obj>" << std::endl;

    std::cout << "Optional: -ch specifies chart threshold (=0)" << std::endl;
    std::cout << "Optional: -co specifies cost threshold (=double max)" << std::endl;

    std::cout << "Optional: -ef specifies error fit coefficient (=1)" << std::endl;
    std::cout << "Optional: -eo specifies error orientation coefficient (=1)" << std::endl;
    std::cout << "Optional: -es specifies error shape coefficient (=1)" << std::endl;

    std::cout << "Optional: -cc specifies cell resolution for grid splitting, along longest axis. When 0, no cell splitting is used. (=0)" << std::endl;
    std::cout << "Optional: -ct specifies threshold for grid chart splitting by normal variance (=0.001)" << std::endl;

    std::cout << "Optional: -debug writes charts to obj file, as colours that override texture coordinates (=false)" << std::endl;
    return 1;
  }

  double cost_threshold = std::numeric_limits<double>::max();
  if (Utils::cmdOptionExists(argv, argv+argc, "-co")) {
    cost_threshold = atof(Utils::getCmdOption(argv, argv + argc, "-co"));
  }
  uint32_t chart_threshold = 0;
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
  double cst = 0.001;
  if (Utils::cmdOptionExists(argv, argv+argc, "-ct")) {
    cst = atof(Utils::getCmdOption(argv, argv + argc, "-ct"));
  }

  CLUSTER_SETTINGS cluster_settings (e_fit_cf, e_ori_cf, e_shape_cf, cst);

  int cell_resolution = 0;
  if (Utils::cmdOptionExists(argv, argv+argc, "-cc")) {
    cell_resolution = atoi(Utils::getCmdOption(argv, argv + argc, "-cc"));
  }

  if (Utils::cmdOptionExists(argv, argv+argc, "-debug")) {
    cluster_settings.write_charts_as_textures = true;
  } 

  std::vector<std::string> materials;

    //load OBJ into arrays
  std::vector<double> vertices;
  std::vector<int> tris;
  std::vector<double> t_coords;
  std::vector<int> tindices;
  BoundingBoxLimits limits = Utils::load_obj( obj_filename, vertices, tris, t_coords, tindices, materials);


  if (vertices.size() == 0 ) {
    std::cout << "didnt find any vertices" << std::endl;
    return 1;
  }
  // std::cout << "Mesh loaded (" << triangles.size() << " triangles, " << vertices.size() / 3 << " vertices, " << tris.size() / 3 << " faces, " << t_coords.size() / 2 << " tex coords)" << std::endl;
  std::cout << "Mesh loaded (" << vertices.size() / 3 << " vertices, " << tris.size() / 3 << " faces, " << t_coords.size() / 2 << " tex coords)" << std::endl;


  std::cout << "loading mtl..." << std::endl;
  std::string mtl_filename = obj_filename.substr(0, obj_filename.size()-4)+".mtl";
  std::map<std::string, std::string> material_map;
  bool load_mtl_success = Utils::load_mtl(mtl_filename, material_map);

  //create face to texture id (implicit id = face, value = texture id)
  std::vector<int> face_textures;
  if (load_mtl_success)
  {
    for (uint32_t i = 0; i < tris.size() / 3; ++i)
    {
      // todo find the texture id for this face and add to list
    }

  }
  else {
    //default texture ids of 0 for every face
    face_textures.resize(tris.size() / 3);
  }

  
  //START chart creation ====================================================================================================================
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

  
  //key: face_id, value: chart_id
  std::map<uint32_t, uint32_t> chart_id_map;
  uint32_t active_charts;

  if (cell_resolution > 0)
  {
    //do grid clustering

    //creates clusters, starting using an arbitrary grid
    active_charts = GridClusterCreator::create_grid_clusters(polyMesh,chart_id_map, limits,cell_resolution, cluster_settings);

    std::cout << "Grid clusters: " << active_charts << std::endl;

  }

  active_charts = ParallelClusterCreator::create_charts(chart_id_map, polyMesh, cost_threshold, chart_threshold, cluster_settings);
  std::cout << "After creating chart clusters: " << active_charts << std::endl;



  //END chart creation ====================================================================================================================



  std::string out_filename = obj_filename.substr(0,obj_filename.size()-4) + "_charts.obj";
  std::string chart_filename = obj_filename.substr(0,obj_filename.size()-4) + "_charts.chart";
  std::ofstream ofs( out_filename );
  OBJ_printer::print_polyhedron_wavefront_with_charts( ofs, polyMesh,chart_id_map, active_charts, !cluster_settings.write_charts_as_textures, chart_filename);
  ofs.close();
  std::cout << "mesh was written to " << out_filename << std::endl;

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
  ofs << "Cluster settings: e_fit: " << cluster_settings.e_fit_cf << ", e_ori: " << cluster_settings.e_ori_cf << ", e_shape: " << cluster_settings.e_shape_cf << std::endl;

  ofs.close();
  std::cout << "Log written to " << log_path << std::endl;



  return EXIT_SUCCESS ; 
}
