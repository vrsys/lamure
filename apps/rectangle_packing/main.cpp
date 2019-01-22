
#include <iostream>
#include <memory>
#include <vector>

#include <cmath>
#include <algorithm>
#include <fstream>
#include <sstream>

#include <scm/core/math.h>
#include "lodepng.h"

#include "texture.h"
#include "frame_buffer.h"



struct vertex {
  scm::math::vec3f pos_;
  scm::math::vec3f nml_;
  scm::math::vec2f old_coord_;
  scm::math::vec2f new_coord_;
};

struct triangle_t {
  vertex v0_;
  vertex v1_;
  vertex v2_;
};

struct rectangle{
	scm::math::vec2f min_;
	scm::math::vec2f max_;
	int id_;
  bool flipped_;
};

struct chart_t {
  int id_;
  std::vector<int> triangle_ids_;
  rectangle rect_;
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
  for(int i=0; i< input.size(); i++){
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
  for (int i=0; i<input.size(); i++){
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

  save_image("texture.png", image, texture.max_.x, texture.max_.y);

  return texture;
}


//load an .obj file and return all vertices, normals and coords interleaved
void load_obj(const std::string& _file, std::vector<triangle_t>& triangles) {

    triangles.clear();

    std::vector<float> v;
    std::vector<uint32_t> vindices;
    std::vector<float> n;
    std::vector<uint32_t> nindices;
    std::vector<float> t;
    std::vector<uint32_t> tindices;

    uint32_t num_tris = 0;

    FILE *file = fopen(_file.c_str(), "r");

    if (0 != file) {

        while (true) {
            char line[128];
            int32_t l = fscanf(file, "%s", line);

            if (l == EOF) break;
            if (strcmp(line, "v") == 0) {
                float vx, vy, vz;
                fscanf(file, "%f %f %f\n", &vx, &vy, &vz);
                v.insert(v.end(), {vx, vy, vz});
            } else if (strcmp(line, "vn") == 0) {
                float nx, ny, nz;
                fscanf(file, "%f %f %f\n", &nx, &ny, &nz);
                n.insert(n.end(), {nx, ny, nz});
            } else if (strcmp(line, "vt") == 0) {
                float tx, ty;
                fscanf(file, "%f %f\n", &tx, &ty);
                t.insert(t.end(), {tx, ty});
            } else if (strcmp(line, "f") == 0) {
                std::string vertex1, vertex2, vertex3;
                uint32_t index[3];
                uint32_t coord[3];
                uint32_t normal[3];
                fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n",
                       &index[0], &coord[0], &normal[0],
                       &index[1], &coord[1], &normal[1],
                       &index[2], &coord[2], &normal[2]);

                vindices.insert(vindices.end(), {index[0], index[1], index[2]});
                tindices.insert(tindices.end(), {coord[0], coord[1], coord[2]});
                nindices.insert(nindices.end(), {normal[0], normal[1], normal[2]});
            }
        }

        fclose(file);

        std::cout << "positions: " << vindices.size() << std::endl;
        std::cout << "normals: " << nindices.size() << std::endl;
        std::cout << "coords: " << tindices.size() << std::endl;

        triangles.resize(nindices.size()/3);

        for (uint32_t i = 0; i < nindices.size()/3; i++) {
          triangle_t tri;
          for (uint32_t j = 0; j < 3; ++j) {
            
            scm::math::vec3f position(
                    v[3 * (vindices[3*i+j] - 1)], v[3 * (vindices[3*i+j] - 1) + 1], v[3 * (vindices[3*i+j] - 1) + 2]);

            scm::math::vec3f normal(
                    n[3 * (nindices[3*i+j] - 1)], n[3 * (nindices[3*i+j] - 1) + 1], n[3 * (nindices[3*i+j] - 1) + 2]);

            scm::math::vec2f coord(
                    t[2 * (tindices[3*i+j] - 1)], t[2 * (tindices[3*i+j] - 1) + 1]);

            
            switch (j) {
              case 0:
              tri.v0_.pos_ =  position;
              tri.v0_.nml_ = normal;
              tri.v0_.old_coord_ = coord;
              break;

              case 1:
              tri.v1_.pos_ =  position;
              tri.v1_.nml_ = normal;
              tri.v1_.old_coord_ = coord;
              break;

              case 2:
              tri.v2_.pos_ =  position;
              tri.v2_.nml_ = normal;
              tri.v2_.old_coord_ = coord;
              break;

              default:
              break;
            }
          }
          triangles[i] = tri;
        }

    } else {
        std::cout << "failed to open file: " << _file << std::endl;
        exit(1);
    }

}



scm::math::vec2f project_to_plane(
  const scm::math::vec3f& v, scm::math::vec3f& plane_normal, 
  const scm::math::vec3f& centroid) {

  scm::math::vec3f v_minus_p(
    v.x - centroid.x,
    v.y - centroid.y,
    v.z - centroid.z);

  //determine tangential coordinate frame before projection

  plane_normal = scm::math::normalize(plane_normal);
  scm::math::vec3f world_up(0.f, 1.f, 0.f);
  if (std::abs(scm::math::dot(world_up, plane_normal)) > 0.8f) {
    world_up = scm::math::vec3f(0.f, 0.f, 1.f);
  }

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

      std::cout << chart_id << std::endl;
    }
  }


  file.close();

  return num_charts;
}

