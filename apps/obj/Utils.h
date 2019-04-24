#ifndef VT_OBJ_RENDERER_UTILS_H
#define VT_OBJ_RENDERER_UTILS_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

//lamure
#include <lamure/types.h>
#include <lamure/ren/camera.h>
#include <lamure/ren/config.h>
#include <lamure/ren/policy.h>

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
    scm::math::vec3f normal_;
    scm::math::vec2f coords_;
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

    // //parses a face string like "f  2//1  8//1  4//1 " into 3 given arrays
    // static void parse_face_string (std::string face_string, int (&index)[3], int (&coord)[3], int (&normal)[3]){

    //   //split by space into faces
    //   std::vector<std::string> faces;
    //   boost::algorithm::trim(face_string);
    //   boost::algorithm::split(faces, face_string, boost::algorithm::is_any_of(" "), boost::algorithm::token_compress_on);

    //   for (int i = 0; i < 3; ++i)
    //   {
    //     //split by / for indices
    //     std::vector<std::string> inds;
    //     boost::algorithm::split(inds, faces[i], [](char c){return c == '/';}, boost::algorithm::token_compress_off);

    //     for (int j = 0; j < (int)inds.size(); ++j)
    //     {
    //       int idx = 0;
    //       //parse value from string
    //       if (inds[j] != ""){
    //         idx = stoi(inds[j]);
    //       }
    //       if (j == 0){index[i] = idx;}
    //       else if (j == 1){coord[i] = idx;}
    //       else if (j == 2){normal[i] = idx;}
          
    //     }
    //   }
    // }


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


                    // fscanf(file, "%d/%d %d/%d %d/%d\n",
                    //        &index[0], &coord[0],
                    //        &index[1], &coord[1],
                    //        &index[2], &coord[2]);

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