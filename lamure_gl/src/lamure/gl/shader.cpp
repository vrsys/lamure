
#include <lamure/gl/shader.h>

namespace lamure {
namespace gl {

inline
shader_t::shader_t()
: program_(0),
  linked_(false)
{

}

inline
shader_t::~shader_t()
{
  disable();
  if (linked_) {
    glDeleteProgram(program_);
  }
}

inline void
shader_t::attach(
  GLenum type,
  const std::string& filename)
{
  LAMURE_ASSERT(!linked_, "cannot attach to linked program");
  shaders_.push_back(load(type, filename));
}

inline GLuint
shader_t::get_program() const
{
  return program_;
}

inline void
shader_t::enable()
{
  LAMURE_ASSERT(linked_, "cannot enable non-linked program");
  glUseProgram(program_);
}

inline void
shader_t::disable()
{
  glUseProgram(0);
}

inline void
shader_t::set(
  const std::string& uniform,
  const lamure::math::mat4f_t& matrix)
{
  LAMURE_ASSERT(linked_, "cannot set uniform on non-linked program");
  glUniformMatrix4fv(glGetUniformLocation(program_, uniform.c_str()), 1, GL_FALSE, &matrix.data_[0]);
}

inline void
shader_t::set(
  const std::string& uniform,
  const lamure::math::vec3f_t& vector)
{
  LAMURE_ASSERT(linked_, "cannot set uniform on non-linked program");
  glUniform3fv(glGetUniformLocation(program_, uniform.c_str()), 1, &vector.data_[0]);
}

inline void
shader_t::set(
  const std::string& uniform,
  const int32_t value)
{
  LAMURE_ASSERT(linked_, "cannot set uniform on non-linked program");
  glUniform1i(glGetUniformLocation(program_, uniform.c_str()), value);
}

inline void
shader_t::set(
  const std::string& uniform,
  const float32_t value)
{
  LAMURE_ASSERT(linked_, "cannot set uniform on non-linked program");
  glUniform1f(glGetUniformLocation(program_, uniform.c_str()), value);
}

inline void
shader_t::set(
  const std::string& uniform,
  const uint32_t slot,
  frame_buffer_t* frame_buffer)
{
  LAMURE_ASSERT(linked_, "cannot set uniform on non-linked program");
  glUniform1i(glGetUniformLocation(program_, uniform.c_str()), slot);
  glActiveTexture(GL_TEXTURE0 + slot);
  frame_buffer->bind_texture(0);
}

inline void
shader_t::set(
  const std::string& uniform,
  const uint32_t slot,
  storage_buffer_t* storage_buffer)
{
  LAMURE_ASSERT(linked_, "cannot set uniform on non-linked program");
  GLuint block_idx = glGetProgramResourceIndex(program_, GL_SHADER_STORAGE_BLOCK, uniform.c_str());
  GLuint binding_point_idx = slot;
  glShaderStorageBlockBinding(program_, block_idx, binding_point_idx);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding_point_idx, storage_buffer->get_buffer()); 
}

inline void
shader_t::set(
  const std::string& uniform,
  const uint32_t slot,
  texture_buffer_t* texture_buffer)
{
  LAMURE_ASSERT(linked_, "cannot set uniform on non-linked program");
  glUniform1i(glGetUniformLocation(program_, uniform.c_str()), slot);
  glActiveTexture(GL_TEXTURE0 + slot);
  texture_buffer->bind_texture();
}

inline void
shader_t::set(
  const std::string& uniform,
  const uint32_t slot,
  texture_2d_t* texture)
{
  LAMURE_ASSERT(linked_, "cannot set uniform on non-linked program");
  glUniform1i(glGetUniformLocation(program_, uniform.c_str()), slot);
  glActiveTexture(GL_TEXTURE0 + slot);
  texture->enable(slot, GL_READ_ONLY);
}

inline void
shader_t::set(
  const std::string& uniform,
  const uint32_t slot,
  texture_3d_t* texture)
{
  LAMURE_ASSERT(linked_, "cannot set uniform on non-linked program");
  glUniform1i(glGetUniformLocation(program_, uniform.c_str()), slot);
  glActiveTexture(GL_TEXTURE0 + slot);
  texture->enable(slot, GL_READ_ONLY);
}


inline void
shader_t::link()
{
  if (!linked_) {
    LAMURE_ASSERT(!shaders_.empty(), "cannot link empty shader");

    program_ = glCreateProgram();

    for (auto& shader : shaders_) {
      glAttachShader(program_, shader);
    }

    glLinkProgram(program_);

    GLint status;
    glGetProgramiv(program_, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
      GLint log_length;
      glGetProgramiv(program_, GL_INFO_LOG_LENGTH, &log_length);

      GLchar* log = new GLchar[log_length + 1];
      glGetProgramInfoLog(program_, log_length, NULL, log);

      std::string error_message = "link shader failure:\n" + std::string(log);
      delete[] log;
      LAMURE_ASSERT(false, error_message);
    }

    for (auto& shader : shaders_) {
      glDetachShader(program_, shader);
      glDeleteShader(shader);
    }

    shaders_.clear();
    linked_ = true;
  }
}


inline GLuint
shader_t::load(
  GLenum shader_type,
  const std::string& filename)
{
  std::string source = load_file(filename.c_str());

  source = std::string("#version "+LAMURE_GLSL_VERSION+"\n"+source);
  GLuint shader = glCreateShader(shader_type);
  const char* filedata = source.c_str();

  glShaderSource(shader, 1, &filedata, NULL);

  glCompileShader(shader);

  GLint status;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if (status == GL_FALSE) {
    GLint log_length;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);

    GLchar* log = new GLchar[log_length + 1];
    glGetShaderInfoLog(shader, log_length, NULL, log);

    const char* type = NULL;
    switch (shader_type) {
        case GL_VERTEX_SHADER: type = "vertex"; break;
        case GL_GEOMETRY_SHADER: type = "geometry"; break;
        case GL_FRAGMENT_SHADER: type = "fragment"; break;
        default: break;
    }

    std::string error_message = "compile shader failure in " + std::string(type) + " shader:\n" + std::string(log);
    delete[] log;
    LAMURE_ASSERT(false, error_message);
  }

  return shader;
}

inline std::string
shader_t::load_file(
  const std::string& filename)
{
  LAMURE_LOG_INFO("loading shader " + filename);

  std::ifstream file;
  file.open(filename.c_str());
  LAMURE_ASSERT(file.good(), "unable to open shader file:\n" + filename);

  std::string line;
  std::string source = "";

  int exclude = -1;
  while(!file.eof()) {
    std::getline(file, line);
    //remove leading spaces
    while (line[0] == ' ' || line[0] == '\t') {
        line.erase(0, 1);
    }
    //remove leading comments
    if (line.size() > 0 && line[0] != '\n') {
      for (unsigned int i = 0; i < line.size(); i++) {
        //remove trailing comments
        if (line[i] == '/' && line[i+1] == '/') {
          if (i > 0) line = line.substr(0, i);
          else line = "";
        }
        //remove excludes, :TODO: handle all cases
        else if (line[i] == '/' && line[i+1] == '*') {
          exclude = i;
          line = line.substr(0, i);
          line.append("\n");
        }
        if (line[i] == '*' && line[i+1] == '/') {
          exclude = -1;
          if (line.size() > i+2) line = line.substr(i+2, line.size()-i-2);
          else line = "";
        }
      }
      if (-1 == exclude && line[0] != '\n') source.append(line);
    }
  }
  file.close();

  return source;
}

} // namespace gl
} // namespace lamure
