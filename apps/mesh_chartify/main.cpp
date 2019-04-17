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
#include <memory>


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

#include <lamure/mesh/bvh.h>

  
#include "kdtree.h"
#include <lamure/mesh/tools.h>

#include "lodepng.h"
#include "texture.h"
#include "frame_buffer.h"

#include "chart_packing.h"

int window_width_ = 1024;
int window_height_ = 1024;


int main( int argc, char** argv ) 
{


  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
  glutInitWindowSize(window_width_, window_height_);
  glutInitWindowPosition(64, 64);
  glutCreateWindow(argv[0]);
  glutSetWindowTitle("Lamure Mesh Preprocessing");
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_TEXTURE_2D);
  glDisable(GL_CULL_FACE);
  glewExperimental = GL_TRUE;
  glewInit();
  glutHideWindow();


  std::string obj_filename = "dino.obj";
  if (Utils::cmdOptionExists(argv, argv+argc, "-f")) {
    obj_filename = std::string(Utils::getCmdOption(argv, argv + argc, "-f"));
  }
  else {
    std::cout << "Please provide an obj filename using -f <filename.obj>" << std::endl;
    std::cout << "Optional: -of specifies outfile name" << std::endl;

    std::cout << "Optional: -ch specifies chart threshold (=infinite)" << std::endl;
    std::cout << "Optional: -co specifies cost threshold (=infinite)" << std::endl;

    std::cout << "Optional: -ef specifies error fit coefficient (=1)" << std::endl;
    std::cout << "Optional: -eo specifies error orientation coefficient (=1)" << std::endl;
    std::cout << "Optional: -es specifies error shape coefficient (=1)" << std::endl;

    std::cout << "Optional: -cc specifies cell resolution for grid splitting, along longest axis. When 0, no cell splitting is used. (=0)" << std::endl;
    std::cout << "Optional: -ct specifies threshold for grid chart splitting by normal variance (=0.001)" << std::endl;

    std::cout << "Optional: -debug writes charts to obj file, as colours that override texture coordinates (=false)" << std::endl;

    std::cout << "Optional: -tkd num triangles per kdtree node (default: 32000)" << std::endl;

    std::cout << "Optional: -tbvh num triangles per kdtree node (default: 8192)" << std::endl;

    std::cout << "Optional: -single-max: specifies largest possible single output texture (=8192)" << std::endl;

    std::cout << "Optional: -multi-max: specifies largest possible output texture (=total size of 4 x single_tex_limit (=32768))" << std::endl;

    return 1;
  }

  std::string out_filename = obj_filename.substr(0,obj_filename.size()-4) + "_charts.obj";
  if (Utils::cmdOptionExists(argv, argv+argc, "-of")) {
    out_filename = Utils::getCmdOption(argv, argv + argc, "-of");
  }

  double cost_threshold = std::numeric_limits<double>::max();
  if (Utils::cmdOptionExists(argv, argv+argc, "-co")) {
    cost_threshold = atof(Utils::getCmdOption(argv, argv + argc, "-co"));
  }
  uint32_t chart_threshold = std::numeric_limits<uint32_t>::max();;
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
  int num_tris_per_node_kdtree = 1024*32;
  if (Utils::cmdOptionExists(argv, argv+argc, "-tkd")) {
    num_tris_per_node_kdtree = atoi(Utils::getCmdOption(argv, argv + argc, "-tkd"));
  }
  int num_tris_per_node_bvh = 8*1024;
  if (Utils::cmdOptionExists(argv, argv+argc, "-tbvh")) {
    num_tris_per_node_bvh = atoi(Utils::getCmdOption(argv, argv + argc, "-tbvh"));
  }

  int single_tex_limit = 8192;
  if (Utils::cmdOptionExists(argv, argv+argc, "-single-max")) {
    single_tex_limit = atoi(Utils::getCmdOption(argv, argv+argc, "-single-max"));
    std::cout << "Single output texture limited to " << single_tex_limit << std::endl;
  }

  int multi_tex_limit = single_tex_limit * 4;
  if (Utils::cmdOptionExists(argv, argv+argc, "-multi-max")) {
    multi_tex_limit = atoi(Utils::getCmdOption(argv, argv+argc, "-multi-max"));
    std::cout << "Multi output texture limited to " << multi_tex_limit << std::endl;
  }


  
  std::vector<lamure::mesh::triangle_t> all_triangles;
  std::vector<std::string> all_materials;

  Utils::load_obj(obj_filename, all_triangles, all_materials);

  std::vector<indexed_triangle_t> all_indexed_triangles;
  
  if (all_triangles.empty()) {
    std::cout << "ERROR: Didnt find any triangles" << std::endl;
    exit(1);
  }

  std::cout << "Mesh loaded (" << all_triangles.size() << " triangles)" << std::endl;
  std::cout << "Loading mtl..." << std::endl;
  std::string mtl_filename = obj_filename.substr(0, obj_filename.size()-4)+".mtl";
  std::map<std::string, std::pair<std::string, int>> material_map;
  bool load_mtl_success = Utils::load_mtl(mtl_filename, material_map);

  if (all_materials.size() != all_triangles.size()) {
    std::cout << "ERROR: incorrect amount of materials found, given number of incoming triangles.";
    exit(1);
  }

  //check materials
  std::map<uint32_t, texture_info> texture_info_map;

  if (load_mtl_success) {

    for (auto mat_it : material_map) {
      std::string texture_filename = mat_it.second.first;
      uint32_t material_id = mat_it.second.second;
      std::cout << "Material " << mat_it.first << " : " << texture_filename << " : " << material_id << std::endl;

      if (boost::filesystem::exists(texture_filename) && boost::algorithm::ends_with(texture_filename, ".png")) {
        texture_info_map[material_id] = {texture_filename, Utils::get_png_dimensions(texture_filename)};
      }
      else if (texture_filename == "") {
        //ok
      }
      else {
        std::cout << "ERROR: texture " << texture_filename << " was not found or is not a .png" << std::endl;
        exit(1);
      }
    }

    for (uint32_t i = 0; i < all_triangles.size(); ++i) {
      auto& tri = all_triangles[i];
      std::string mat = all_materials[i];

      indexed_triangle_t idx_tri;
      idx_tri.v0_ = tri.v0_;
      idx_tri.v1_ = tri.v1_;
      idx_tri.v2_ = tri.v2_;
      idx_tri.tex_idx_ = material_map[mat].second;

      all_indexed_triangles.push_back(idx_tri);
    }
  }
  else {
    std::cout << "ERROR: Failed to load .mtl" << std::endl;
    exit(1);
  }

  all_triangles.clear();

  std::cout << "Building kd tree..." << std::endl;
  std::shared_ptr<kdtree_t> kdtree = std::make_shared<kdtree_t>(all_indexed_triangles, num_tris_per_node_kdtree);

  uint32_t first_leaf_id = kdtree->get_first_node_id_of_depth(kdtree->get_depth());
  uint32_t num_leaf_ids = kdtree->get_length_of_depth(kdtree->get_depth());

  std::vector<uint32_t> node_ids;
  for (uint32_t i = 0; i < num_leaf_ids; ++i) {
    node_ids.push_back(i + first_leaf_id);
  }

  
  std::map<uint32_t, std::map<uint32_t, uint32_t>> per_node_chart_id_map;
  std::map<uint32_t, Polyhedron> per_node_polyhedron;

  int prev_percent = -1;

  auto lambda_chartify = [&](uint64_t i, uint32_t id)->void{

    int percent = (int)(((float)i / (float)node_ids.size())*100.f);
    if (percent != prev_percent) {
      prev_percent = percent;
      std::cout << "Chartification: " << percent << " %" << std::endl;
    }

    //build polyhedron from node.begin to node.end in accordance with indices

    uint32_t node_id = node_ids[i];
    const auto& node = kdtree->get_nodes()[node_id];
    const auto& indices = kdtree->get_indices();

    BoundingBoxLimits limits;
    limits.min = scm::math::vec3f(std::numeric_limits<float>::max());
    limits.max = scm::math::vec3f(std::numeric_limits<float>::lowest());

    std::vector<indexed_triangle_t> node_triangles;
    for (uint32_t idx = node.begin_; idx < node.end_; ++idx) {

      const auto& tri = all_indexed_triangles[indices[idx]];
      node_triangles.push_back(tri);
    
      limits.min.x = std::min(limits.min.x, tri.v0_.pos_.x);
      limits.min.y = std::min(limits.min.y, tri.v0_.pos_.y);
      limits.min.z = std::min(limits.min.z, tri.v0_.pos_.z);

      limits.min.x = std::min(limits.min.x, tri.v1_.pos_.x);
      limits.min.y = std::min(limits.min.y, tri.v1_.pos_.y);
      limits.min.z = std::min(limits.min.z, tri.v1_.pos_.z);

      limits.min.x = std::min(limits.min.x, tri.v2_.pos_.x);
      limits.min.y = std::min(limits.min.y, tri.v2_.pos_.y);
      limits.min.z = std::min(limits.min.z, tri.v2_.pos_.z);

      limits.max.x = std::max(limits.max.x, tri.v0_.pos_.x);
      limits.max.y = std::max(limits.max.y, tri.v0_.pos_.y);
      limits.max.z = std::max(limits.max.z, tri.v0_.pos_.z);

      limits.max.x = std::max(limits.max.x, tri.v1_.pos_.x);
      limits.max.y = std::max(limits.max.y, tri.v1_.pos_.y);
      limits.max.z = std::max(limits.max.z, tri.v1_.pos_.z);

      limits.max.x = std::max(limits.max.x, tri.v2_.pos_.x);
      limits.max.y = std::max(limits.max.y, tri.v2_.pos_.y);
      limits.max.z = std::max(limits.max.z, tri.v2_.pos_.z);
    }


    Polyhedron polyMesh;
    polyhedron_builder<HalfedgeDS> builder(node_triangles);
    polyMesh.delegate(builder);

    if (!CGAL::is_triangle_mesh(polyMesh) || !polyMesh.is_valid(false)){
      std::cerr << "ERROR: Input geometry is not valid / not triangulated." << std::endl;
      return;
    }
    
    //key: face_id, value: chart_id
    std::map<uint32_t, uint32_t> chart_id_map;
    uint32_t active_charts;

    if (cell_resolution > 0) { //do grid clustering
      active_charts = GridClusterCreator::create_grid_clusters(polyMesh, chart_id_map, limits, cell_resolution, cluster_settings);
    }

    active_charts = ParallelClusterCreator::create_charts(chart_id_map, polyMesh, cost_threshold, chart_threshold, cluster_settings);
    //std::cout << "After creating chart clusters: " << active_charts << std::endl;

    per_node_chart_id_map[node_id] = chart_id_map;
    per_node_polyhedron[node_id] = polyMesh;

  };
  

  uint32_t num_threads = std::min((size_t)24, node_ids.size());
  lamure::mesh::parallel_for(num_threads, node_ids.size(), lambda_chartify);

  //convert back to triangle soup
  std::vector<lamure::mesh::Triangle_Chartid> triangles;

  uint32_t num_areas = 0;
  for (auto& per_node_chart_id_map_it : per_node_chart_id_map) {
    uint32_t node_id = per_node_chart_id_map_it.first;
    auto polyMesh = per_node_polyhedron[node_id];

    //create index
    typedef CGAL::Inverse_index<Polyhedron::Vertex_const_iterator> Index;
    Index index(polyMesh.vertices_begin(), polyMesh.vertices_end());
 
    //compute normals
    std::map<face_descriptor,Vector> fnormals;
    std::map<vertex_descriptor,Vector> vnormals;
    CGAL::Polygon_mesh_processing::compute_normals(polyMesh,
      boost::make_assoc_property_map(vnormals),
      boost::make_assoc_property_map(fnormals));

    uint32_t nml_id = 0;

    //extract triangle soup
    for(Polyhedron::Facet_const_iterator fi = polyMesh.facets_begin(); fi != polyMesh.facets_end(); ++fi) {
      Polyhedron::Halfedge_around_facet_const_circulator hc = fi->facet_begin();
      Polyhedron::Halfedge_around_facet_const_circulator hc_end = hc;

      if (circulator_size(hc) != 3) {
        std::cout << "ERROR: mesh corrupt!" << std::endl;
        exit(1);
      }

      lamure::mesh::Triangle_Chartid tri;

      Polyhedron::Vertex_const_iterator it = polyMesh.vertices_begin();
      std::advance(it, index[Polyhedron::Vertex_const_iterator(hc->vertex())]);
      tri.v0_.pos_ = scm::math::vec3f(it->point().x(), it->point().y(), it->point().z());
      tri.v0_.tex_ = scm::math::vec2f(fi->t_coords[0].x(), fi->t_coords[0].y());
      auto nml_it = vertices(polyMesh).begin();
      std::advance(nml_it, nml_id);
      tri.v0_.nml_ = scm::math::vec3f(vnormals[*nml_it].x(), vnormals[*nml_it].y(), vnormals[*nml_it].z());
      ++hc; ++nml_it;

      it = polyMesh.vertices_begin();
      std::advance(it, index[Polyhedron::Vertex_const_iterator(hc->vertex())]);
      tri.v1_.pos_ = scm::math::vec3f(it->point().x(), it->point().y(), it->point().z());
      tri.v1_.tex_ = scm::math::vec2f(fi->t_coords[1].x(), fi->t_coords[1].y());
      nml_it = vertices(polyMesh).begin();
      std::advance(nml_it, nml_id);
      tri.v1_.nml_ = scm::math::vec3f(vnormals[*nml_it].x(), vnormals[*nml_it].y(), vnormals[*nml_it].z());
      ++hc; ++nml_it;

      it = polyMesh.vertices_begin();
      std::advance(it, index[Polyhedron::Vertex_const_iterator(hc->vertex())]);
      tri.v2_.pos_ = scm::math::vec3f(it->point().x(), it->point().y(), it->point().z());
      tri.v2_.tex_ = scm::math::vec2f(fi->t_coords[2].x(), fi->t_coords[2].y());
      nml_it = vertices(polyMesh).begin();
      std::advance(nml_it, nml_id);
      tri.v2_.nml_ = scm::math::vec3f(vnormals[*nml_it].x(), vnormals[*nml_it].y(), vnormals[*nml_it].z());
      ++hc; ++nml_it;


      tri.area_id = num_areas;
      tri.chart_id = per_node_chart_id_map_it.second[fi->id()];
      tri.tex_id = fi->tex_id;

      triangles.push_back(tri);

 
    }
    ++num_areas;
       
  }

  
  std::cout << "Creating LOD hierarchy..." << std::endl;

  auto bvh = std::make_shared<lamure::mesh::bvh>(triangles, num_tris_per_node_bvh);
