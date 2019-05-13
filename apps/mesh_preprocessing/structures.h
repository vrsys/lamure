#include "includes.h"
#include "constants.h"

#ifndef LAMURE_MP_STRUCTURES_H
#define LAMURE_MP_STRUCTURES_H

struct cmd_options
{
    std::string out_filename; // TODO: is never in use
    double cost_threshold;
    uint32_t chart_threshold;
    double e_fit_cf;
    double e_ori_cf;
    double e_shape_cf;
    double cst;
    bool write_charts_as_textures; // TODO: is never in use
    int num_tris_per_node_kdtree;
    int num_tris_per_node_bvh;
    int single_tex_limit;
    int multi_tex_limit;
    bool want_raw_file;
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

#ifdef FLUSH_APP_STATE
namespace boost
{
namespace serialization
{
template <class Archive>
void serialize(Archive& ar, app_state& p, const unsigned int /* version */)
{
    ar& p.all_indexed_triangles;
    ar& p.texture_info_map;

    ar& p.kdtree; // TODO: verify

    ar& p.node_ids;
    ar& p.per_node_chart_id_map;
    // ar& p.per_node_polyhedron; <- no easy way to achieve this

    ar& p.num_areas;
    ar& p.triangles;

    ar& p.bvh; // TODO: verify

    ar& p.chart_map;

    ar& p.image_rect;
    ar& p.area_rects;

    ar& p.t_d;
    ar& p.viewports;

    ar& p.to_upload_per_texture;
    ar& p.area_images;
    // ar& p.handles; <- no point to persist these
    // ar& p.frame_buffers; <- no point to persist these
    // ar& p.textures; <- no point to persist these
}

template <class Archive>
void serialize(Archive& ar, texture_dims& p, const unsigned int /* version */)
{
    ar& p.full_texture_height_;
    ar& p.full_texture_width_;
    ar& p.render_to_texture_height_;
    ar& p.render_to_texture_width_;
}

template <class Archive>
void serialize(Archive& ar, blit_vertex_t& p, const unsigned int /* version */)
{
    ar& p.new_coord_;
    ar& p.old_coord_;
}

template <class Archive>
void serialize(Archive& ar, texture_info& p, const unsigned int /* version */)
{
    ar& p.size_;
    ar& p.filename_;
}

template <class Archive>
void serialize(Archive& ar, scm::math::vec2f& p, const unsigned int /* version */)
{
    ar& p.data_array;
}

template <class Archive>
void serialize(Archive& ar, scm::math::vec3f& p, const unsigned int /* version */)
{
    ar& p.data_array;
}

template <class Archive>
void serialize(Archive& ar, scm::math::vec3d& p, const unsigned int /* version */)
{
    ar& p.data_array;
}

template <class Archive>
void serialize(Archive& ar, scm::math::vec2i& p, const unsigned int /* version */)
{
    ar& p.data_array;
}

template <class Archive>
void serialize(Archive& ar, scm::math::vec2ui& p, const unsigned int /* version */)
{
    ar& p.data_array;
}

template <class Archive>
void serialize(Archive& ar, rectangle& p, const unsigned int /* version */)
{
    ar& p.flipped_;
    ar& p.id_;
    ar& p.max_;
    ar& p.min_;
}

template <class Archive>
void serialize(Archive& ar, chart& p, const unsigned int /* version */)
{
    ar& p.id_;
    ar& p.rect_;
    ar& p.box_;
    ar& p.all_triangle_ids_;
    ar& p.original_triangle_ids_;
    ar& p.all_triangle_new_coods_;
    ar& p.projection;
    ar& p.real_to_tex_ratio_old;
    ar& p.real_to_tex_ratio_new;
}

template <class Archive>
void serialize(Archive& ar, indexed_triangle_t& p, const unsigned int /* version */)
{
    ar& p.v0_;
    ar& p.v1_;
    ar& p.v2_;
    ar& p.tex_idx_;
    ar& p.tri_id_;
}

template <class Archive>
void serialize(Archive& ar, projection_info& p, const unsigned int /* version */)
{
    ar& p.largest_max;
    ar& p.proj_centroid;
    ar& p.proj_normal;
    ar& p.proj_world_up;
    ar& p.tex_coord_offset;
    ar& p.tex_space_rect;
}

template <class Archive>
void serialize(Archive& ar, lamure::mesh::vertex& p, const unsigned int /* version */)
{
    ar& p.pos_;
    ar& p.nml_;
    ar& p.tex_;
}

template <class Archive>
void serialize(Archive& ar, lamure::mesh::Triangle_Chartid& p, const unsigned int /* version */)
{
    ar& p.v0_;
    ar& p.v1_;
    ar& p.v2_;
    ar& p.chart_id;
    ar& p.tex_id;
    ar& p.area_id;
    ar& p.tri_id;
}

template <class Archive>
void serialize(Archive& ar, viewport& p, const unsigned int /* version */)
{
    ar& p.normed_dims;
    ar& p.normed_offset;
}

template <class Archive>
void serialize(Archive& ar, kdtree_t::node_t& p, const unsigned int /* version */)
{
    ar& p.node_id_;
    ar& p.min_;
    ar& p.max_;
    ar& p.begin_;
    ar& p.end_;
    ar& p.error_;
}

} // namespace serialization
} // namespace boost
#endif

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
