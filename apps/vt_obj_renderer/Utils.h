#ifndef VT_OBJ_RENDERER_UTILS_H
#define VT_OBJ_RENDERER_UTILS_H

//lamure
#include <lamure/types.h>
#include <lamure/ren/camera.h>
#include <lamure/ren/config.h>
#include <lamure/ren/policy.h>

//lamure vt
#include <lamure/vt/VTConfig.h>
#include <lamure/vt/ren/CutDatabase.h>
#include <lamure/vt/ren/CutUpdate.h>

//schism
#include <scm/core.h>
#include <scm/core/io/tools.h>
#include <scm/gl_util/primitives/primitives_fwd.h>
#include <scm/gl_util/primitives/quad.h>
#include <scm/gl_util/font/font_face.h>
#include <scm/gl_util/font/text.h>
#include <scm/gl_util/font/text_renderer.h>
#include <scm/gl_util/data/imaging/texture_loader.h>
#include <scm/gl_util/primitives/wavefront_obj.h>

struct vt_info {
    uint32_t texture_id_;
    uint16_t view_id_;
    uint16_t context_id_;
    uint64_t cut_id_;
    vt::CutUpdate *cut_update_;

    std::vector<scm::gl::texture_2d_ptr> index_texture_hierarchy_;
    scm::gl::texture_2d_ptr physical_texture_;

    scm::math::vec2ui physical_texture_size_;
    scm::math::vec2ui physical_texture_tile_size_;
    size_t size_feedback_;

    int32_t  *feedback_lod_cpu_buffer_;
#ifdef RASTERIZATION_COUNT
    uint32_t *feedback_count_cpu_buffer_;
#endif

    scm::gl::buffer_ptr feedback_index_ib_;
    scm::gl::buffer_ptr feedback_index_vb_;
    scm::gl::vertex_array_ptr feedback_vao_;
    scm::gl::buffer_ptr feedback_inv_index_;

    scm::gl::buffer_ptr feedback_lod_storage_;
#ifdef RASTERIZATION_COUNT
    scm::gl::buffer_ptr feedback_count_storage_;
#endif

    int toggle_visualization_;
    bool enable_hierarchy_;
};

struct resource {
    uint64_t num_primitives_{0};
    scm::gl::buffer_ptr buffer_;
    scm::gl::vertex_array_ptr array_;
};

struct input {
    float trackball_x_ = 0.f;
    float trackball_y_ = 0.f;
    scm::math::vec2i mouse_;
    scm::math::vec2i prev_mouse_;
    lamure::ren::camera::mouse_state mouse_state_;
};

struct vertex {
    scm::math::vec3f position_;
    scm::math::vec2f coords_;
    scm::math::vec3f normal_;
};

struct Utils {
    static char *get_cmd_option(char **begin, char **end, const std::string &option) {
        char **it = std::find(begin, end, option);
        if (it != end && ++it != end) {
            return *it;
        }
        return 0;
    }

    static bool cmd_option_exists(char **begin, char **end, const std::string &option) {
        return std::find(begin, end, option) != end;
    }

    static scm::gl::data_format get_tex_format() {
        switch (vt::VTConfig::get_instance().get_format_texture()) {
            case vt::VTConfig::R8:
                return scm::gl::FORMAT_R_8;
            case vt::VTConfig::RGB8:
                return scm::gl::FORMAT_RGB_8;
            case vt::VTConfig::RGBA8:
            default:
                return scm::gl::FORMAT_RGBA_8;
        }
    }

    //load an .obj file and return all vertices, normals and coords interleaved
    static uint32_t load_obj(const std::string &_file, std::vector<vertex> &_vertices) {
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

            _vertices.resize(nindices.size());

            for (uint32_t i = 0; i < nindices.size(); i++) {
                vertex vertex;

                vertex.position_ = scm::math::vec3f(
                        v[3 * (vindices[i] - 1)], v[3 * (vindices[i] - 1) + 1], v[3 * (vindices[i] - 1) + 2]);

                vertex.normal_ = scm::math::vec3f(
                        n[3 * (nindices[i] - 1)], n[3 * (nindices[i] - 1) + 1], n[3 * (nindices[i] - 1) + 2]);

                vertex.coords_ = scm::math::vec2f(
                        t[2 * (tindices[i] - 1)], t[2 * (tindices[i] - 1) + 1]);

                _vertices[i] = vertex;
            }

            num_tris = _vertices.size() / 3;

        } else {
            std::cout << "failed to open file: " << _file << std::endl;
            exit(1);
        }

        return num_tris;
    }

};

#endif