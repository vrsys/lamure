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

int render_to_texture_width_ = 4096;
int render_to_texture_height_ = 4096;

int full_texture_width_ = 4096;
int full_texture_height_ = 4096;


struct viewport {
  scm::math::vec2f normed_dims;
  scm::math::vec2f normed_offset;
};

std::vector<viewport> viewports_;

std::vector< std::shared_ptr<texture_t>> textures_;

GLuint shader_program_;
GLuint vertex_buffer_;

GLuint dilation_shader_program_;
GLuint dilation_vertex_buffer_;

std::vector<std::shared_ptr<frame_buffer_t>> frame_buffers_;

std::shared_ptr<texture_t> load_image(const std::string& filepath) {
  std::vector<unsigned char> img;
  unsigned int width = 0;
  unsigned int height = 0;
  int tex_error = lodepng::decode(img, width, height, filepath);
  if (tex_error) {
    std::cout << "ERROR: unable to load image file (not a .png?) " << filepath << std::endl;
    exit(1);
  }
  else {
    std::cout << "Loaded texture from " << filepath << std::endl;}

  auto texture = std::make_shared<texture_t>(width, height, GL_LINEAR);
  texture->set_pixels(&img[0]);

  return texture;
}


void save_image(std::string& filename, std::vector<uint8_t>& image, int width, int height) {
  unsigned int tex_error = lodepng::encode(filename, image, width, height);

  if (tex_error) {
    std::cerr << "ERROR: unable to save image file " << filename << std::endl;
    std::cerr << tex_error << ": " << lodepng_error_text(tex_error) << std::endl;
  }
  std::cout << "Saved image to " << filename << std::endl;

}


void save_framebuffer_to_image(std::string filename, std::shared_ptr<frame_buffer_t> frame_buffer) {

  std::vector<uint8_t> pixels;
  frame_buffer->get_pixels(0, pixels);

  int tex_error = lodepng::encode(filename, pixels, frame_buffer->get_width(), frame_buffer->get_height());
  if (tex_error) {
    std::cout << "ERROR: unable to save image file " << filename << std::endl;
    exit(1);
  }
  std::cout << "Framebuffer written to image " << filename << std::endl;

}

//subroutine for error-checking during shader compilation
GLint compile_shader(const std::string& _src, GLint _shader_type) {

  const char* shader_src = _src.c_str();
  GLuint shader = glCreateShader(_shader_type);
  
  glShaderSource(shader, 1, &shader_src, NULL);
  glCompileShader(shader);

  GLint status;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if (status == GL_FALSE) {
    GLint log_length;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);

    GLchar* log = new GLchar[log_length + 1];
    glGetShaderInfoLog(shader, log_length, NULL, log);

    const char* type = NULL;
    switch (_shader_type) {
      case GL_VERTEX_SHADER: type = "vertex"; break;
      case GL_FRAGMENT_SHADER: type = "fragment"; break;
      default: break;
    }

    std::string error_message = "ERROR: Compile shader failure in " + std::string(type) + " shader:\n" + std::string(log);
    delete[] log;
    std::cout << error_message << std::endl;
    exit(1);
  }

  return shader;

}


//compile and link the shader programs
void make_shader_program() {

  //locates vertices at new uv position on screen
  //passes old uvs in order to read from the texture 
  
  std::string vertex_shader_src = "#version 420\n\
    layout (location = 0) in vec2 vertex_old_coord;\n\
    layout (location = 1) in vec2 vertex_new_coord;\n\
    \n\
    uniform vec2 viewport_offset;\n\
    uniform vec2 viewport_scale;\n\
    \n\
    varying vec2 passed_uv;\n\
    \n\
    void main() {\n\
      vec2 coord = vec2(vertex_new_coord.x, vertex_new_coord.y);\n\
      vec2 coord_translated = coord - viewport_offset; \n\
      vec2 coord_scaled = coord_translated / viewport_scale; \n\
      gl_Position = vec4((coord_scaled-0.5)*2.0, 0.5, 1.0);\n\
      passed_uv = vertex_old_coord;\n\
    }";

  std::string fragment_shader_src = "#version 420\n\
    uniform sampler2D image;\n\
    varying vec2 passed_uv;\n\
    \n\
    layout (location = 0) out vec4 fragment_color;\n\
    \n\
    void main() {\n\
      vec4 color = texture(image, passed_uv).rgba;\n\
      fragment_color = vec4(color.rgb, 1.0);\n\
    }";


  //compile shaders
  GLint vertex_shader = compile_shader(vertex_shader_src, GL_VERTEX_SHADER);
  GLint fragment_shader = compile_shader(fragment_shader_src, GL_FRAGMENT_SHADER);

  //create the GL resource and save the handle for the shader program
  shader_program_ = glCreateProgram();
  glAttachShader(shader_program_, vertex_shader);
  glAttachShader(shader_program_, fragment_shader);
  glLinkProgram(shader_program_);

  //since the program is already linked, we do not need to keep the separate shader stages
  glDetachShader(shader_program_, vertex_shader);
  glDeleteShader(vertex_shader);
  glDetachShader(shader_program_, fragment_shader);
  glDeleteShader(fragment_shader);
}


