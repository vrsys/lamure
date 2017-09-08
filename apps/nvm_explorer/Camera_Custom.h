#ifndef LAMURE_CAMERA_CUSTOM_H
#define LAMURE_CAMERA_CUSTOM_H

#include "Frustum.h"
#include <FreeImagePlus.h>
#include <lamure/pro/data/entities/Camera.h>
#include <lamure/pro/data/entities/SparsePoint.h>
#include <scm/core/math.h>
#include <scm/core/math/quat.h>
#include <scm/core/math/vec3.h>

class Camera_Custom : public prov::Camera
{
  public:
    Camera_Custom(prov::Camera camera);

    void init(scm::shared_ptr<scm::gl::render_device> device, std::vector<prov::SparsePoint> &vector_point);
    void update_scale_frustum(float offset);
    void bind_texture(scm::shared_ptr<scm::gl::render_context> context);
    void update_transformation();
    // void calculate_frustum();
    // scm::gl::sampler_state_ptr get_state();
    // scm::gl::texture_2d_ptr get_texture();

    void load_texture(scm::shared_ptr<scm::gl::render_device> device);

    int &get_count_lines();
    scm::math::mat4f &get_transformation();
    scm::math::mat4f &get_transformation_image_plane();
    // const arr<vec3d, 8> &get_frustum_vertices() { return _frustum_vertices; }
    // void set_frustum_vertices(const arr<vec3d, 8> &_frustum_vertices) { this->_frustum_vertices = _frustum_vertices; }
    scm::gl::vertex_array_ptr get_vertex_array_object_lines();
    scm::gl::vertex_array_ptr get_vertex_array_object_pixels();
    std::vector<scm::math::vec3f> &get_vector_pixels_brush();
    Frustum &get_frustum();

    void add_pixel_brush(scm::math::vec3f position, scm::shared_ptr<scm::gl::render_device> device);

  private:
    std::vector<Struct_Line> convert_lines_to_struct_line(std::vector<prov::SparsePoint> &vector_point);
    void update_transformation_image_plane();
    void update_pixels_brush();

    scm::math::mat4f _transformation = scm::math::mat4f::identity();
    scm::math::mat4f _transformation_image_plane = scm::math::mat4f::identity();
    // arr<vec3d, 8> _frustum_vertices;
    Frustum _frustum;

    std::vector<scm::math::vec3f> _vector_pixels_brush;
    scm::gl::vertex_array_ptr _vertex_array_object_lines;
    scm::gl::buffer_ptr _vertex_buffer_object_lines;

    scm::gl::vertex_array_ptr _vertex_array_object_pixels;
    scm::gl::buffer_ptr _vertex_buffer_object_pixels;

    scm::gl::texture_2d_ptr _texture;
    scm::gl::sampler_state_ptr _state;
    int _count_lines = 0;
};

#endif // LAMURE_CAMERA_CUSTOM_H
