// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <set>

#include <lamure/types.h>
#include <lamure/ren/dataset.h>
#include <lamure/ren/bvh.h>
#include <lamure/ren/lod_stream.h>

#include "lodepng.h"
#include "texture.h"
#include "frame_buffer.h"


struct vertex {
  scm::math::vec3f pos_;
  scm::math::vec3f nml_;
  scm::math::vec2f old_coord_;
  scm::math::vec2f new_coord_;
};

struct triangle {
  vertex v0_;
  vertex v1_;
  vertex v2_;
};

struct rectangle {
  scm::math::vec2f min_;
  scm::math::vec2f max_;
  int id_;
  bool flipped_;
};


struct projection_info {
  scm::math::vec3f proj_centroid;
  scm::math::vec3f proj_normal;
  scm::math::vec3f proj_world_up;

  rectangle tex_space_rect;
  scm::math::vec2f tex_coord_offset;

  float largest_max;
};

struct chart {
  int id_;
  rectangle rect_;
  lamure::bounding_box box_;
  std::set<int> all_triangle_ids_;
  std::set<int> original_triangle_ids_;
  projection_info projection;
  double real_to_tex_ratio_old;
  double real_to_tex_ratio_new;
};



int window_width_ = 1024;
int window_height_ = 1024;

int elapsed_ms_ = 0;
int num_vertices_ = 0;


GLuint shader_program_; //contains GPU-code
GLuint vertex_buffer_; //contains 3d model
std::shared_ptr<texture_t> texture_; //contains GPU image

std::shared_ptr<frame_buffer_t> frame_buffer_; //contains resulting image


struct blit_vertex {
  scm::math::vec2f old_coord_;
  scm::math::vec2f new_coord_;
};





char* get_cmd_option(char** begin, char** end, const std::string & option) {
    char** it = std::find(begin, end, option);
    if (it != end && ++it != end)
        return *it;
    return 0;
}

bool cmd_option_exists(char** begin, char** end, const std::string& option) {
    return std::find(begin, end, option) != end;
}


int load_chart_file(std::string chart_file, std::vector<int>& chart_id_per_triangle) {

  int num_charts = 0;

  std::ifstream file(chart_file);

  std::string line;
  while (std::getline(file, line)) {
    std::stringstream ss(line);
    std::string chart_id_str;

    while (std::getline(ss, chart_id_str, ' ')) {
      
      int chart_id = atoi(chart_id_str.c_str());
      num_charts = std::max(num_charts, chart_id+1);
      chart_id_per_triangle.push_back(chart_id);
    }
  }


  file.close();

  return num_charts;
}



std::shared_ptr<texture_t> load_image(const std::string& filepath) {
  std::vector<unsigned char> img;
  unsigned int width = 0;
  unsigned int height = 0;
  int tex_error = lodepng::decode(img, width, height, filepath);
  if (tex_error) {
    std::cout << "unable to load image file " << filepath << std::endl;
  }
  std::cout << "image " << filepath << " loaded" << std::endl;

  auto texture = std::make_shared<texture_t>(width, height, GL_LINEAR);
  texture->set_pixels(&img[0]);

  return texture;
}


scm::math::vec2f project_to_plane(
  const scm::math::vec3f& v, scm::math::vec3f& plane_normal, 
  const scm::math::vec3f& centroid,
  const scm::math::vec3f& world_up
  ) {

  scm::math::vec3f v_minus_p(
    v.x - centroid.x,
    v.y - centroid.y,
    v.z - centroid.z);

  auto plane_right = scm::math::cross(plane_normal, world_up);
  plane_right = scm::math::normalize(plane_right);
  auto plane_up = scm::math::cross(plane_normal, plane_right);
  plane_up = scm::math::normalize(plane_up);


  //project vertices to the plane
  scm::math::vec2f projected_v(
    scm::math::dot(v_minus_p, plane_right),
    scm::math::dot(v_minus_p, plane_up));

  return projected_v;

}