//compile and link the shader programs
void make_dilation_shader_program() {

  std::string vertex_shader_src = "#version 420\n\
    layout (location = 0) in vec2 vertex_old_coord;\n\
    layout (location = 1) in vec2 vertex_new_coord;\n\
    \n\
    varying vec2 passed_uv;\n\
    \n\
    void main() {\n\
      vec2 coord = vec2(vertex_old_coord.x, vertex_old_coord.y);\n\
      gl_Position = vec4((coord), 0.5, 1.0);\n\
      passed_uv = vertex_new_coord;\n\
    }";

  std::string fragment_shader_src = "#version 420\n\
    uniform sampler2D image;\n\
    uniform int image_width;\n\
    uniform int image_height;\n\
    varying vec2 passed_uv;\n\
    \n\
    layout (location = 0) out vec4 fragment_color;\n\
    \n\
    vec4 weighted_dilation() {\n\
      vec4 fallback_color = vec4(1.0, 0.0, 1.0, 1.0);\n\
      vec4 accumulated_color = vec4(0.0, 0.0, 0.0, 1.0);\n\
      float accumulated_weight = 0.0;\n\
      for(int y_offset = -1; y_offset < 2; ++y_offset) {\n\
        if( 0 == y_offset) {\n\
          continue;\n\
        }\n\
        ivec2 sampling_frag_coord = ivec2(gl_FragCoord.xy) + ivec2(0, y_offset);\n\
\n\
        if( \n\
           sampling_frag_coord.y >= image_height || sampling_frag_coord.y < 0 ) {\n\
            continue;\n\
           }\n\
\n\
        vec3 sampled_color = texelFetch(image, ivec2(sampling_frag_coord), 0).rgb;\n\
\n\
        if( !(1.0 == sampled_color.r && 0.0 == sampled_color.g && 1.0 == sampled_color.b) ) {\n\
          ivec2 pixel_dist = ivec2(gl_FragCoord.xy) - sampling_frag_coord;\n\
          float eucl_distance = sqrt(pixel_dist.x * pixel_dist.x + pixel_dist.y * pixel_dist.y);\n\
          float weight = 1.0 / eucl_distance;\n\
          accumulated_color += vec4(sampled_color, 1.0) * weight;\n\
          accumulated_weight += weight;\n\
        }\n\
\n\
    }\n\
\n\
      for(int x_offset = -1; x_offset < 2; ++x_offset) {\n\
        if( 0 == x_offset) {\n\
          continue;\n\
        }\n\
        ivec2 sampling_frag_coord = ivec2(gl_FragCoord.xy) + ivec2(x_offset, 0);\n\
\n\
        if( \n\
           sampling_frag_coord.x >= image_width || sampling_frag_coord.x < 0 ) {\n\
            continue;\n\
           }\n\
\n\
        vec3 sampled_color = texelFetch(image, ivec2(sampling_frag_coord), 0).rgb;\n\
\n\
        if( !(1.0 == sampled_color.r && 0.0 == sampled_color.g && 1.0 == sampled_color.b) ) {\n\
          ivec2 pixel_dist = ivec2(gl_FragCoord.xy) - sampling_frag_coord;\n\
          float eucl_distance = sqrt(pixel_dist.x * pixel_dist.x + pixel_dist.y * pixel_dist.y);\n\
          float weight = 1.0 / eucl_distance;\n\
          accumulated_color += vec4(sampled_color, 1.0) * weight;\n\
          accumulated_weight += weight;\n\
        }\n\
\n\
    }\n\
\n\
\n\
      if(accumulated_weight > 0.0) {\n\
        //return vec4(0.0, 1.0, 0.0, 1.0);\n\
        return vec4( (accumulated_color / accumulated_weight).rgb, 1.0);\n\
      } else {\n\
        return fallback_color;\n\
      }\n\
    }\n\
\n\
    void main() {\n\
      vec4 color = texture(image, passed_uv).rgba;\n\
    \n\
      if(1.0 == color.r && 0.0 == color.g && 1.0 == color.b) {\n\
      \n\
        fragment_color = weighted_dilation();\n\
       //fragment_color = vec4(1.0, 0.0, 0.0, 1.0); \n\
      } else {\n\
        fragment_color = texelFetch(image, ivec2(gl_FragCoord.xy), 0);\n\
      }\n\
    }";


  //compile shaders
  GLint vertex_shader = compile_shader(vertex_shader_src, GL_VERTEX_SHADER);
  GLint fragment_shader = compile_shader(fragment_shader_src, GL_FRAGMENT_SHADER);

  //create the GL resource and save the handle for the shader program
  dilation_shader_program_ = glCreateProgram();
  glAttachShader(dilation_shader_program_, vertex_shader);
  glAttachShader(dilation_shader_program_, fragment_shader);
  glLinkProgram(dilation_shader_program_);

  //since the program is already linked, we do not need to keep the separate shader stages
  glDetachShader(dilation_shader_program_, vertex_shader);
  glDeleteShader(vertex_shader);
  glDetachShader(dilation_shader_program_, fragment_shader);
  glDeleteShader(fragment_shader);
}