/*
  std::string bvh_filename = obj_filename.substr(0, obj_filename.size()-4)+".bvh";
  bvh->write_bvh_file(bvh_filename);
  std::cout << "Bvh file written to " << bvh_filename << std::endl;

  std::string lod_filename = obj_filename.substr(0, obj_filename.size()-4)+".lod";
  bvh->write_lod_file(lod_filename);
  std::cout << "Lod file written to " << lod_filename << std::endl;
  
  std::string lod_chart_filename = obj_filename.substr(0, obj_filename.size()-4)+".lodchart";
  bvh->write_chart_lod_file(lod_chart_filename);
  std::cout << "Lod chart file written to " << lod_chart_filename << std::endl;

  std::string lod_tex_id_filename = obj_filename.substr(0, obj_filename.size()-4)+".lodtexid";
  bvh->write_lod_tex_id_file(lod_tex_id_filename);
  std::cout << "Lod tex id file written to " << lod_tex_id_filename << std::endl;

  */

  std::cout << "Preparing charts..." << std::endl;


  //prep charts for projection
  std::map<uint32_t, std::map<uint32_t, chart>> chart_map;

  for (uint32_t tri_id = 0; tri_id < triangles.size(); ++tri_id) {
    const auto& tri = triangles[tri_id];
    uint32_t area_id = tri.area_id;
    uint32_t chart_id = tri.chart_id;

    chart_map[area_id][chart_id].id_ = chart_id;
    chart_map[area_id][chart_id].original_triangle_ids_.insert(tri_id);

  }

  for (uint32_t area_id = 0; area_id < num_areas; ++area_id) {

    //init charts
    for (auto& it : chart_map[area_id]) {

      it.second.rect_ = rectangle{
        scm::math::vec2f(std::numeric_limits<float>::max()),
        scm::math::vec2f(std::numeric_limits<float>::lowest()),
        it.first, 
        false};

      it.second.box_ = lamure::bounding_box(
        scm::math::vec3d(std::numeric_limits<float>::max()),
        scm::math::vec3d(std::numeric_limits<float>::lowest()));
      
    }

  }


  std::cout << "Expanding charts throughout BVH..." << std::endl;

  //grow chart boxes by all triangles in all levels of bvh
  for (uint32_t node_id = 0; node_id < bvh->get_num_nodes(); node_id++) {
    
    const std::vector<lamure::mesh::Triangle_Chartid>& tris = bvh->get_triangles(node_id);

    for (const auto& tri : tris) {
      chart_map[tri.area_id][tri.chart_id].box_.expand(scm::math::vec3d(tri.v0_.pos_));
      chart_map[tri.area_id][tri.chart_id].box_.expand(scm::math::vec3d(tri.v1_.pos_));
      chart_map[tri.area_id][tri.chart_id].box_.expand(scm::math::vec3d(tri.v2_.pos_));
    }
  }
  
  std::cout << "Assigning additional triangles to charts..." << std::endl;

  std::vector<uint32_t> area_ids;
  for (uint32_t area_id = 0; area_id < num_areas; ++area_id) {
    area_ids.push_back(area_id);
  }

  auto lambda_append = [&](uint64_t i, uint32_t id)->void{

    //compare all triangles with chart bounding boxes
  
    uint32_t area_id = area_ids[i];

    for (auto& it : chart_map[area_id]) {

      int chart_id = it.first;
      auto& chart = it.second;

      chart.all_triangle_ids_.insert(chart.original_triangle_ids_.begin(), chart.original_triangle_ids_.end());

      //add any triangles that intersect chart
      for (uint32_t tri_id = 0; tri_id < triangles.size(); ++tri_id) {
        const auto& tri = triangles[tri_id];
        if (chart.box_.contains(scm::math::vec3d(tri.v0_.pos_))
          || chart.box_.contains(scm::math::vec3d(tri.v1_.pos_))
          || chart.box_.contains(scm::math::vec3d(tri.v2_.pos_))) {

          chart.all_triangle_ids_.insert(tri_id);
        }
      }

      //report chart parameters
      /*
      std::cout << "chart id " << chart_id << std::endl;
      std::cout << "box min " << chart.box_.min() << std::endl;
      std::cout << "box max " << chart.box_.max() << std::endl;
      std::cout << "original triangles " << chart.original_triangle_ids_.size() << std::endl;
      std::cout << "all triangles " << chart.all_triangle_ids_.size() << std::endl;
      */
    }

  };

  
  num_threads = std::min((size_t)24, area_ids.size());
  lamure::mesh::parallel_for(num_threads, area_ids.size(), lambda_append);




  std::vector<rectangle> area_rects;

  for (uint32_t area_id = 0; area_id < num_areas; ++area_id) {
    calculate_chart_tex_space_sizes(chart_map[area_id], triangles, texture_info_map);

    project_charts(chart_map[area_id], triangles);

    std::cout << "Projected " << chart_map[area_id].size() << " charts for area " << area_id << std::endl;

    std::cout << "Running rectangle packing for area " << area_id << std::endl;
    
    //scale the rectangles
    const float scale = 400.f;

    std::vector<rectangle> rects;
    for (auto& chart_it : chart_map[area_id]) {
      chart& chart = chart_it.second;
      if (chart.original_triangle_ids_.size() > 0) {
        rectangle rect = chart.rect_;
        rect.max_ *= scale;
        rects.push_back(rect);
      }
    }

    //rectangle packing
    rectangle area_rect = pack(rects);
    area_rect.id_ = area_id;
    area_rect.flipped_ = false;
    area_rects.push_back(area_rect);

    std::cout << "Packing of area " << area_id << " complete (" << area_rect.max_.x << ", " << area_rect.max_.y << ")" << std::endl;

    //apply rectangles
    for (const auto& rect : rects) {
      chart_map[area_id][rect.id_].rect_ = rect;
      chart_map[area_id][rect.id_].projection.tex_space_rect = rect;//save for rendering from texture later on
    }

  }

  std::cout << "Packing " << area_rects.size() << " areas..." << std::endl;
  rectangle image = pack(area_rects);
  for (auto& rect : area_rects) {
    std::cout << "Area " << rect.id_ << " min: (" << rect.min_.x << ", " << rect.min_.y << ")" << std::endl;
    std::cout << "Area " << rect.id_ << " max: (" << rect.max_.x << ", " << rect.max_.y << ")" << std::endl;

    //next, apply the global transformation from area packing onto all individual chart rects per area

  }



  //default (minimum) output texture size = single texture size
  window_width_ = single_tex_limit;
  window_height_ = single_tex_limit;
  std::cout << "Single texture size limit: " << single_tex_limit << std::endl;
  std::cout << "Multi texture size limit: " << multi_tex_limit << std::endl;
  

  //cleanup
  bvh.reset();


