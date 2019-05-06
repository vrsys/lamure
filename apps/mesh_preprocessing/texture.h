
#ifndef TEXTURE_H_
#define TEXTURE_H_

#include <iostream>

#include <GL/glew.h>
#include <GL/freeglut.h>

class texture_t
{
  public:
    texture_t();
    texture_t(uint32_t width, uint32_t height, GLenum interpolation);
    texture_t(uint32_t width, uint32_t height, GLenum internal_format, GLenum format, GLenum internal_type, GLenum interpolation);
    ~texture_t();

    void set_pixels(GLubyte* data);

    uint32_t get_num_elements() const;
    uint32_t get_element_size() const;
    uint32_t get_size() const;

    void enable(uint32_t slot);
    void disable();

    uint32_t get_width() const;
    uint32_t get_height() const;

    void draw(float _angle = 0.f);

  protected:
    uint32_t width_;
    uint32_t height_;
    uint32_t bytes_per_pixel_;

    uint32_t num_elements_;
    uint32_t element_size_;
    uint32_t num_bytes_;

    GLuint buffer_;
    GLuint texture_;
    GLuint internal_format_;
    GLuint format_;
    GLuint internal_type_;
    GLuint interpolation_;

    void create();

  private:
};

#endif