int main( int argc, char** argv ) 
{


  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
  glutInitWindowSize(render_to_texture_width_, render_to_texture_height_);
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

    std::cout << "Optional: -co specifies cost threshold (=infinite)" << std::endl;

    std::cout << "Optional: -cc specifies cell resolution for grid splitting, along longest axis. When 0, no cell splitting is used. (=0)" << std::endl;
    std::cout << "Optional: -ct specifies threshold for grid chart splitting by normal variance (=0.001)" << std::endl;

    std::cout << "Optional: -debug writes charts to obj file, as colours that override texture coordinates (=false)" << std::endl;

    std::cout << "Optional: -tkd num triangles per kdtree node (default: 32000)" << std::endl;

    std::cout << "Optional: -tbvh num triangles per kdtree node (default: 8192)" << std::endl;

    std::cout << "Optional: -single-max: specifies largest possible single output texture (=4096)" << std::endl;

    std::cout << "Optional: -multi-max: specifies largest possible output texture (=8192)" << std::endl;

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

  int single_tex_limit = 4096;
  if (Utils::cmdOptionExists(argv, argv+argc, "-single-max")) {
    single_tex_limit = atoi(Utils::getCmdOption(argv, argv+argc, "-single-max"));
    std::cout << "Single output texture limited to " << single_tex_limit << std::endl;
  }

  int multi_tex_limit = 8192;
  if (Utils::cmdOptionExists(argv, argv+argc, "-multi-max")) {
    multi_tex_limit = atoi(Utils::getCmdOption(argv, argv+argc, "-multi-max"));
    std::cout << "Multi output texture limited to " << multi_tex_limit << std::endl;
  }

  std::cout << "CELL RES " << cell_resolution << std::endl;

#ifdef ADHOC_PARSER
  std::vector<lamure::mesh::triangle_t> all_triangles;
  std::vector<std::string> all_materials;

  std::cout << "Loading obj from " << obj_filename << "..." << std::endl;

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

      if (texture_filename.size() > 3) {
        texture_filename = texture_filename.substr(0, texture_filename.size()-3)+"png";
      }
 
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
      idx_tri.tri_id_ = i;

      all_indexed_triangles.push_back(idx_tri);
    }
  }
  else {
    std::cout << "ERROR: Failed to load .mtl" << std::endl;
    exit(1);
  }

  all_triangles.clear();
#endif

#ifdef VCG_PARSER
  std::vector<indexed_triangle_t> all_indexed_triangles;
  std::map<uint32_t, texture_info> texture_info_map;

  std::cout << "Loading obj from " << obj_filename << "..." << std::endl;

  Utils::load_obj(obj_filename, all_indexed_triangles, texture_info_map);