void project(std::vector<chart>& charts, std::vector<triangle>& triangles) {

  //keep a record of the largest chart edge
  float largest_max = 0.f;


  std::cout << "Projecting to plane:\n";

  //iterate all charts
  for (uint32_t chart_id = 0; chart_id < charts.size(); ++chart_id) {
    chart& chart = charts[chart_id];

    scm::math::vec3f avg_normal(0.f, 0.f, 0.f);
    scm::math::vec3f centroid(0.f, 0.f, 0.f);

    // compute average normal and centroid
    for (auto tri_id : chart.all_triangle_ids_) {

      triangle& tri = triangles[tri_id];

      avg_normal.x += tri.v0_.nml_.x;
      avg_normal.y += tri.v0_.nml_.y;
      avg_normal.z += tri.v0_.nml_.z;

      avg_normal.x += tri.v1_.nml_.x;
      avg_normal.y += tri.v1_.nml_.y;
      avg_normal.z += tri.v1_.nml_.z;

      avg_normal.x += tri.v2_.nml_.x;
      avg_normal.y += tri.v2_.nml_.y;
      avg_normal.z += tri.v2_.nml_.z;

      centroid.x += tri.v0_.pos_.x;
      centroid.y += tri.v0_.pos_.y;
      centroid.z += tri.v0_.pos_.z;

      centroid.x += tri.v1_.pos_.x;
      centroid.y += tri.v1_.pos_.y;
      centroid.z += tri.v1_.pos_.z;

      centroid.x += tri.v2_.pos_.x;
      centroid.y += tri.v2_.pos_.y;
      centroid.z += tri.v2_.pos_.z;
    }

    avg_normal.x /= (float)(chart.all_triangle_ids_.size()*3);
    avg_normal.y /= (float)(chart.all_triangle_ids_.size()*3);
    avg_normal.z /= (float)(chart.all_triangle_ids_.size()*3);

    centroid.x /= (float)(chart.all_triangle_ids_.size()*3);
    centroid.y /= (float)(chart.all_triangle_ids_.size()*3);
    centroid.z /= (float)(chart.all_triangle_ids_.size()*3);

    // std::cout << "chart id " << chart_id << ", num tris: " << chart.all_triangle_ids_.size() << std::endl;
    // std::cout << "  avg_n: (" << avg_normal.x << " " << avg_normal.y << " " << avg_normal.z << ")" << std::endl;
    // std::cout << "  centr: (" << centroid.x << " " << centroid.y << " " << centroid.z << ")" << std::endl;

    avg_normal = scm::math::normalize(avg_normal);
    
    //compute world up vector
    scm::math::vec3f world_up(0.f, 1.f, 0.f);
    if (std::abs(scm::math::dot(world_up, avg_normal)) > 0.8f) {
      world_up = scm::math::vec3f(0.f, 0.f, 1.f);
    }

    //record centroid, projection normal and world up for calculating inner triangle UVs later on
    chart.projection.proj_centroid = centroid;
    chart.projection.proj_normal = avg_normal;
    chart.projection.proj_world_up = world_up;


    //project all vertices into that plane
    for (auto tri_id : chart.all_triangle_ids_) {

      triangle& tri = triangles[tri_id];

      scm::math::vec2f projected_v0 = project_to_plane(tri.v0_.pos_, avg_normal, centroid, world_up);
      scm::math::vec2f projected_v1 = project_to_plane(tri.v1_.pos_, avg_normal, centroid, world_up);
      scm::math::vec2f projected_v2 = project_to_plane(tri.v2_.pos_, avg_normal, centroid, world_up);

      tri.v0_.new_coord_ = projected_v0;
      tri.v1_.new_coord_ = projected_v1;
      tri.v2_.new_coord_ = projected_v2;

      // std::cout << "tri_id " << tri_id << " projected v0: (" << projected_v0.x << " " << projected_v0.y << ")" << std::endl;
      // std::cout << "tri_id " << tri_id << " projected v1: (" << projected_v1.x << " " << projected_v1.y << ")" << std::endl;
      // std::cout << "tri_id " << tri_id << " projected v2: (" << projected_v2.x << " " << projected_v2.y << ")" << std::endl;

    }

    //compute rectangle for the current chart
    //initialize rectangle min and max
    chart.rect_.id_ = chart.id_;
    chart.rect_.flipped_ = false;
    chart.rect_.min_.x = std::numeric_limits<float>::max();
    chart.rect_.min_.y = std::numeric_limits<float>::max();
    chart.rect_.max_.x = std::numeric_limits<float>::lowest();
    chart.rect_.max_.y = std::numeric_limits<float>::lowest();

    //compute the bounding rectangle for each chart
    for (auto tri_id : chart.all_triangle_ids_) {
      triangle& tri = triangles[tri_id];

      chart.rect_.min_.x = std::min(chart.rect_.min_.x, tri.v0_.new_coord_.x);
      chart.rect_.min_.y = std::min(chart.rect_.min_.y, tri.v0_.new_coord_.y);
      chart.rect_.min_.x = std::min(chart.rect_.min_.x, tri.v1_.new_coord_.x);
      chart.rect_.min_.y = std::min(chart.rect_.min_.y, tri.v1_.new_coord_.y);
      chart.rect_.min_.x = std::min(chart.rect_.min_.x, tri.v2_.new_coord_.x);
      chart.rect_.min_.y = std::min(chart.rect_.min_.y, tri.v2_.new_coord_.y);

      chart.rect_.max_.x = std::max(chart.rect_.max_.x, tri.v0_.new_coord_.x);
      chart.rect_.max_.y = std::max(chart.rect_.max_.y, tri.v0_.new_coord_.y);
      chart.rect_.max_.x = std::max(chart.rect_.max_.x, tri.v1_.new_coord_.x);
      chart.rect_.max_.y = std::max(chart.rect_.max_.y, tri.v1_.new_coord_.y);
      chart.rect_.max_.x = std::max(chart.rect_.max_.x, tri.v2_.new_coord_.x);
      chart.rect_.max_.y = std::max(chart.rect_.max_.y, tri.v2_.new_coord_.y);
    }

    //record offset for rendering from texture
    chart.projection.tex_coord_offset.x = chart.rect_.min_.x;
    chart.projection.tex_coord_offset.y = chart.rect_.min_.y;

    //shift min to the origin
    chart.rect_.max_.x -= chart.rect_.min_.x;
    chart.rect_.max_.y -= chart.rect_.min_.y;

    //update largest chart
    largest_max = std::max(largest_max, chart.rect_.max_.x);
    largest_max = std::max(largest_max, chart.rect_.max_.y);




    // shift projected coordinates to min = 0
    for (auto tri_id : chart.all_triangle_ids_) {
      triangle& tri = triangles[tri_id];

      tri.v0_.new_coord_.x -= chart.rect_.min_.x;
      tri.v0_.new_coord_.y -= chart.rect_.min_.y;
      tri.v1_.new_coord_.x -= chart.rect_.min_.x;
      tri.v1_.new_coord_.y -= chart.rect_.min_.y;
      tri.v2_.new_coord_.x -= chart.rect_.min_.x;
      tri.v2_.new_coord_.y -= chart.rect_.min_.y;

      // std::cout << "tri_id " << tri_id << " projected v0: (" << tri.v0_.new_coord_.x << " " << tri.v0_.new_coord_.y << ")" << std::endl;
      // std::cout << "tri_id " << tri_id << " projected v1: (" << tri.v1_.new_coord_.x << " " << tri.v1_.new_coord_.y << ")" << std::endl;
      // std::cout << "tri_id " << tri_id << " projected v2: (" << tri.v2_.new_coord_.x << " " << tri.v2_.new_coord_.y << ")" << std::endl;

    }

    chart.rect_.min_.x = 0.f;
    chart.rect_.min_.y = 0.f;


    // std::cout << "chart " << chart_id << " rect min: " << chart.rect_.min_.x << ", " << chart.rect_.min_.y << std::endl;
    // std::cout << "chart " << chart_id << " rect max: " << chart.rect_.max_.x << ", " << chart.rect_.max_.y << std::endl;

  }

  std::cout << "normalize charts but keep relative size:\n";
  for (int chart_id = 0; chart_id < charts.size(); ++chart_id) {
    chart& chart = charts[chart_id];
    
    // normalize largest_max to 1
    for (auto tri_id : chart.all_triangle_ids_) {
      triangle& tri = triangles[tri_id];

      tri.v0_.new_coord_.x /= largest_max;
      tri.v0_.new_coord_.y /= largest_max;
      tri.v1_.new_coord_.x /= largest_max;
      tri.v1_.new_coord_.y /= largest_max;
      tri.v2_.new_coord_.x /= largest_max;
      tri.v2_.new_coord_.y /= largest_max;
    }

    chart.rect_.max_.x /= largest_max;
    chart.rect_.max_.y /= largest_max;

    chart.projection.largest_max = largest_max;

    // std::cout << "chart " << chart_id << " rect min: " << chart.rect_.min_.x << ", " << chart.rect_.min_.y << std::endl;
    // std::cout << "chart " << chart_id << " rect max: " << chart.rect_.max_.x << ", " << chart.rect_.max_.y << std::endl;

  }
}



