#include "includes.h"
#include "constants.h"

#ifndef LAMURE_MP_STRUCTURES_H
#define LAMURE_MP_STRUCTURES_H

struct cmd_options
{
    std::string out_filename;
    double cost_threshold;
    uint32_t chart_threshold;
    double e_fit_cf;
    double e_ori_cf;
    double e_shape_cf;
    double cst;
    bool write_charts_as_textures;
    int num_tris_per_node_kdtree;
    int num_tris_per_node_bvh;
    int single_tex_limit;
    int multi_tex_limit;
};

struct texture_dims
{
    uint32_t render_to_texture_width_ = 4096;
    uint32_t render_to_texture_height_ = 4096;

    uint32_t full_texture_width_ = 4096;
    uint32_t full_texture_height_ = 4096;
};

struct viewport
{
    scm::math::vec2f normed_dims;
    scm::math::vec2f normed_offset;
};

struct blit_vertex_t
{
    scm::math::vec2f old_coord_;
    scm::math::vec2f new_coord_;
};

struct GL_handles
{
    GLuint shader_program_;
    GLuint vertex_buffer_;

    GLuint dilation_shader_program_;
    GLuint dilation_vertex_buffer_;
};

struct app_state
{
    // trivial types
    uint32_t num_areas;

    // structures
    texture_dims t_d;
    rectangle image_rect;
    GL_handles handles;

    // vectors
    std::vector<indexed_triangle_t> all_indexed_triangles;
    std::vector<lamure::mesh::Triangle_Chartid> triangles; // TODO: duplicate type?

    std::vector<uint32_t> node_ids;
    std::vector<viewport> viewports;
    std::vector<rectangle> area_rects;
    std::vector<std::vector<uint8_t>> area_images;
    std::vector<std::vector<blit_vertex_t>> to_upload_per_texture;
    std::vector<std::shared_ptr<frame_buffer_t>> frame_buffers;
    std::vector<std::shared_ptr<texture_t>> textures;

    // maps
    std::map<uint32_t, texture_info> texture_info_map;
    std::map<uint32_t, Polyhedron> per_node_polyhedron;

    std::map<uint32_t, std::map<uint32_t, uint32_t>> per_node_chart_id_map;
    std::map<uint32_t, std::map<uint32_t, chart>> chart_map; // TODO: duplicate type?

    // shared_ptr-s
    std::shared_ptr<kdtree_t> kdtree;
    std::shared_ptr<lamure::mesh::bvh> bvh;
};

#ifdef VCG_PARSER
using namespace vcg;

class CVertex;
class CFace;

struct MyTypes : public UsedTypes<Use<CVertex>::AsVertexType, Use<CFace>::AsFaceType>
{
};

class CVertex : public Vertex<MyTypes, vertex::VFAdj, vertex::Coord3f, vertex::Mark, vertex::TexCoord2f, vertex::BitFlags, vertex::Normal3f>
{
};
class CFace : public Face<MyTypes, face::FFAdj, face::VFAdj, face::WedgeTexCoord2f, face::Normal3f, face::VertexRef, face::BitFlags, face::Mark>
{
};
class CMesh : public vcg::tri::TriMesh<vector<CVertex>, vector<CFace>>
{
};
#endif

#endif // LAMURE_MP_STRUCTURES_H
