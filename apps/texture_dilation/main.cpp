
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


std::string out_dilated_texture_file = "";



GLuint shader_program_; //contains GPU-code
GLuint vertex_buffer_; //contains 3d model
std::shared_ptr<texture_t> texture_; //contains GPU image

std::vector<std::shared_ptr<frame_buffer_t>> frame_buffers_; //contains resulting images
int current_framebuffer_ = 0;
int num_dilations = 255;


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




void glut_display() {

  frame_buffers_[0]->enable();

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

  frame_buffers_[0]->disable();

  for (int i = 0; i < num_dilations; ++i) {

    current_framebuffer_ = (i+1)%2;

    frame_buffers_[current_framebuffer_]->enable();
    int current_texture = 0;
    if (current_framebuffer_ == 0) {
      current_texture = 1;
    }


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
    frame_buffers_[current_texture]->bind_texture(slot);

    //draw triangles from the currently bound buffer
    glDrawArrays(GL_TRIANGLES, 0, num_vertices_);

    //unbind, unuse
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);

    frame_buffers_[current_texture]->unbind_texture(slot);

    frame_buffers_[current_framebuffer_]->disable();
  }



  //frame_buffer_->draw(0);

  save_image(out_dilated_texture_file, frame_buffers_[current_framebuffer_]);

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

  if(argc < 4) {
    std::cout << "usage: " << argv[0] << " " << "<in_file>.png <out_file>.png num_dilations\n";
    return 0;
  }

  in_undilated_texture_file = argv[1];
  out_dilated_texture_file = argv[2];
  num_dilations = atoi(argv[3]);

  std::cout << "Num dilations: " << num_dilations << std::endl;

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


  for (int i = 0; i < 2; ++i) {
    frame_buffers_.push_back(std::make_shared<frame_buffer_t>(1, window_width_, window_height_, GL_RGBA, GL_NEAREST));
  }



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