void save_image(std::string filename, std::vector<uint8_t> image, int width, int height) {
  int tex_error = lodepng::encode(filename, image, width, height);
  if (tex_error) {
    std::cout << "unable to save image file " << filename << std::endl;
  }
  std::cout << "image " << filename << " saved" << std::endl;

}


void save_image(std::string filename, std::shared_ptr<frame_buffer_t> frame_buffer) {

  std::vector<uint8_t> pixels;
  frame_buffer->get_pixels(0, pixels);

  int tex_error = lodepng::encode(filename, pixels, frame_buffer->get_width(), frame_buffer->get_height());
  if (tex_error) {
    std::cout << "unable to save image file " << filename << std::endl;
  }
  std::cout << "image " << filename << " saved" << std::endl;

}


//comparison function to sort the rectangles by height 
bool sort_by_height (rectangle i, rectangle j){
  bool i_smaller_j = false;
  float height_of_i = i.max_.y-i.min_.y;
  float height_of_j = j.max_.y-j.min_.y;
  if (height_of_i > height_of_j) {
    i_smaller_j = true;
  }
  return i_smaller_j;
}

rectangle pack(std::vector<rectangle>& input) {

  //make sure all rectangles stand on the shorter side
  for(uint32_t i=0; i< input.size(); i++){
    auto& rect=input[i];
    if ((rect.max_.x-rect.min_.x) > (rect.max_.y - rect.min_.y)){
      float temp = rect.max_.y;
      rect.max_.y=rect.max_.x;
      rect.max_.x=temp;
      rect.flipped_ = true;
    }
  }

  //sort by height
  std::sort (input.begin(),input.end(),sort_by_height);

  //calc the size of the texture
  float dim = sqrtf(input.size());
  std::cout << dim << " -> " << std::ceil(dim) << std::endl;
  dim = std::ceil(dim);


  //get the largest rect
  std::cout << input[0].max_.y-input[0].min_.y << std::endl;
  float max_height = input[0].max_.y-input[0].min_.y;

  //compute the average height of all rects
  float sum = 0.f;
  for (uint32_t i=0; i<input.size(); i++){
    sum+= input[i].max_.y-input[i].min_.y; 
  }
  float average_height = sum/((float)input.size());

  //heuristically take half
  dim *= 0.9f;
  

  rectangle texture{scm::math::vec2f(0,0), scm::math::vec2f((int)((dim+1)*average_height),(int)((dim+1)*average_height)),0};

  std::cout << "texture size: " << texture.max_.x << " " << texture.max_.y<< std::endl;

  

  //pack the rects
  int offset_x =0;
  int offset_y =0;
  float highest_of_current_line = input[0].max_.y-input[0].min_.y;
  for(int i=0; i< input.size(); i++){
    auto& rect = input[i];
    if ((offset_x+ (rect.max_.x - rect.min_.x)) > texture.max_.x){

      offset_y += highest_of_current_line;
      offset_x =0;
      highest_of_current_line = rect.max_.y - rect.min_.y;
      
    }
    
    rect.max_.x= offset_x + (rect.max_.x - rect.min_.x);
    rect.min_.x= offset_x;
    offset_x+= rect.max_.x - rect.min_.x;


  rect.max_.y = offset_y + (rect.max_.y - rect.min_.y);
  rect.min_.y = offset_y;
  


  }

  //done

  //print the result
  for (int i=0; i< input.size(); i++){
    auto& rect = input[i];
    std::cout<< "rectangle["<< rect.id_<< "]"<<"  min("<< rect.min_.x<<" ,"<< rect.min_.y<<")"<<std::endl;
    std::cout<< "rectangle["<< rect.id_<< "]"<<"  max("<< rect.max_.x<< " ,"<<rect.max_.y<<")"<<std::endl;
  }

  //output test image for rectangle packing
  std::vector<unsigned char> image;
  image.resize(texture.max_.x*texture.max_.y*4);
  for (int i = 0; i < image.size()/4; ++i) {
    image[i*4+0] = 255;
    image[i*4+1] = 0;
    image[i*4+2] = 0;
    image[i*4+3] = 255;
  }
  for (int i=0; i< input.size(); i++){
    auto& rect = input[i];
    int color = (255/input.size())*rect.id_;
    for (int x = rect.min_.x; x < rect.max_.x; x++) {
      for (int y = rect.min_.y; y < rect.max_.y; ++y) {
        int pixel = (x + texture.max_.x*y) * 4;
           image[pixel] = (char)color;
           image[pixel+1] = (char)color;
           image[pixel+2] = (char)color;
           image[pixel+3] = (char)255;
      }
    }
  }
  save_image("data/mesh_prepro_texture.png", image, texture.max_.x, texture.max_.y);

  return texture;
}