#if 0


  //write obj

  typedef typename Polyhedron::Vertex_const_iterator                  VCI;
  typedef typename Polyhedron::Facet_const_iterator                   FCI;
  typedef typename Polyhedron::Halfedge_around_facet_const_circulator HFCC;
  typedef CGAL::Inverse_index< VCI> Index;

  std::cout << "Writing obj... " << out_filename << std::endl;

  std::ofstream ofs(out_filename);
  File_writer_wavefront_xtnd writer(ofs);

  uint32_t num_vertices = 0;
  uint32_t num_halfedges = 0;
  uint32_t num_facets = 0;
  for (auto& it : per_node_polyhedron) {
    num_vertices += it.second.size_of_vertices();
    num_halfedges += it.second.size_of_halfedges();
    num_facets += it.second.size_of_facets();
  }

  writer.write_header(ofs, num_vertices, num_halfedges, num_facets);

  for (auto& it : per_node_chart_id_map) {
    uint32_t node_id = it.first;
    //auto chart_id_map = it.second;
    auto polyMesh = per_node_polyhedron[node_id];

    for (VCI vi = polyMesh.vertices_begin(); vi != polyMesh.vertices_end(); ++vi) {
      writer.write_vertex(::CGAL::to_double( vi->point().x()),
                          ::CGAL::to_double( vi->point().y()),
                          ::CGAL::to_double( vi->point().z()));
    }
  }

  writer.write_tex_coord_header(num_facets * 3);

  for (auto& it : per_node_chart_id_map) {
    uint32_t node_id = it.first;
    //auto chart_id_map = it.second;
    auto polyMesh = per_node_polyhedron[node_id];
       
    //for each face, write tex coords
    for (FCI fi = polyMesh.facets_begin(); fi != polyMesh.facets_end(); ++fi) {
      writer.write_tex_coord(fi->t_coords[0].x(), fi->t_coords[0].y());
      writer.write_tex_coord(fi->t_coords[1].x(), fi->t_coords[1].y());
      writer.write_tex_coord(fi->t_coords[2].x(), fi->t_coords[2].y());
    }
  }

  writer.write_normal_header(num_facets);

  for (auto& it : per_node_chart_id_map) {
    uint32_t node_id = it.first;
    //auto chart_id_map = it.second;
    auto polyMesh = per_node_polyhedron[node_id];

    std::map<face_descriptor,Vector> fnormals;
    std::map<vertex_descriptor,Vector> vnormals;
    CGAL::Polygon_mesh_processing::compute_normals(polyMesh,
      boost::make_assoc_property_map(vnormals),
      boost::make_assoc_property_map(fnormals));

    for (face_descriptor fd: faces(polyMesh)){
      writer.write_normal((double)fnormals[fd].x(),(double)fnormals[fd].y(),(double)fnormals[fd].z());
    }

  }

  writer.write_facet_header();

  int32_t face_counter = 0;
  int32_t face_id = 0;

  for (auto& it : per_node_chart_id_map) {
    uint32_t node_id = it.first;
    auto chart_id_map = it.second;
    auto polyMesh = per_node_polyhedron[node_id];

    
    Index index(polyMesh.vertices_begin(), polyMesh.vertices_end());
    uint32_t highest_idx = 0;

    for(FCI fi = polyMesh.facets_begin(); fi != polyMesh.facets_end(); ++fi) {

      HFCC hc = fi->facet_begin();
      HFCC hc_end = hc;
      std::size_t nn = circulator_size(hc);
      CGAL_assertion(nn >= 3);
      writer.write_facet_begin(n);

      const int id = fi->id();
      int chart_id = chart_id_map[id];
      int edge = 0;
      do {
          uint32_t idx = index[VCI(hc->vertex())];
          highest_idx = std::max(idx, highest_idx);
          writer.write_facet_vertex_index(face_counter+idx, (face_id*3)+edge, face_id); // for uv coords

          ++hc;
          edge++;
      } while (hc != hc_end);
      writer.write_facet_end();
      face_id++;
    }

    face_counter += highest_idx+1;

  }

  writer.write_footer();

  ofs.close();

  std::cout << "Obj file written to:  " << out_filename << std::endl;



  //write charty charts
  
  std::string chart_filename = out_filename.substr(0,out_filename.size()-4) + ".chart";
  std::ofstream ocfs(chart_filename);

  int32_t chart_id_counter = 0;

  for (auto& it : per_node_chart_id_map) {
    uint32_t node_id = it.first;
    auto chart_id_map = it.second;
    auto polyMesh = per_node_polyhedron[node_id];    

    int32_t highest_chart_id = 0;
  
    for (FCI fi = polyMesh.facets_begin(); fi != polyMesh.facets_end(); ++fi) {
      const int32_t id = fi->id();
      const int32_t chart_id = chart_id_map[id];
      ocfs << chart_id_counter+chart_id << " ";
      highest_chart_id = std::max(highest_chart_id, chart_id);
    }
    
    chart_id_counter += highest_chart_id;

  }

  ocfs.close();

  std::cout << "Chart file written to:  " << chart_filename << std::endl;



  std::string tex_file_name = out_filename.substr(0, out_filename.size()-4) + ".texid";
  std::ofstream otfs(tex_file_name);
  for (auto& it : per_node_chart_id_map) {
    uint32_t node_id = it.first;
    auto chart_id_map = it.second;
    auto polyMesh = per_node_polyhedron[node_id];    

    for (Facet_iterator fi = polyMesh.facets_begin(); fi != polyMesh.facets_end(); ++fi) {    
      otfs << fi->tex_id << " ";
    }

  }
  otfs.close();
  std::cout << "Texture id per face file written to:  " << tex_file_name << std::endl;

  std::cout << "TOTAL NUM CHARTS: " << chart_id_counter << std::endl;
#endif

  return 0 ; 
}