#endif

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

    if (!CGAL::is_triangle_mesh(polyMesh)){//} || !polyMesh.is_valid(false)){
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

#ifdef RECOMPUTE_NORMALS
    //compute normals
    std::map<face_descriptor,Vector> fnormals;
    std::map<vertex_descriptor,Vector> vnormals;
    CGAL::Polygon_mesh_processing::compute_normals(polyMesh,
      boost::make_assoc_property_map(vnormals),
      boost::make_assoc_property_map(fnormals));

    uint32_t nml_id = 0;
#endif

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
      tri.v0_.nml_ = scm::math::vec3f(it->point().normal.x(), it->point().normal.y(), it->point().normal.z());
      ++hc;

      it = polyMesh.vertices_begin();
      std::advance(it, index[Polyhedron::Vertex_const_iterator(hc->vertex())]);
      tri.v1_.pos_ = scm::math::vec3f(it->point().x(), it->point().y(), it->point().z());
      tri.v1_.tex_ = scm::math::vec2f(fi->t_coords[1].x(), fi->t_coords[1].y());
      tri.v1_.nml_ = scm::math::vec3f(it->point().normal.x(), it->point().normal.y(), it->point().normal.z());
      ++hc;

      it = polyMesh.vertices_begin();
      std::advance(it, index[Polyhedron::Vertex_const_iterator(hc->vertex())]);
      tri.v2_.pos_ = scm::math::vec3f(it->point().x(), it->point().y(), it->point().z());
      tri.v2_.tex_ = scm::math::vec2f(fi->t_coords[2].x(), fi->t_coords[2].y());
      tri.v2_.nml_ = scm::math::vec3f(it->point().normal.x(), it->point().normal.y(), it->point().normal.z());
      ++hc;


      tri.area_id = num_areas;
      tri.chart_id = per_node_chart_id_map_it.second[fi->id()];
      tri.tex_id = fi->tex_id;
      tri.tri_id = fi->tri_id;

      //if (tri.tri_id >= 0 && tri.tri_id < all_indexed_triangles.size()) {
        triangles.push_back(tri);
      //}

 
    }
    ++num_areas;
  
  }

  std::cout << "Creating LOD hierarchy..." << std::endl;

  auto bvh = std::make_shared<lamure::mesh::bvh>(triangles, num_tris_per_node_bvh);



  //here, we make sure that triangles is in the same ordering as the leaf level triangles
  
  first_leaf_id = bvh->get_first_node_id_of_depth(bvh->get_depth());
  num_leaf_ids = bvh->get_length_of_depth(bvh->get_depth());
  
  triangles.clear();
  for (uint32_t node_id = first_leaf_id; node_id < first_leaf_id+num_leaf_ids; ++node_id) {
    triangles.insert(triangles.end(), bvh->get_triangles(node_id).begin(), bvh->get_triangles(node_id).end());
  }

  std::cout << "Reordered " << triangles.size() << " triangles" << std::endl;

  std::cout << "Preparing charts..." << std::endl;

  //prep charts for projection
  std::map<uint32_t, std::map<uint32_t, chart>> chart_map;

  for (uint32_t tri_id = 0; tri_id < triangles.size(); ++tri_id) {
    const auto& tri = triangles[tri_id];
    uint32_t area_id = tri.area_id;
    uint32_t chart_id = tri.chart_id;

    if (chart_id != -1 && tri.get_area() > 0.f) {
      chart_map[area_id][chart_id].id_ = chart_id;
      chart_map[area_id][chart_id].original_triangle_ids_.insert(tri_id);
    }
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
        scm::math::vec3d(std::numeric_limits<float>::lowest())
      );

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
        if (tri.chart_id != -1 && tri.get_area() > 0.f) {
          if (chart.box_.contains(scm::math::vec3d(tri.v0_.pos_))
            || chart.box_.contains(scm::math::vec3d(tri.v1_.pos_))
            || chart.box_.contains(scm::math::vec3d(tri.v2_.pos_))) {

            chart.all_triangle_ids_.insert(tri_id);
          }
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




  const float packing_scale = 400.f;

  std::vector<rectangle> area_rects;

  for (uint32_t area_id = 0; area_id < num_areas; ++area_id) {
    calculate_chart_tex_space_sizes(chart_map[area_id], triangles, texture_info_map);

    project_charts(chart_map[area_id], triangles);

    std::cout << "Projected " << chart_map[area_id].size() << " charts for area " << area_id << std::endl;

    std::cout << "Running rectangle packing for area " << area_id << std::endl;
    
    //init the rectangles
    std::vector<rectangle> rects;
    for (auto& chart_it : chart_map[area_id]) {
      chart& chart = chart_it.second;
      if (chart.original_triangle_ids_.size() > 0) {
        rectangle rect = chart.rect_;
        rect.max_ *= packing_scale;
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
  rectangle image_rect = pack(area_rects);
  std::cout << "Packing of all areas complete (" << image_rect.max_.x << ", " << image_rect.max_.y << ")" << std::endl;

  std::cout << "Applying texture space transformation..." << std::endl;

  // TODO: this loop is way too slow
  for (auto& area_rect : area_rects) {
    std::cout << "Area " << area_rect.id_ << " min: (" << area_rect.min_.x << ", " << area_rect.min_.y << ")" << std::endl;
    std::cout << "Area " << area_rect.id_ << " max: (" << area_rect.max_.x << ", " << area_rect.max_.y << ")" << std::endl;

    //next, apply the global transformation from area packing onto all individual chart rects per area
    for (auto& chart_it : chart_map[area_rect.id_]) {
      auto& chart = chart_it.second;
      chart.rect_.min_ += area_rect.min_;
     
      //apply this transformation to the new parameterization

      // TODO: can be parallel!
      for (auto tri_id : chart.all_triangle_ids_) {
        if ((chart.rect_.flipped_ && !area_rect.flipped_) || (area_rect.flipped_ && !chart.rect_.flipped_)) {
          float temp = chart.all_triangle_new_coods_[tri_id][0].x;
          chart.all_triangle_new_coods_[tri_id][0].x = chart.all_triangle_new_coods_[tri_id][0].y;
          chart.all_triangle_new_coods_[tri_id][0].y = temp;

          temp = chart.all_triangle_new_coods_[tri_id][1].x;
          chart.all_triangle_new_coods_[tri_id][1].x = chart.all_triangle_new_coods_[tri_id][1].y;
          chart.all_triangle_new_coods_[tri_id][1].y = temp;

          temp = chart.all_triangle_new_coods_[tri_id][2].x;
          chart.all_triangle_new_coods_[tri_id][2].x = chart.all_triangle_new_coods_[tri_id][2].y;
          chart.all_triangle_new_coods_[tri_id][2].y = temp;
        }

        for (uint32_t i = 0; i < 3; ++i) {
          chart.all_triangle_new_coods_[tri_id][i] *= packing_scale;
          chart.all_triangle_new_coods_[tri_id][i].x += chart.rect_.min_.x;
          chart.all_triangle_new_coods_[tri_id][i].y += chart.rect_.min_.y;
          chart.all_triangle_new_coods_[tri_id][i].x /= image_rect.max_.x;
          chart.all_triangle_new_coods_[tri_id][i].y /= image_rect.max_.x;
        }
      }
    }
  }

  
  struct blit_vertex_t {
    scm::math::vec2f old_coord_;
    scm::math::vec2f new_coord_;
  };

  //use 2D array to account for different textures (if no textures were found, make sure it has at least one row)
  std::vector<std::vector<blit_vertex_t>> to_upload_per_texture;
  std::cout << "texture info map size: " << texture_info_map.size() << std::endl;
  to_upload_per_texture.resize(texture_info_map.size());


  //replacing texture coordinates in LOD file
  //...and at the same time, we will fill the upload per texture list
  std::cout << "Updating texture coordinates in leaf-level LOD nodes..." << std::endl;
  uint32_t num_invalid_tris = 0;
  uint32_t num_dropped_tris = 0;

  for (uint32_t node_id = first_leaf_id; node_id < first_leaf_id+num_leaf_ids; ++node_id) {

    auto& tris = bvh->get_triangles(node_id);

    for (int local_tri_id = 0; local_tri_id < tris.size(); ++local_tri_id) {
      int32_t tri_id = ((node_id-first_leaf_id)*(bvh->get_primitives_per_node()/3))+local_tri_id;

      auto& tri = tris[local_tri_id];
      //std::cout << "tri id << " << tri.tri_id << " area id " << tri.area_id << " chart id " << tri.chart_id << std::endl;

      if (tri.chart_id != -1 && tri.get_area() > 0.f) {

        auto& chart = chart_map[tri.area_id][tri.chart_id];

        if (chart.all_triangle_ids_.find(tri_id) != chart.all_triangle_ids_.end()) {

          uint32_t not_found_mask = 0;

          //create per-texture render list
          if (tri.tex_id != -1) {

            const auto& old_tri = all_indexed_triangles[tri.tri_id];

            double epsilon = FLT_EPSILON;
            
            //obtain original coordinates 
            //since indexed cgal polyhedra dont preserve texture coordinates correctly
            if (scm::math::length(tri.v0_.pos_-old_tri.v0_.pos_) < epsilon) tri.v0_.tex_ = old_tri.v0_.tex_;
            else if (scm::math::length(tri.v0_.pos_-old_tri.v1_.pos_) < epsilon) tri.v0_.tex_ = old_tri.v1_.tex_;
            else if (scm::math::length(tri.v0_.pos_-old_tri.v2_.pos_) < epsilon) tri.v0_.tex_ = old_tri.v2_.tex_;
            else { 
              std::cout << "WARNING: tex coord v0 could not be disambiguated (" << (int)(tri.tri_id == old_tri.tri_id_) << ")" << std::endl;
              not_found_mask |= 1;
            }

            if (scm::math::length(tri.v1_.pos_-old_tri.v0_.pos_) < epsilon) tri.v1_.tex_ = old_tri.v0_.tex_;
            else if (scm::math::length(tri.v1_.pos_-old_tri.v1_.pos_) < epsilon) tri.v1_.tex_ = old_tri.v1_.tex_;
            else if (scm::math::length(tri.v1_.pos_-old_tri.v2_.pos_) < epsilon) tri.v1_.tex_ = old_tri.v2_.tex_;
            else { 
              std::cout << "WARNING: tex coord v1 could not be disambiguated (" << (int)(tri.tri_id == old_tri.tri_id_) << ")" << std::endl;
              not_found_mask |= 2;
            }

            if (scm::math::length(tri.v2_.pos_-old_tri.v0_.pos_) < epsilon) tri.v2_.tex_ = old_tri.v0_.tex_;
            else if (scm::math::length(tri.v2_.pos_-old_tri.v1_.pos_) < epsilon) tri.v2_.tex_ = old_tri.v1_.tex_;
            else if (scm::math::length(tri.v2_.pos_-old_tri.v2_.pos_) < epsilon) tri.v2_.tex_ = old_tri.v2_.tex_;
            else { 
              std::cout << "WARNING: tex coord v2 could not be disambiguated (" << (int)(tri.tri_id == old_tri.tri_id_) << ")" << std::endl;
              not_found_mask |= 4;
            }

            if (not_found_mask == 0) {
              to_upload_per_texture[tri.tex_id].push_back(blit_vertex_t{tri.v0_.tex_, chart.all_triangle_new_coods_[tri_id][0]});
              to_upload_per_texture[tri.tex_id].push_back(blit_vertex_t{tri.v1_.tex_, chart.all_triangle_new_coods_[tri_id][1]});
              to_upload_per_texture[tri.tex_id].push_back(blit_vertex_t{tri.v2_.tex_, chart.all_triangle_new_coods_[tri_id][2]});
            }
          }
          else {
            ++num_dropped_tris;
          }

          //override texture coordinates
          if (not_found_mask & 1 == 0) tri.v0_.tex_ = chart.all_triangle_new_coods_[tri_id][0];
          if (not_found_mask & 2 == 0) tri.v1_.tex_ = chart.all_triangle_new_coods_[tri_id][1];
          if (not_found_mask & 4 == 0) tri.v2_.tex_ = chart.all_triangle_new_coods_[tri_id][2];

          tri.v0_.tex_.y = 1.0-tri.v0_.tex_.y; //flip y coord
          tri.v1_.tex_.y = 1.0-tri.v1_.tex_.y;
          tri.v2_.tex_.y = 1.0-tri.v2_.tex_.y;

        }
        else {
          ++num_dropped_tris;
        }
      }
      else {
        ++num_invalid_tris;
      }
    }
  }

  std::cout << "Updating texture coordinates in inner LOD nodes..." << std::endl;
  for (uint32_t node_id = 0; node_id < first_leaf_id; ++node_id) {
    
    auto& tris = bvh->get_triangles(node_id);
    for (int local_tri_id = 0; local_tri_id < tris.size(); ++local_tri_id) {
      auto& tri = tris[local_tri_id];
      if (tri.chart_id != -1) {
        auto& proj_info = chart_map[tri.area_id][tri.chart_id].projection;
        rectangle& chart_rect = chart_map[tri.area_id][tri.chart_id].rect_;
        rectangle& area_rect = area_rects[tri.area_id];

        //at this point we will need to project all triangles of inner nodes to their respective charts using the corresponding chart plane
        
        scm::math::vec3f original_v;

        for (uint32_t i = 0; i < 3; ++i) {
          switch (i) {
            case 0: original_v = tri.v0_.pos_; break;
            case 1: original_v = tri.v1_.pos_; break;
            case 2: original_v = tri.v2_.pos_; break;
            default: break;
          }
          scm::math::vec2f projected_v = project_to_plane(original_v, proj_info.proj_normal, proj_info.proj_centroid, proj_info.proj_world_up);
          
          projected_v -= proj_info.tex_coord_offset; //correct by offset (so that min uv coord = 0) 
          
          projected_v /= proj_info.largest_max; //apply normalisation factor
          if ((chart_rect.flipped_ && !area_rect.flipped_) || (area_rect.flipped_ && !chart_rect.flipped_)) { //flip if needed
            float temp = projected_v.x;
            projected_v.x = projected_v.y;
            projected_v.y = temp;
          }
          projected_v *= packing_scale; //scale
          projected_v += chart_rect.min_; //offset position in texture
          projected_v /= image_rect.max_; //scale down to normalised image space
          projected_v.y = 1.0 - projected_v.y; //flip y coord

          //replace existing coords
          switch (i) {
            case 0: tri.v0_.tex_ = projected_v; break;
            case 1: tri.v1_.tex_ = projected_v; break;
            case 2: tri.v2_.tex_ = projected_v; break;
            default: break;
          }
        }
      }
      else {
        ++num_invalid_tris;
      }
    }

  }

  std::cout << "Num tris with invalid chart ids encountered: " << num_invalid_tris << std::endl;
  std::cout << "Num dropped tris encountered: " << num_dropped_tris << std::endl;

  std::string bvh_filename = obj_filename.substr(0, obj_filename.size()-4)+".bvh";
  bvh->write_bvh_file(bvh_filename);
  std::cout << "Bvh file written to " << bvh_filename << std::endl;

  std::string lod_filename = obj_filename.substr(0, obj_filename.size()-4)+".lod";
  bvh->write_lod_file(lod_filename);
  std::cout << "Lod file written to " << lod_filename << std::endl;
  
  //cleanup
  std::cout << "Cleanup bvh" << std::endl;
  bvh.reset();


  std::cout << "Single texture size limit: " << single_tex_limit << std::endl;
  std::cout << "Multi texture size limit: " << multi_tex_limit << std::endl;


  render_to_texture_width_ = std::max(single_tex_limit, 4096);
  render_to_texture_height_ = std::max(single_tex_limit, 4096);

  multi_tex_limit = std::max(render_to_texture_width_, multi_tex_limit);

  full_texture_width_ = render_to_texture_width_;
  full_texture_height_ = render_to_texture_height_;
  
  //double texture size up to 8k if a given percentage of charts do not have enough pixels
  const double target_percentage_charts_with_enough_pixels = 0.90;
  std::cout << "Adjusting final texture size (" << full_texture_width_ << " x " << full_texture_height_ << ")" << std::endl;

  calculate_new_chart_tex_space_sizes(chart_map, triangles, scm::math::vec2i(full_texture_width_, full_texture_height_));
  while (!is_output_texture_big_enough(chart_map, target_percentage_charts_with_enough_pixels)) {

    if (std::max(full_texture_width_, full_texture_height_) >= multi_tex_limit) {
      std::cout << "Maximum texture size limit reached (" << full_texture_width_ << " x " << full_texture_height_ << ")" << std::endl;
      break;
    }

    full_texture_width_ *= 2;
    full_texture_height_ *= 2;

    std::cout << "Not enough pixels! Adjusting final texture size (" << full_texture_width_ << " x " << full_texture_height_ << ")" << std::endl;

    calculate_new_chart_tex_space_sizes(chart_map, triangles, scm::math::vec2i(full_texture_width_, full_texture_height_));
  }


  //if output texture is bigger than 8k, create a set if viewports that will be rendered separately
  if (full_texture_width_ > render_to_texture_width_ || full_texture_height_ > render_to_texture_height_) {
    //calc num of viewports needed from size of output texture
    int viewports_w = std::ceil(full_texture_width_ / render_to_texture_width_);
    int viewports_h = std::ceil(full_texture_height_ / render_to_texture_height_);

    scm::math::vec2f viewport_normed_size(1.f / viewports_w, 1.f / viewports_h);

    //create a vector of viewports needed 
    for (int y = 0; y < viewports_h; ++y) {
      for (int x = 0; x < viewports_w; ++x) {
        viewport vp;
        vp.normed_dims = viewport_normed_size;
        vp.normed_offset = scm::math::vec2f(viewport_normed_size.x * x, viewport_normed_size.y * y);
        viewports_.push_back(vp);

        //std::cout << "Viewport " << viewports.size() -1 << ": " << viewport_normed_size.x << "x" << viewport_normed_size.y << " at (" << vp.normed_offset.x << "," << vp.normed_offset.y << ")\n";
      }
    }
  }
  else {
    viewport single_viewport;
    single_viewport.normed_offset = scm::math::vec2f(0.f,0.f);
    single_viewport.normed_dims = scm::math::vec2f(1.0,1.0);
    viewports_.push_back(single_viewport);

    //std::cout << "Viewport " << viewports.size() -1 << ": " << single_viewport.normed_dims.x << "x" << single_viewport.normed_dims.y << " at (" << single_viewport.normed_offset.x << "," << single_viewport.normed_offset.y << ")\n";
  }
  std::cout << "Created " << viewports_.size() << " viewports to render multiple output textures" << std::endl;

  

  
  std::cout << "Loading all textures..." << std::endl;
  for (auto tex_it : texture_info_map) {
    textures_.push_back(load_image(tex_it.second.filename_));
  }


  std::cout << "Compiling shaders..." << std::endl;
  make_shader_program();
  make_dilation_shader_program();

  std::cout << "Creating framebuffers..." << std::endl;
  
  //create output frame buffers
  for (int i = 0; i < 2; ++i) {
    frame_buffers_.push_back(std::make_shared<frame_buffer_t>(1, render_to_texture_width_, render_to_texture_height_, GL_RGBA, GL_LINEAR));
  }

  //create vertex buffer for dilation
  float screen_space_quad_geometry[30] {
    -1.0, -1.0, 0.0, 0.0,
     1.0, -1.0, 1.0, 0.0,
    -1.0,  1.0, 0.0, 1.0,

      1.0, -1.0, 1.0, 0.0,
      1.0,  1.0, 1.0, 1.0,
     -1.0,  1.0, 0.0, 1.0
  };
  glGenBuffers(1, &dilation_vertex_buffer_);
  glBindBuffer(GL_ARRAY_BUFFER, dilation_vertex_buffer_);
  glBufferData(GL_ARRAY_BUFFER, 6*4*sizeof(float), &screen_space_quad_geometry[0], GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);


  //set the viewport size
  glViewport(0, 0, (GLsizei)render_to_texture_width_, (GLsizei)render_to_texture_height_);

  //set background colour
  glClearColor(1.0f, 0.0f, 1.0f, 1.0f);

  glGenBuffers(1, &vertex_buffer_);

  std::vector<std::vector<uint8_t>> area_images(viewports_.size());

  for (uint32_t view_id = 0; view_id < viewports_.size(); ++view_id) {
    std::cout << "Rendering into viewport " << view_id << "..." << std::endl;

    viewport vport = viewports_[view_id];

    std::cout << "Viewport start: " << vport.normed_offset.x << ", " << vport.normed_offset.y << std::endl;
    std::cout << "Viewport size: " << vport.normed_dims.x << ", " << vport.normed_dims.y << std::endl;

    frame_buffers_[0]->enable();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (uint32_t i = 0; i < to_upload_per_texture.size(); ++i) {
      
      uint32_t num_vertices = to_upload_per_texture[i].size();

      if (num_vertices == 0) {
        std::cout << "Nothing to render for texture " << i << " ("<< texture_info_map[i].filename_ << ")" << std::endl;
        continue;
      }

      std::cout << "Rendering from texture " << i << " ("<< texture_info_map[i].filename_ << ")" << std::endl;

      glUseProgram(shader_program_);

      //upload this vector to GPU
      glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
      glBufferData(GL_ARRAY_BUFFER, num_vertices*sizeof(blit_vertex_t), &to_upload_per_texture[i][0], GL_STREAM_DRAW);

      
      //define the layout of the vertex buffer:
      //setup 2 attributes per vertex (2x texture coord)
      glEnableVertexAttribArray(0);
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(blit_vertex_t), (void*)0);
      glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(blit_vertex_t), (void*)(2*sizeof(float)));

      //get texture location
      int slot = 0;
      glUniform1i(glGetUniformLocation(shader_program_, "image"), slot);
      glUniform2f(glGetUniformLocation(shader_program_, "viewport_offset"), vport.normed_offset[0], vport.normed_offset[1]);
      glUniform2f(glGetUniformLocation(shader_program_, "viewport_scale"), vport.normed_dims[0], vport.normed_dims[1]);

      glActiveTexture(GL_TEXTURE0 + slot);

      //here, enable the current texture
      textures_[i]->enable(slot);

      //draw triangles from the currently bound buffer
      glDrawArrays(GL_TRIANGLES, 0, num_vertices);

      //unbind, unuse
      glBindBuffer(GL_ARRAY_BUFFER, 0);
      glUseProgram(0);

      textures_[i]->disable();


    } //end for each texture

    frame_buffers_[0]->disable();

    uint32_t current_framebuffer = 0;
    if (true) {

      std::cout << "Dilating view " << view_id << "..." << std::endl;

      uint32_t num_dilations = render_to_texture_width_/2;
    
      for (int i = 0; i < num_dilations; ++i) {

        current_framebuffer = (i+1)%2;

        frame_buffers_[current_framebuffer]->enable();
        int current_texture = 0;
        if (current_framebuffer == 0) {
          current_texture = 1;
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(dilation_shader_program_);
        glBindBuffer(GL_ARRAY_BUFFER, dilation_vertex_buffer_);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(blit_vertex_t), (void*)0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(blit_vertex_t), (void*)(2*sizeof(float)));

        int slot = 0;
        glUniform1i(glGetUniformLocation(dilation_shader_program_, "image"), slot);
        glUniform1i(glGetUniformLocation(dilation_shader_program_, "image_width"), render_to_texture_width_);
        glUniform1i(glGetUniformLocation(dilation_shader_program_, "image_height"), render_to_texture_height_);
        glActiveTexture(GL_TEXTURE0 + slot);
        frame_buffers_[current_texture]->bind_texture(slot);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glUseProgram(0);

        frame_buffers_[current_texture]->unbind_texture(slot);

        frame_buffers_[current_framebuffer]->disable();
      }

    }

    std::vector<uint8_t> pixels;
    frame_buffers_[current_framebuffer]->get_pixels(0, pixels);
    area_images[view_id] = pixels;

  } //end for each viewport

  std::cout << "Producing final texture..." << std::endl;

#if 1

  //concatenate all area images to one big texture
  uint32_t num_bytes_per_pixel = 4;
  std::vector<uint8_t> final_texture(full_texture_width_*full_texture_height_*num_bytes_per_pixel);
  
  uint32_t num_lookups_per_line = full_texture_width_ / render_to_texture_width_;

  for (uint32_t y = 0; y < full_texture_height_; ++y) { //for each line
    for (uint32_t tex_x = 0; tex_x < num_lookups_per_line; ++tex_x) {
      void* dst = ((void*)&final_texture[0]) + y*full_texture_width_*num_bytes_per_pixel + tex_x*render_to_texture_width_*num_bytes_per_pixel;
      uint32_t tex_y = y / render_to_texture_height_;
      uint32_t tex_id = tex_y * num_lookups_per_line + tex_x;
      void* src = ((void*)&area_images[tex_id][0]) + (y % render_to_texture_height_)*render_to_texture_width_*num_bytes_per_pixel; //+ 0;

      memcpy(dst, src, render_to_texture_width_*num_bytes_per_pixel);
    }
  }


  std::string image_filename = bvh_filename.substr(0, bvh_filename.size()-4) + "_texture.png";
  save_image(image_filename, final_texture, full_texture_width_, full_texture_height_);

#else 
  std::ofstream raw_file;
  std::string image_filename = bvh_filename.substr(0, bvh_filename.size()-4) 
    + "_rgba_w" + std::to_string(full_texture_width_) + "_h" + std::to_string(full_texture_height_) 
    + ".data";
  raw_file.open(image_filename, std::ios::out | std::ios::trunc | std::ios::binary);

  //concatenate all area images to one big texture
  uint32_t num_bytes_per_pixel = 4;
  
  uint32_t num_lookups_per_line = full_texture_width_ / render_to_texture_width_;

  for (uint32_t y = 0; y < full_texture_height_; ++y) { //for each line
    for (uint32_t tex_x = 0; tex_x < num_lookups_per_line; ++tex_x) {
      uint32_t tex_y = y / render_to_texture_height_;
      uint32_t tex_id = tex_y * num_lookups_per_line + tex_x;
      char* src = ((char*)&area_images[tex_id][0]) + (y % render_to_texture_height_)*render_to_texture_width_*num_bytes_per_pixel; //+ 0;

      raw_file.write(src, render_to_texture_width_*num_bytes_per_pixel);
    }
  }

  raw_file.close();

#endif

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