// void write_projection_info_file(std::vector<chart>& charts, std::string outfile_name){

//   std::ofstream ofs( outfile_name, std::ios::binary);

//   //write number of charts
//   uint32_t num_charts = charts.size();
//   ofs.write((char*) &num_charts, sizeof(num_charts));


//   for (auto& chart : charts)
//   {
//     ofs.write((char*) &(chart.projection), sizeof(projection_info));
//   }

//   ofs.close();

//   std::cout << "Wrote projection file to " << outfile_name << std::endl;
// }


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

    std::string error_message = "compile shader failure in " + std::string(type) + " shader:\n" + std::string(log);
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
    varying vec2 passed_uv;\n\
    \n\
    void main() {\n\
      vec2 coord = vec2(vertex_new_coord.x, vertex_new_coord.y);\n\
      gl_Position = vec4((coord-0.5)*2.0, 0.5, 1.0);\n\
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

//on every 16 ms
static void glut_timer(int32_t _e) {
  glutPostRedisplay();
  glutTimerFunc(16, glut_timer, 1);
  elapsed_ms_ += 16;
}



void glut_display() {

  frame_buffer_->enable();


  //set the viewport, background color, and reset default framebuffer
  glViewport(0, 0, (GLsizei)window_width_, (GLsizei)window_height_);
  glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  //use the shader program we created
  glUseProgram(shader_program_);

  //bind the VBO of the model such that the next draw call will render with these vertices
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);


  //define the layout of the vertex buffer:
  //setup 2 attributes per vertex (2x texture coord)
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(blit_vertex), (void*)0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(blit_vertex), (void*)(2*sizeof(float)));

  //get texture location
  int slot = 0;
  glUniform1i(glGetUniformLocation(shader_program_, "image"), slot);
  glActiveTexture(GL_TEXTURE0 + slot);
  texture_->enable(slot);

  //draw triangles from the currently bound buffer
  glDrawArrays(GL_TRIANGLES, 0, num_vertices_);

  //unbind, unuse
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glUseProgram(0);

  texture_->disable();

  frame_buffer_->disable();

  //frame_buffer_->draw(0);

  save_image("data/mesh_prepro_result.png", frame_buffer_);

  exit(1);


  glutSwapBuffers();
}

//returns a city block distance between 2 texture coordinates, given a width and height of the texture
int calc_city_block_length(scm::math::vec2f coord1, scm::math::vec2f coord2, int tex_width, int tex_height){

  scm::math::vec2i real_coord1(std::floor(coord1.x * tex_width), std::floor(coord1.y * tex_height));
  scm::math::vec2i real_coord2(std::floor(coord2.x * tex_width), std::floor(coord2.y * tex_height));

  return std::abs(real_coord1.x - real_coord2.x) + std::abs(real_coord1.y - real_coord2.y); 
}

