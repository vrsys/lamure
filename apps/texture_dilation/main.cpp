
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

struct projection_info {
  scm::math::vec3f proj_centroid;
  scm::math::vec3f proj_normal;
  scm::math::vec3f proj_world_up;

  rectangle tex_space_rect;
  scm::math::vec2f tex_coord_offset;
};

struct chart_t {
  int id_;
  std::vector<int> triangle_ids_;
  rectangle rect_;
  projection_info projection;
};

 

uint window_width_ = 1024;
uint window_height_ = 1024;

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


std::vector<unsigned char> img;

void load_image(const std::string& filepath) {

  //unsigned int width = 0;
  //unsigned int height = 0;
  //use texture size as window size
  int tex_error = lodepng::decode(img, window_width_, window_height_, filepath);
  if (tex_error) {
    std::cout << "unable to load image file " << filepath << std::endl;
  }

}

std::shared_ptr<texture_t> copy_image_to_texture(std::vector<unsigned char>& img) {
  auto texture = std::make_shared<texture_t>(window_width_, window_height_, GL_NEAREST);
  texture->set_pixels(&img[0]);

  return texture;
}


void save_image(std::string filename, std::vector<uint8_t> image, int width, int height) {
  int tex_error = lodepng::encode(filename, image, width, height);
  if (tex_error) {
    std::cout << "unable to save image file " << filename << std::endl;
  }

}


void save_image(std::string filename, std::shared_ptr<frame_buffer_t> frame_buffer) {

  std::vector<uint8_t> pixels;
  frame_buffer->get_pixels(0, pixels);

  int tex_error = lodepng::encode(filename, pixels, frame_buffer->get_width(), frame_buffer->get_height());
  if (tex_error) {
    std::cout << "unable to save image file " << filename << std::endl;
  }


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



std::string out_dilated_texture_file = "";

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
  glUniform1i(glGetUniformLocation(shader_program_, "image_width"), window_width_);
  glUniform1i(glGetUniformLocation(shader_program_, "image_height"), window_height_);
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

  save_image(out_dilated_texture_file, frame_buffer_);

  exit(0);
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


  std::string in_undilated_texture_file = "";

  if(argc < 3) {
    std::cout << "usage: " << argv[0] << " " << "<in_file>.png <out_file>.png\n";
    return 0;
  }

  in_undilated_texture_file = argv[1];
  out_dilated_texture_file = argv[2];

  //load the texture png file
  load_image(in_undilated_texture_file);

  std::cout << "Loaded image for dilation.\n";


  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
  glutInitWindowSize(window_width_, window_height_);
  glutInitWindowPosition(64, 64);
  glutCreateWindow(argv[0]);
  glutSetWindowTitle("texture dilater");
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_TEXTURE_2D);
  glDisable(GL_CULL_FACE);
  glewExperimental = GL_TRUE;
  glewInit();

  //std::string obj_file = "test_mesh_1.obj";
  //std::string chart_file = "test_mesh_1.chart";

  // std::string obj_file = "triceratops_1332.obj";
  // std::string chart_file = "triceratops_1332.chart";
  // std::string texture_file = "xiaoguai.png";

  texture_ = copy_image_to_texture(img);


  frame_buffer_ = std::make_shared<frame_buffer_t>(1, window_width_, window_height_, GL_RGBA, GL_NEAREST);




  //fill the vertex buffer
  num_vertices_ = 6;

  // each line: x, y, u, v
  float screen_space_quad_geometry[30] {
    -1.0, -1.0, 0.0, 0.0,
     1.0, -1.0, 1.0, 0.0,
    -1.0,  1.0, 0.0, 1.0,

      1.0, -1.0, 1.0, 0.0,
      1.0,  1.0, 1.0, 1.0,
     -1.0,  1.0, 0.0, 1.0
  };


  //create a vertex buffer and populate it with our data
  glGenBuffers(1, &vertex_buffer_);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
  glBufferData(GL_ARRAY_BUFFER, num_vertices_*4*sizeof(float), &screen_space_quad_geometry[0], GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  //create shaders
  make_shader_program();


  //register glut-callbacks
  //the arguments are pointers to the functions we defined above
  glutDisplayFunc(glut_display);
  
  glutShowWindow();

  //set a timeout of 16 ms until glut_timer is triggered
  glutTimerFunc(0, glut_timer, 1);

  //start the main loop (which mainly calls the glutDisplayFunc)
  glutMainLoop();





  return 0;
}