void project(std::vector<chart_t>& charts, std::vector<triangle_t>& triangles) {

  //keep a record of the largest chart edge
  float largest_max = 0.f;

  //iterate all charts
  for (int chart_id = 0; chart_id < charts.size(); ++chart_id) {
    chart_t& chart = charts[chart_id];

    scm::math::vec3f avg_normal(0.f, 0.f, 0.f);
    scm::math::vec3f centroid(0.f, 0.f, 0.f);

    // compute average normal and centroid
    for (int i = 0; i < chart.triangle_ids_.size(); ++i) {
      int tri_id = chart.triangle_ids_[i];
      triangle_t& tri = triangles[tri_id];

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

    avg_normal.x /= (float)(chart.triangle_ids_.size()*3);
    avg_normal.y /= (float)(chart.triangle_ids_.size()*3);
    avg_normal.z /= (float)(chart.triangle_ids_.size()*3);

    centroid.x /= (float)(chart.triangle_ids_.size()*3);
    centroid.y /= (float)(chart.triangle_ids_.size()*3);
    centroid.z /= (float)(chart.triangle_ids_.size()*3);

    std::cout << "chart id " << chart_id << ", num tris: " << chart.triangle_ids_.size() << std::endl;
    std::cout << "  avg_n: (" << avg_normal.x << " " << avg_normal.y << " " << avg_normal.z << ")" << std::endl;
    std::cout << "  centr: (" << centroid.x << " " << centroid.y << " " << centroid.z << ")" << std::endl;

    //project all vertices into that plane
    for (int i = 0; i < chart.triangle_ids_.size(); ++i) {
      int tri_id = chart.triangle_ids_[i];
      triangle_t& tri = triangles[tri_id];

      scm::math::vec2f projected_v0 = project_to_plane(tri.v0_.pos_, avg_normal, centroid);
      scm::math::vec2f projected_v1 = project_to_plane(tri.v1_.pos_, avg_normal, centroid);
      scm::math::vec2f projected_v2 = project_to_plane(tri.v2_.pos_, avg_normal, centroid);

      tri.v0_.new_coord_ = projected_v0;
      tri.v1_.new_coord_ = projected_v1;
      tri.v2_.new_coord_ = projected_v2;

      std::cout << "tri_id " << tri_id << " projected v0: (" << projected_v0.x << " " << projected_v0.y << ")" << std::endl;
      std::cout << "tri_id " << tri_id << " projected v1: (" << projected_v1.x << " " << projected_v1.y << ")" << std::endl;
      std::cout << "tri_id " << tri_id << " projected v2: (" << projected_v2.x << " " << projected_v2.y << ")" << std::endl;

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
    for (int i = 0; i < chart.triangle_ids_.size(); ++i) {
      int tri_id = chart.triangle_ids_[i];
      triangle_t& tri = triangles[tri_id];

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

    chart.rect_.max_.x -= chart.rect_.min_.x;
    chart.rect_.max_.y -= chart.rect_.min_.y;

    //update largest chart
    largest_max = std::max(largest_max, chart.rect_.max_.x);
    largest_max = std::max(largest_max, chart.rect_.max_.y);

    // shift projected coordinates to min = 0
    for (int i = 0; i < chart.triangle_ids_.size(); ++i) {
      int tri_id = chart.triangle_ids_[i];
      triangle_t& tri = triangles[tri_id];

      tri.v0_.new_coord_.x -= chart.rect_.min_.x;
      tri.v0_.new_coord_.y -= chart.rect_.min_.y;
      tri.v1_.new_coord_.x -= chart.rect_.min_.x;
      tri.v1_.new_coord_.y -= chart.rect_.min_.y;
      tri.v2_.new_coord_.x -= chart.rect_.min_.x;
      tri.v2_.new_coord_.y -= chart.rect_.min_.y;

      std::cout << "tri_id " << tri_id << " projected v0: (" << tri.v0_.new_coord_.x << " " << tri.v0_.new_coord_.y << ")" << std::endl;
      std::cout << "tri_id " << tri_id << " projected v1: (" << tri.v1_.new_coord_.x << " " << tri.v1_.new_coord_.y << ")" << std::endl;
      std::cout << "tri_id " << tri_id << " projected v2: (" << tri.v2_.new_coord_.x << " " << tri.v2_.new_coord_.y << ")" << std::endl;

    }

    chart.rect_.min_.x = 0.f;
    chart.rect_.min_.y = 0.f;

  }

  // normalize charts but keep relative size
  for (int chart_id = 0; chart_id < charts.size(); ++chart_id) {
    chart_t& chart = charts[chart_id];
    
    // normalize largest_max to 1
    for (int i = 0; i < chart.triangle_ids_.size(); ++i) {
      int tri_id = chart.triangle_ids_[i];
      triangle_t& tri = triangles[tri_id];

      tri.v0_.new_coord_.x /= largest_max;
      tri.v0_.new_coord_.y /= largest_max;
      tri.v1_.new_coord_.x /= largest_max;
      tri.v1_.new_coord_.y /= largest_max;
      tri.v2_.new_coord_.x /= largest_max;
      tri.v2_.new_coord_.y /= largest_max;
    }

    chart.rect_.max_.x /= largest_max;
    chart.rect_.max_.y /= largest_max;

    std::cout << "chart " << chart_id << " rect min: " << chart.rect_.min_.x << ", " << chart.rect_.min_.y << std::endl;
    std::cout << "chart " << chart_id << " rect max: " << chart.rect_.max_.x << ", " << chart.rect_.max_.y << std::endl;

  }
}


void glut_display() {

  frame_buffer_->enable();


  //set the viewport, background color, and reset default framebuffer
  glViewport(0, 0, (GLsizei)window_width_, (GLsizei)window_height_);
  glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
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

  save_image("result.png", frame_buffer_);

  exit(0);

  glutSwapBuffers();
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

    std::string error_message = "compile shader failure in " + std::string(type) + " shader:\n" + std::string(log);
    delete[] log;
    std::cout << error_message << std::endl;
    exit(1);
  }

  return shader;

}

//compile and link the shader programs
void make_shader_program() {

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




//entry point
int main(int argc, char **argv) {


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

  //std::string obj_file = "test_mesh_1.obj";
  //std::string chart_file = "test_mesh_1.chart";

  std::string obj_file = "triceratops_1332.obj";
  std::string chart_file = "triceratops_1332.chart";
  std::string texture_file = "xiaoguai.png";

  //load the obj
  std::vector<triangle_t> triangles;
  load_obj(obj_file, triangles);

  std::cout << "Number of triangles: " << triangles.size() << std::endl;

  //load chart file
  
  std::vector<int> chart_id_per_triangle;
  int num_charts = load_chart_file(chart_file, chart_id_per_triangle);

  std::cout << "Number of chart ids: " << chart_id_per_triangle.size() << std::endl;

  

  std::vector<chart_t> charts;
  //initially, lets compile the charts

  for (int chart_id = 0; chart_id < num_charts; ++chart_id) {
    charts.push_back(chart_t{chart_id, std::vector<int>()});
  }

  //lets assign triangles to these charts
  for (int tri_id = 0; tri_id < triangles.size(); ++tri_id) {
    //infer the chart id for that triangle
    int chart_id = chart_id_per_triangle[tri_id];
    charts[chart_id].triangle_ids_.push_back(tri_id);
  }

  //done

  project(charts, triangles);


  std::cout << "running rectangle packing" << std::endl;

  float scale = 400.f;

  //scale the rectangles
  std::vector<rectangle> rects;
  for (int chart_id = 0; chart_id < charts.size(); ++chart_id) {
    chart_t& chart = charts[chart_id];
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
  }

  //apply new coordinates and pack tris


  //pack tris
  std::vector<blit_vertex> to_upload;

  for (int chart_id = 0; chart_id < charts.size(); ++chart_id) {
    chart_t& chart = charts[chart_id];
    rectangle& rect = chart.rect_;

    for (int i = 0; i < chart.triangle_ids_.size(); ++i) {
      int tri_id = chart.triangle_ids_[i];


      std::cout << "before v0: " << triangles[tri_id].v0_.new_coord_.x 
       << " " << triangles[tri_id].v0_.new_coord_.y << std::endl;

      std::cout << "before v1: " << triangles[tri_id].v1_.new_coord_.x 
       << " " << triangles[tri_id].v1_.new_coord_.y << std::endl;

       std::cout << "before v2: " << triangles[tri_id].v2_.new_coord_.x 
       << " " << triangles[tri_id].v2_.new_coord_.y << std::endl;


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

      std::cout << "min to add x " << rect.min_.x << std::endl;
      std::cout << "min to add y " << rect.min_.y << std::endl;
      std::cout << "image size " << image.max_.x << std::endl;
 
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

      std::cout << "v0: " << triangles[tri_id].v0_.new_coord_.x 
       << " " << triangles[tri_id].v0_.new_coord_.y << std::endl;

       std::cout << "v1: " << triangles[tri_id].v1_.new_coord_.x 
       << " " << triangles[tri_id].v1_.new_coord_.y << std::endl;

       std::cout << "v2: " << triangles[tri_id].v2_.new_coord_.x 
       << " " << triangles[tri_id].v2_.new_coord_.y << std::endl;

       to_upload.push_back(blit_vertex{triangles[tri_id].v0_.old_coord_, triangles[tri_id].v0_.new_coord_});
       to_upload.push_back(blit_vertex{triangles[tri_id].v1_.old_coord_, triangles[tri_id].v1_.new_coord_});
       to_upload.push_back(blit_vertex{triangles[tri_id].v2_.old_coord_, triangles[tri_id].v2_.new_coord_});
    } 
  }

  //load the texture png file
  texture_ = load_image(texture_file);



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