//calculates a per-chart ratio of 3D space to texture space
//ratio is number of pixels for 1 unit in real space
enum PixelResolutionCalculationType { USE_OLD_COORDS, USE_NEW_COORDS};
void calculate_chart_tex_space_sizes(PixelResolutionCalculationType type, std::vector<chart>& charts, std::vector<triangle>& triangles, int tex_width, int tex_height){



  if (type == USE_OLD_COORDS){
    std::cout << "Calculating pixel sizes with old coords" << std::endl;
  }
  else {
    std::cout << "Calculating pixel sizes with new coords" << std::endl;
  }

  //for all charts
  for(auto& chart : charts){

    double real_tex_ratio = 0.0;

    //for all triangles
    for (auto& tri_id : chart.all_triangle_ids_)
    {
      auto& tri = triangles[tri_id];
      double real_length = scm::math::length(tri.v0_.pos_ - tri.v1_.pos_);

      //calculate lengths as cityblock distances 
      double tex_length;
      if (type == USE_OLD_COORDS)
      {
        tex_length = calc_city_block_length(tri.v0_.old_coord_, tri.v1_.old_coord_, tex_width, tex_height);
      }
      else {
        tex_length = calc_city_block_length(tri.v0_.new_coord_, tri.v1_.new_coord_, tex_width, tex_height);
      }
      real_tex_ratio += (tex_length / real_length);
    }

    if (type == USE_OLD_COORDS)
    {
      chart.real_to_tex_ratio_old = real_tex_ratio / chart.all_triangle_ids_.size();
    }
    else {
      chart.real_to_tex_ratio_new = real_tex_ratio / chart.all_triangle_ids_.size();
    }
  }

}


int main(int argc, char *argv[]) {
    

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(window_width_, window_height_);
    glutInitWindowPosition(64, 64);
    glutCreateWindow(argv[0]);
    glutSetWindowTitle("texture renderer");
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
    glewExperimental = GL_TRUE;
    glewInit();


    if (argc == 1 || 
      cmd_option_exists(argv, argv+argc, "-h") ||
      !cmd_option_exists(argv, argv+argc, "-f")) {
      
      std::cout << "Usage: " << argv[0] << "<flags> -f <input_file>" << std::endl <<
         "INFO: bvh_leaf_extractor " << std::endl <<
         "\t-f: selects .bvh input file" << std::endl <<
         "\t-c: selects .lodchart input file" << std::endl <<
         "\t-t: selects .png texture input file" << std::endl <<
         std::endl;
      return 0;
    }

    std::string bvh_filename = std::string(get_cmd_option(argv, argv + argc, "-f"));
    std::string chart_lod_filename = std::string(get_cmd_option(argv, argv + argc, "-c"));
    std::string texture_filename = std::string(get_cmd_option(argv, argv + argc, "-t"));

    std::string lod_filename = bvh_filename.substr(0, bvh_filename.size()-3) + "lod";

    std::vector<int> chart_id_per_triangle;
    int num_charts = load_chart_file(chart_lod_filename, chart_id_per_triangle);
    std::cout << "lodchart file lodaded." << std::endl;

    std::shared_ptr<lamure::ren::bvh> bvh = std::make_shared<lamure::ren::bvh>(bvh_filename);
    std::cout << "bvh file loaded." << std::endl;

    std::shared_ptr<lamure::ren::lod_stream> lod = std::make_shared<lamure::ren::lod_stream>();
    lod->open(lod_filename);
    std::cout << "lod file loaded." << std::endl;

    //load the texture png file
    texture_ = load_image(texture_filename);

    
    std::vector<chart> charts;
    //initially, lets compile the charts

    for (int chart_id = 0; chart_id < num_charts; ++chart_id) {
      rectangle rect{
        scm::math::vec2f(std::numeric_limits<float>::max()),
        scm::math::vec2f(std::numeric_limits<float>::lowest()),
        chart_id, 
        false};
      lamure::bounding_box box(
        scm::math::vec3d(std::numeric_limits<float>::max()),
        scm::math::vec3d(std::numeric_limits<float>::lowest()));
      
      charts.push_back(chart{chart_id, rect, box, std::set<int>()});
    }


    //compute the bounding box of all triangles in a chart
    size_t vertices_per_node = bvh->get_primitives_per_node();
    size_t size_of_vertex = bvh->get_size_of_primitive();
    
    std::vector<lamure::ren::dataset::serialized_vertex> vertices;
    vertices.resize(vertices_per_node);

    std::cout << "lod file read with " << bvh->get_num_nodes() << " nodes\n";
    std::cout << vertices_per_node <<  " vertices per node\n";
    std::cout << "vertex size = " << size_of_vertex << std::endl;


    //for each node in lod
    for (uint32_t node_id = 0; node_id < bvh->get_num_nodes(); node_id++) {
      
      lod->read((char*)&vertices[0], (vertices_per_node*node_id*size_of_vertex),
        (vertices_per_node * size_of_vertex));

      //for every vertex
      for (int vertex_id = 0; vertex_id < vertices_per_node; ++vertex_id) {

        int chart_id = chart_id_per_triangle[node_id*(vertices_per_node/3)+vertex_id/3];
        auto& chart = charts[chart_id];

        const auto& vertex = vertices[vertex_id];
        scm::math::vec3d v(vertex.v_x_, vertex.v_y_, vertex.v_z_);

        scm::math::vec3d min_vertex = chart.box_.min();
        scm::math::vec3d max_vertex = chart.box_.max();


        min_vertex.x = std::min(min_vertex.x, v.x);
        min_vertex.y = std::min(min_vertex.y, v.y);
        min_vertex.z = std::min(min_vertex.z, v.z);

        max_vertex.x = std::max(max_vertex.x, v.x);
        max_vertex.y = std::max(max_vertex.y, v.y);
        max_vertex.z = std::max(max_vertex.z, v.z);

        chart.box_ = lamure::bounding_box(min_vertex, max_vertex);

      }
    }

    //compare all triangles with chart bounding boxes
    uint32_t first_leaf = bvh->get_first_node_id_of_depth(bvh->get_depth());
    uint32_t num_leafs = bvh->get_length_of_depth(bvh->get_depth());

    for (int chart_id = 0; chart_id < num_charts; ++chart_id) {

      //intersect all bvh leaf nodes with chart bounding boxes
      auto& chart = charts[chart_id];

      for (int leaf_id = first_leaf; leaf_id < first_leaf+num_leafs; ++leaf_id) {
        lamure::bounding_box leaf_box(
          scm::math::vec3d(bvh->get_bounding_boxes()[leaf_id].min_vertex()),
          scm::math::vec3d(bvh->get_bounding_boxes()[leaf_id].max_vertex()));

        //disabled - adding any triangles that intersect chart projection
        // //broad check that the leaf and chart intersect before checking individual triangles
        // if (chart.box_.intersects(leaf_box)) {

        //   lod->read((char*)&vertices[0], (vertices_per_node*leaf_id*size_of_vertex),
        //     (vertices_per_node * size_of_vertex));

        //   for (int vertex_id = 0; vertex_id < vertices_per_node; ++vertex_id) {
        //     const auto& vertex = vertices[vertex_id];
        //     scm::math::vec3d v(vertex.v_x_, vertex.v_y_, vertex.v_z_);

        //     //find out if this triangle intersects the chart box
        //     if (chart.box_.contains(v)) {
        //       int lod_tri_id = (leaf_id-first_leaf)*(vertices_per_node/3)+(vertex_id/3);
        //       // chart.all_triangle_ids_.insert(lod_tri_id);

        //       //is it ok to add the same triangle multiple times?
        //     }
        //   }
        // }

        //get tri id and then chart id from lodchart list
        //if chart is this chart, add to appropriate tri list
        for (int tri_id = 0; tri_id < vertices_per_node/3; ++tri_id) {
          
          if (chart_id == chart_id_per_triangle[leaf_id*(vertices_per_node/3)+tri_id]) {
            int lod_tri_id = (leaf_id-first_leaf)*(vertices_per_node/3)+tri_id;
            chart.all_triangle_ids_.insert(lod_tri_id);
          }
        }
      }
      
    }




    //report chart parameters
    for (int chart_id = 0; chart_id < num_charts; ++chart_id) {
      auto& chart = charts[chart_id];
      std::cout << "chart id " << chart_id << std::endl;
      std::cout << "box min " << chart.box_.min() << std::endl;
      std::cout << "box max " << chart.box_.max() << std::endl;
      std::cout << "original triangles " << chart.original_triangle_ids_.size() << std::endl;
      std::cout << "all triangles " << chart.all_triangle_ids_.size() << std::endl;
    }

    std::cout << "tris per node " << vertices_per_node/3 << std::endl;


    //for each leaf triangle in lod, build a triangle struct from serialised vertices
    std::vector<triangle> triangles;
    for (uint32_t leaf_id = first_leaf; leaf_id < first_leaf+num_leafs; ++leaf_id) {

      lod->read((char*)&vertices[0], (vertices_per_node*leaf_id*size_of_vertex),
        (vertices_per_node * size_of_vertex));

      for (int tri_id = 0; tri_id < vertices_per_node/3; ++tri_id) {

        triangle tri;
        tri.v0_.pos_ = scm::math::vec3f(vertices[tri_id*3].v_x_, vertices[tri_id*3].v_y_, vertices[tri_id*3].v_z_);
        tri.v0_.nml_ = scm::math::vec3f(vertices[tri_id*3].n_x_, vertices[tri_id*3].n_y_, vertices[tri_id*3].n_z_);
        tri.v0_.old_coord_ = scm::math::vec2f(vertices[tri_id*3].c_x_, vertices[tri_id*3].c_y_);

        tri.v1_.pos_ = scm::math::vec3f(vertices[tri_id*3+1].v_x_, vertices[tri_id*3+1].v_y_, vertices[tri_id*3+1].v_z_);
        tri.v1_.nml_ = scm::math::vec3f(vertices[tri_id*3+1].n_x_, vertices[tri_id*3+1].n_y_, vertices[tri_id*3+1].n_z_);
        tri.v1_.old_coord_ = scm::math::vec2f(vertices[tri_id*3+1].c_x_, vertices[tri_id*3+1].c_y_);

        tri.v2_.pos_ = scm::math::vec3f(vertices[tri_id*3+2].v_x_, vertices[tri_id*3+2].v_y_, vertices[tri_id*3+2].v_z_);
        tri.v2_.nml_ = scm::math::vec3f(vertices[tri_id*3+2].n_x_, vertices[tri_id*3+2].n_y_, vertices[tri_id*3+2].n_z_);
        tri.v2_.old_coord_ = scm::math::vec2f(vertices[tri_id*3+2].c_x_, vertices[tri_id*3+2].c_y_);

        triangles.push_back(tri);
      }
    }

    std::cout << "Created list of " << triangles.size() << " leaf level triangles\n";


    //for each chart, calculate relative size of real space to original tex space
    calculate_chart_tex_space_sizes(USE_OLD_COORDS, charts, triangles, texture_->get_width(), texture_->get_height());

    project(charts, triangles);



    std::cout << "running rectangle packing" << std::endl;

    const float scale = 400.f;

    //scale the rectangles
    std::vector<rectangle> rects;
    for (int chart_id = 0; chart_id < charts.size(); ++chart_id) {
      chart& chart = charts[chart_id];
      rectangle rect = chart.rect_;
      rect.max_.x *= scale;
      rect.max_.y *= scale;
      rects.push_back(rect);
    }

    //rectangle packing
    rectangle image = pack(rects);

    //apply rectangles
    for (const auto& rect : rects) {
      charts[rect.id_].rect_ = rect;

      charts[rect.id_].projection.tex_space_rect = rect;//save for rendering from texture later on
    }





  //apply new coordinates and pack tris

  //pack tris
  std::vector<blit_vertex> to_upload;

  for (int chart_id = 0; chart_id < charts.size(); ++chart_id) {
    chart& chart = charts[chart_id];
    rectangle& rect = chart.rect_;

    for (auto tri_id : chart.all_triangle_ids_) {


      // std::cout << "before v0: " << triangles[tri_id].v0_.new_coord_.x 
      //  << " " << triangles[tri_id].v0_.new_coord_.y << std::endl;

      // std::cout << "before v1: " << triangles[tri_id].v1_.new_coord_.x 
      //  << " " << triangles[tri_id].v1_.new_coord_.y << std::endl;

      //  std::cout << "before v2: " << triangles[tri_id].v2_.new_coord_.x 
      //  << " " << triangles[tri_id].v2_.new_coord_.y << std::endl;


       if (rect.flipped_) {
         float temp = triangles[tri_id].v0_.new_coord_.x;
         triangles[tri_id].v0_.new_coord_.x = triangles[tri_id].v0_.new_coord_.y;
         triangles[tri_id].v0_.new_coord_.y = temp;

         temp = triangles[tri_id].v1_.new_coord_.x;
         triangles[tri_id].v1_.new_coord_.x = triangles[tri_id].v1_.new_coord_.y;
         triangles[tri_id].v1_.new_coord_.y = temp;

         temp = triangles[tri_id].v2_.new_coord_.x;
         triangles[tri_id].v2_.new_coord_.x = triangles[tri_id].v2_.new_coord_.y;
         triangles[tri_id].v2_.new_coord_.y = temp;
       }


      triangles[tri_id].v0_.new_coord_.x *= scale;
      triangles[tri_id].v0_.new_coord_.y *= scale;
      triangles[tri_id].v1_.new_coord_.x *= scale;
      triangles[tri_id].v1_.new_coord_.y *= scale;
      triangles[tri_id].v2_.new_coord_.x *= scale;
      triangles[tri_id].v2_.new_coord_.y *= scale;

      // std::cout << "min to add x " << rect.min_.x << std::endl;
      // std::cout << "min to add y " << rect.min_.y << std::endl;
      // std::cout << "image size " << image.max_.x << std::endl;
 
      triangles[tri_id].v0_.new_coord_.x += rect.min_.x;
      triangles[tri_id].v0_.new_coord_.y += rect.min_.y;
      triangles[tri_id].v1_.new_coord_.x += rect.min_.x;
      triangles[tri_id].v1_.new_coord_.y += rect.min_.y;
      triangles[tri_id].v2_.new_coord_.x += rect.min_.x;
      triangles[tri_id].v2_.new_coord_.y += rect.min_.y;

      triangles[tri_id].v0_.new_coord_.x /= image.max_.x;
      triangles[tri_id].v0_.new_coord_.y /= image.max_.x;
      triangles[tri_id].v1_.new_coord_.x /= image.max_.x;
      triangles[tri_id].v1_.new_coord_.y /= image.max_.x;
      triangles[tri_id].v2_.new_coord_.x /= image.max_.x;
      triangles[tri_id].v2_.new_coord_.y /= image.max_.x;

      // std::cout << "v0: " << triangles[tri_id].v0_.new_coord_.x 
      //  << " " << triangles[tri_id].v0_.new_coord_.y << std::endl;

      //  std::cout << "v1: " << triangles[tri_id].v1_.new_coord_.x 
      //  << " " << triangles[tri_id].v1_.new_coord_.y << std::endl;

      //  std::cout << "v2: " << triangles[tri_id].v2_.new_coord_.x 
      //  << " " << triangles[tri_id].v2_.new_coord_.y << std::endl;

       to_upload.push_back(blit_vertex{triangles[tri_id].v0_.old_coord_, triangles[tri_id].v0_.new_coord_});
       to_upload.push_back(blit_vertex{triangles[tri_id].v1_.old_coord_, triangles[tri_id].v1_.new_coord_});
       to_upload.push_back(blit_vertex{triangles[tri_id].v2_.old_coord_, triangles[tri_id].v2_.new_coord_});
    } 
  }


  //for each chart, calculate relative size of real space to new tex space
  calculate_chart_tex_space_sizes(USE_NEW_COORDS, charts, triangles, texture_->get_width(), texture_->get_height());


  for (auto& chart : charts)
  {
    std::cout << "Chart " << chart.id_ << ": old ratio " << chart.real_to_tex_ratio_old << std::endl;
    std::cout << "-------- " << ": new ratio " << chart.real_to_tex_ratio_new << std::endl;
  }



    //replacing texture coordinates in LOD file
  std::cout << "replacing tex coords for inner nodes..." << std::endl;
  std::string out_lod_filename = bvh_filename.substr(0, bvh_filename.size()-4) + "_uv.lod";
  std::shared_ptr<lamure::ren::lod_stream> lod_out = std::make_shared<lamure::ren::lod_stream>();
  lod_out->open_for_writing(out_lod_filename);
 
  for (uint32_t node_id = 0; node_id < bvh->get_num_nodes(); ++node_id) { //loops only inner nodes

    //load the node
    lod->read((char*)&vertices[0], (vertices_per_node*node_id*size_of_vertex),
      (vertices_per_node * size_of_vertex));

    //if leaf node, just replace tex coord from corresponding triangle
    if (node_id >= first_leaf && node_id < (first_leaf+num_leafs) ){

      for (int vertex_id = 0; vertex_id < vertices_per_node; ++vertex_id) {
        const int tri_id = ((node_id - first_leaf)*(vertices_per_node/3))+(vertex_id/3);

        switch (vertex_id % 3) {
          case 0:
          {
            vertices[vertex_id].c_x_ = triangles[tri_id].v0_.new_coord_.x;
            vertices[vertex_id].c_y_ = 1.0 - triangles[tri_id].v0_.new_coord_.y;
            break;
          }
          case 1:
          {
            vertices[vertex_id].c_x_ = triangles[tri_id].v1_.new_coord_.x;
            vertices[vertex_id].c_y_ = 1.0 -  triangles[tri_id].v1_.new_coord_.y;
            break;
          }
          case 2:
          {
            vertices[vertex_id].c_x_ = triangles[tri_id].v2_.new_coord_.x;
            vertices[vertex_id].c_y_ = 1.0 - triangles[tri_id].v2_.new_coord_.y;
            break;
          }
          default:
          break;
        }

      }
    }

    // if inner node, project vertices onto plane to get tex coords 
    else
    {
      //for each vertex
      for (int vertex_id = 0; vertex_id < vertices_per_node; ++vertex_id) {

        auto& vertex = vertices[vertex_id];
        const int lod_tri_id = (node_id)*(vertices_per_node/3)+(vertex_id/3);
        const int chart_id = chart_id_per_triangle[lod_tri_id];
        //access projection info for the relevant chart

        if (chart_id != -1){

          auto& proj_info = charts[chart_id].projection;
          rectangle& rect = charts[chart_id].rect_;

           //at this point we will need to project all triangles of inner nodes to their respective charts using the corresponding chart plane
          scm::math::vec3f original_v (vertex.v_x_, vertex.v_y_, vertex.v_z_);
          scm::math::vec2f projected_v = project_to_plane(original_v, proj_info.proj_normal, proj_info.proj_centroid, proj_info.proj_world_up);
          
          //correct by offset (so that min uv coord = 0) 
          projected_v -= proj_info.tex_coord_offset;
          //apply normalisation factor
          projected_v /= proj_info.largest_max;
          //flip if needed
          if (rect.flipped_)
          {
            float temp = projected_v.x;
            projected_v.x = projected_v.y;
            projected_v.y = temp;
          }
          //scale
          projected_v *= scale;
          //offset position in texture
          projected_v += rect.min_;
          //scale down to normalised image space
          projected_v /= image.max_;
          //flip y coord
          projected_v.y = 1.0 - projected_v.y; 

          //replace existing coords in vertex array
          vertex.c_x_ = projected_v.x;
          vertex.c_y_ = projected_v.y;  

        }
      }
    }
    //afterwards, write the node to new file
    lod_out->write((char*)&vertices[0], (vertices_per_node*node_id*size_of_vertex),
      (vertices_per_node * size_of_vertex));
  }

  lod_out->close();
  lod_out.reset();
  lod->close();
  lod.reset();

  std::cout << "OUTPUT updated lod file: " << out_lod_filename << std::endl;

  //write output bvh
  std::string out_bvh_filename = bvh_filename.substr(0, bvh_filename.size()-4) + "_uv.bvh";
  bvh->write_bvh_file(out_bvh_filename);
  bvh.reset();
  std::cout << "OUTPUT updated bvh file: " << out_bvh_filename << std::endl;


  frame_buffer_ = std::make_shared<frame_buffer_t>(1, window_width_, window_height_, GL_RGBA, GL_LINEAR);

  //fill the vertex buffer
  num_vertices_ = to_upload.size();

  //create a vertex buffer and populate it with our data
  glGenBuffers(1, &vertex_buffer_);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
  glBufferData(GL_ARRAY_BUFFER, num_vertices_*sizeof(blit_vertex), &to_upload[0], GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  //create shaders
  make_shader_program();

  //register glut-callbacks
  //the arguments are pointers to the functions we defined above
  glutDisplayFunc(glut_display);
  
  glutShowWindow();

  //set a timeout of 16 ms until glut_timer is triggered
  glutTimerFunc(16, glut_timer, 1);

  //start the main loop (which mainly calls the glutDisplayFunc)
  glutMainLoop();

    return 0;
}



