// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <boost/assign/list_of.hpp>
#include <queue>
#include <numeric>
#include <unordered_map>

#include <lamure/config.h>
#include "VTRenderer.h"
#include "imgui.h"
#include "imgui_impl_glfw_gl3.h"
#include <lamure/vt/ren/CutDatabase.h>
#include <lamure/vt/ren/CutUpdate.h>

namespace vt
{
VTRenderer::VTRenderer(CutUpdate* cut_update) : _data_res(), _ctxt_res(), _view_res()
{
    this->_cut_update = cut_update;
    this->init();
}

void VTRenderer::init()
{
    _toggle_visualization = 0;
    _enable_hierarchy = true;
}

void VTRenderer::add_data(uint64_t cut_id, uint32_t context_id, uint32_t data_id)
{
    using namespace scm::math;
    using namespace scm::gl;

    _ctxt_res[context_id]->_render_context->apply();

    data_res* res = new data_res();

    uint16_t depth = (uint16_t)((*CutDatabase::get_instance().get_cut_map())[cut_id]->get_atlas()->getDepth());
    uint16_t level = 0;
    while(level < depth)
    {
        uint32_t size_index_texture = (uint32_t)QuadTree::get_tiles_per_row(level);

        auto index_texture_level_ptr = _ctxt_res[context_id]->_device->create_texture_2d(vec2ui(size_index_texture, size_index_texture), FORMAT_RGBA_8UI);

        _ctxt_res[context_id]->_render_context->clear_image_data(index_texture_level_ptr, 0, FORMAT_RGBA_8UI, 0);
        res->_index_hierarchy.emplace_back(index_texture_level_ptr);

        level++;
    }

    _data_res[data_id] = res;

    _ctxt_res[context_id]->_render_context->sync();
}
void VTRenderer::add_view(uint16_t view_id, uint32_t width, uint32_t height, float scale)
{
    view_res* res = new view_res();

    res->_width = width;
    res->_height = height;
    res->_start = std::chrono::high_resolution_clock::now();
    res->_scale = scale;

    _view_res[view_id] = res;
}
void VTRenderer::add_context(uint16_t context_id)
{
    using namespace scm::math;
    using namespace scm::gl;

    ctxt_res* res = new ctxt_res();

    res->_device.reset(new render_device());
    res->_render_context = res->_device->create_context();

    res->_render_context->apply();

    res->_obj.reset(new wavefront_obj_geometry(res->_device, std::string(LAMURE_PRIMITIVES_DIR) + "/seulen_70087x272484.obj"));

    std::string fs_vt_color, vs_vt, vs_vt_feedback;

    {
        using namespace boost::assign;

        if(!scm::io::read_text_file(std::string(LAMURE_SHADERS_DIR) + "/virtual_texturing.glslv", vs_vt) ||
           !scm::io::read_text_file(std::string(LAMURE_SHADERS_DIR) + "/virtual_texturing_hierarchical.glslf", fs_vt_color) ||
           !scm::io::read_text_file(std::string(LAMURE_SHADERS_DIR) + "/virtual_texturing_inv_index_generation.glslv", vs_vt_feedback))
        {
            scm::err() << "error reading shader files" << scm::log::end;
            throw std::runtime_error("Error reading shader files");
        }

        res->_shader_vt_feedback = res->_device->create_program(list_of(res->_device->create_shader(STAGE_VERTEX_SHADER, vs_vt_feedback)));
        res->_shader_vt = res->_device->create_program(list_of(res->_device->create_shader(STAGE_VERTEX_SHADER, vs_vt))(res->_device->create_shader(STAGE_FRAGMENT_SHADER, fs_vt_color)));

        res->_dstate_less = res->_device->create_depth_stencil_state(true, true, COMPARISON_LESS);
        res->_blend_state = res->_device->create_blend_state(true, FUNC_SRC_COLOR, FUNC_ONE_MINUS_SRC_ALPHA, FUNC_SRC_ALPHA, FUNC_ONE_MINUS_SRC_ALPHA);

        res->_filter_nearest = res->_device->create_sampler_state(FILTER_MIN_MAG_NEAREST, WRAP_CLAMP_TO_EDGE);
        res->_filter_linear = res->_device->create_sampler_state(FILTER_MIN_MAG_LINEAR, WRAP_CLAMP_TO_EDGE);

        res->_ms_no_cull = res->_device->create_rasterizer_state(FILL_SOLID, CULL_NONE, ORIENT_CCW, true);
        res->_ms_cull = res->_device->create_rasterizer_state(FILL_SOLID, CULL_BACK, ORIENT_CCW, true);
    }

    if(!res->_shader_vt || !res->_shader_vt_feedback)
    {
        scm::err() << "error creating shader program" << scm::log::end;
        throw std::runtime_error("Error creating shader program");
    }

    res->_physical_texture_dimension = vec2ui(VTConfig::get_instance().get_phys_tex_tile_width(), VTConfig::get_instance().get_phys_tex_tile_width());
    vec2ui physical_texture_size = vec2ui(VTConfig::get_instance().get_phys_tex_px_width(), VTConfig::get_instance().get_phys_tex_px_width());
    res->_physical_texture = res->_device->create_texture_2d(physical_texture_size, get_tex_format(), 1, VTConfig::get_instance().get_phys_tex_layers() + 1);

    res->_size_feedback = VTConfig::get_instance().get_phys_tex_tile_width() * VTConfig::get_instance().get_phys_tex_tile_width() * VTConfig::get_instance().get_phys_tex_layers();

    std::vector<uint32_t> slot_indices(res->_size_feedback);
    std::iota(slot_indices.begin(), slot_indices.end(), 0);

    res->_feedback_index_ib = res->_device->create_buffer(BIND_INDEX_BUFFER, USAGE_STATIC_DRAW, res->_size_feedback * size_of_format(FORMAT_R_32UI), &slot_indices[0]);
    res->_feedback_index_vb = res->_device->create_buffer(BIND_VERTEX_BUFFER, USAGE_DYNAMIC_DRAW, res->_size_feedback * size_of_format(FORMAT_R_32UI), 0);
    res->_feedback_vao = res->_device->create_vertex_array(vertex_format(0, 0, TYPE_UINT, size_of_format(FORMAT_R_32UI)), boost::assign::list_of(res->_feedback_index_vb), res->_shader_vt_feedback);
    res->_feedback_lod_storage = res->_device->create_buffer(BIND_STORAGE_BUFFER, USAGE_STREAM_COPY, res->_size_feedback * size_of_format(FORMAT_R_32I));
#ifdef RASTERIZATION_COUNT
    res->_feedback_count_storage = res->_device->create_buffer(BIND_STORAGE_BUFFER, USAGE_STREAM_COPY, res->_size_feedback * size_of_format(FORMAT_R_32UI));
#endif
    res->_feedback_inv_index = res->_device->create_buffer(BIND_STORAGE_BUFFER, USAGE_DYNAMIC_COPY, res->_size_feedback * size_of_format(FORMAT_R_32UI));

    res->_feedback_lod_cpu_buffer = new int32_t[res->_size_feedback];
#ifdef RASTERIZATION_COUNT
    res->_feedback_count_cpu_buffer = new uint32_t[res->_size_feedback];
#endif

    for(size_t i = 0; i < res->_size_feedback; ++i)
    {
        res->_feedback_lod_cpu_buffer[i] = 0;
#ifdef RASTERIZATION_COUNT
        res->_feedback_count_cpu_buffer[i] = 0;
#endif
    }

    res->_render_context->sync();

    _ctxt_res[context_id] = res;
}
void VTRenderer::update_view(uint16_t view_id, uint32_t width, uint32_t height, float scale, const scm::math::mat4f& view_matrix)
{
    _view_res[view_id]->_width = width;
    _view_res[view_id]->_height = height;
    _view_res[view_id]->_scale = scale;
    _view_res[view_id]->_view_matrix = view_matrix;
}
void VTRenderer::clear_buffers(uint16_t context_id)
{
    using namespace scm::math;
    using namespace scm::gl;

    context_state_objects_guard csg(_ctxt_res[context_id]->_render_context);
    context_texture_units_guard tug(_ctxt_res[context_id]->_render_context);

    _ctxt_res[context_id]->_render_context->set_default_frame_buffer();

    _ctxt_res[context_id]->_render_context->clear_default_color_buffer(FRAMEBUFFER_BACK, vec4f(.2f, .2f, .2f, 1.0f));
    _ctxt_res[context_id]->_render_context->clear_default_depth_stencil_buffer();

    _ctxt_res[context_id]->_render_context->apply();
}
void VTRenderer::render(uint32_t color_data_id, uint16_t view_id, uint16_t context_id)
{
    _ctxt_res[context_id]->_render_context->apply();

    bool feedback_enabled = _cut_update->can_accept_feedback(context_id);

    if(feedback_enabled)
    {
        update_feedback_layout(context_id);
    }

    _ctxt_res[context_id]->_shader_vt->uniform("enable_feedback", feedback_enabled);

    using namespace scm::math;
    using namespace scm::gl;

    uint64_t color_cut_id = (((uint64_t)color_data_id) << 32) | ((uint64_t)view_id << 16) | ((uint64_t)context_id);
    uint32_t max_depth_level_color = (*CutDatabase::get_instance().get_cut_map())[color_cut_id]->get_atlas()->getDepth() - 1;

    mat4f projection_matrix = mat4f::identity();
    perspective_matrix(projection_matrix, 10.f + _view_res[view_id]->_scale * 100.f, float(_view_res[view_id]->_width) / float(_view_res[view_id]->_height), 0.01f, 1000.0f);
    std::chrono::duration<double> elapsed_seconds = std::chrono::high_resolution_clock::now() - _view_res[view_id]->_start;

    mat4f model_matrix = mat4f::identity(); //* make_rotation((float)elapsed_seconds.count(), 0.f, 1.f, 0.f);

    mat4f model_view_matrix = _view_res[view_id]->_view_matrix * model_matrix;
    _ctxt_res[context_id]->_shader_vt->uniform("projection_matrix", projection_matrix);
    _ctxt_res[context_id]->_shader_vt->uniform("model_view_matrix", model_view_matrix);

    _ctxt_res[context_id]->_shader_vt->uniform("physical_texture_dim", _ctxt_res[context_id]->_physical_texture_dimension);
    _ctxt_res[context_id]->_shader_vt->uniform("max_level", max_depth_level_color);
    _ctxt_res[context_id]->_shader_vt->uniform("tile_size", scm::math::vec2((uint32_t)VTConfig::get_instance().get_size_tile()));
    _ctxt_res[context_id]->_shader_vt->uniform("tile_padding", scm::math::vec2((uint32_t)VTConfig::get_instance().get_size_padding()));

    _ctxt_res[context_id]->_shader_vt->uniform("enable_hierarchy", _enable_hierarchy);
    _ctxt_res[context_id]->_shader_vt->uniform("toggle_visualization", _toggle_visualization);

    for(uint32_t i = 0; i < _data_res[color_data_id]->_index_hierarchy.size(); ++i)
    {
        std::string texture_string = "hierarchical_idx_textures";
        _ctxt_res[context_id]->_shader_vt->uniform(texture_string, i, int((i)));
    }

    _ctxt_res[context_id]->_shader_vt->uniform("physical_texture_array", 17);

    context_state_objects_guard csg(_ctxt_res[context_id]->_render_context);
    context_texture_units_guard tug(_ctxt_res[context_id]->_render_context);
    context_framebuffer_guard fbg(_ctxt_res[context_id]->_render_context);

    _ctxt_res[context_id]->_render_context->set_viewport(viewport(vec2ui(0, 0), 1 * vec2ui(_view_res[view_id]->_width, _view_res[view_id]->_height)));

    _ctxt_res[context_id]->_render_context->set_depth_stencil_state(_ctxt_res[context_id]->_dstate_less);
    _ctxt_res[context_id]->_render_context->set_rasterizer_state(_ctxt_res[context_id]->_ms_cull);
    _ctxt_res[context_id]->_render_context->set_blend_state(_ctxt_res[context_id]->_blend_state);

    _ctxt_res[context_id]->_render_context->bind_program(_ctxt_res[context_id]->_shader_vt);

    _ctxt_res[context_id]->_render_context->sync();

    apply_cut_update(context_id);

    for(uint16_t i = 0; i < _data_res[color_data_id]->_index_hierarchy.size(); ++i)
    {
        _ctxt_res[context_id]->_render_context->bind_texture(_data_res[color_data_id]->_index_hierarchy.at(i), _ctxt_res[context_id]->_filter_nearest, i);
    }

    _ctxt_res[context_id]->_render_context->bind_texture(_ctxt_res[context_id]->_physical_texture, _ctxt_res[context_id]->_filter_linear, 17);

    //////////

    _ctxt_res[context_id]->_render_context->bind_storage_buffer(_ctxt_res[context_id]->_feedback_lod_storage, 0);
    _ctxt_res[context_id]->_render_context->bind_storage_buffer(_ctxt_res[context_id]->_feedback_inv_index, 2);
#ifdef RASTERIZATION_COUNT
    _ctxt_res[context_id]->_render_context->bind_storage_buffer(_ctxt_res[context_id]->_feedback_count_storage, 1);
#endif

    _ctxt_res[context_id]->_render_context->apply();

    _ctxt_res[context_id]->_obj->draw(_ctxt_res[context_id]->_render_context, geometry::MODE_SOLID);

    _ctxt_res[context_id]->_render_context->sync();

    if(feedback_enabled)
    {
        collect_feedback(context_id);
    }
}

void VTRenderer::collect_feedback(uint16_t context_id)
{
    _ctxt_res[context_id]->_render_context->apply();

    using namespace scm::math;
    using namespace scm::gl;

    context_state_objects_guard csg(_ctxt_res[context_id]->_render_context);
    context_texture_units_guard tug(_ctxt_res[context_id]->_render_context);

    memset(_ctxt_res[context_id]->_feedback_lod_cpu_buffer, 0, _ctxt_res[context_id]->_size_feedback * size_of_format(FORMAT_R_32I));

    int32_t* feedback_lod = (int32_t*)_ctxt_res[context_id]->_render_context->map_buffer(_ctxt_res[context_id]->_feedback_lod_storage, ACCESS_READ_ONLY);
    memcpy(_ctxt_res[context_id]->_feedback_lod_cpu_buffer, feedback_lod, _cut_update->get_context_feedback(context_id)->get_allocated_slot_index().size() * size_of_format(FORMAT_R_32I));
    _ctxt_res[context_id]->_render_context->sync();

    _ctxt_res[context_id]->_render_context->unmap_buffer(_ctxt_res[context_id]->_feedback_lod_storage);
    _ctxt_res[context_id]->_render_context->clear_buffer_data(_ctxt_res[context_id]->_feedback_lod_storage, FORMAT_R_32I, nullptr);

#ifdef RASTERIZATION_COUNT
    memset(_ctxt_res[context_id]->_feedback_count_cpu_buffer, 0, _ctxt_res[context_id]->_size_feedback * size_of_format(FORMAT_R_32UI));

    uint32_t* feedback_count = (uint32_t*)_ctxt_res[context_id]->_render_context->map_buffer(_ctxt_res[context_id]->_feedback_count_storage, ACCESS_READ_ONLY);
    memcpy(_ctxt_res[context_id]->_feedback_count_cpu_buffer, feedback_count, _ctxt_res[context_id]->_size_feedback * size_of_format(FORMAT_R_32UI));

    _ctxt_res[context_id]->_render_context->sync();
    _cut_update->feedback(context_id, _ctxt_res[context_id]->_feedback_lod_cpu_buffer, _ctxt_res[context_id]->_feedback_count_cpu_buffer);
#else
    _ctxt_res[context_id]->_render_context->sync();
    _cut_update->feedback(context_id, _ctxt_res[context_id]->_feedback_lod_cpu_buffer, nullptr);
#endif

#ifdef RASTERIZATION_COUNT
    _ctxt_res[context_id]->_render_context->unmap_buffer(_ctxt_res[context_id]->_feedback_count_storage);
    _ctxt_res[context_id]->_render_context->clear_buffer_data(_ctxt_res[context_id]->_feedback_count_storage, FORMAT_R_32UI, nullptr);
#endif
}

void VTRenderer::apply_cut_update(uint16_t context_id)
{
#ifndef NDEBUG
    auto start = std::chrono::high_resolution_clock::now();
#endif

    CutDatabase* cut_db = &CutDatabase::get_instance();

    for(cut_map_entry_type cut_entry : (*cut_db->get_cut_map()))
    {
        if(Cut::get_context_id(cut_entry.first) != context_id)
        {
            continue;
        }

        Cut* cut = cut_db->start_reading_cut(cut_entry.first);

        if(!cut->is_drawn())
        {
            cut_db->stop_reading_cut(cut_entry.first);
            continue;
        }

        std::set<uint16_t> updated_levels;

        for(auto position_slot_updated : cut->get_front()->get_mem_slots_updated())
        {
            const mem_slot_type* mem_slot_updated = cut_db->read_mem_slot_at(position_slot_updated.second, context_id);

            if(mem_slot_updated == nullptr || !mem_slot_updated->updated || !mem_slot_updated->locked || mem_slot_updated->pointer == nullptr)
            {
                if(mem_slot_updated == nullptr)
                {
                    std::cerr << "Mem slot at " << position_slot_updated.second << " is null" << std::endl;
                }
                else
                {
                    std::cerr << "Mem slot at " << position_slot_updated.second << std::endl;
                    std::cerr << "Mem slot #" << mem_slot_updated->position << std::endl;
                    std::cerr << "Tile id: " << mem_slot_updated->tile_id << std::endl;
                    std::cerr << "Locked: " << mem_slot_updated->locked << std::endl;
                    std::cerr << "Updated: " << mem_slot_updated->updated << std::endl;
                    std::cerr << "Pointer valid: " << (mem_slot_updated->pointer != nullptr) << std::endl;
                }

                throw std::runtime_error("updated mem slot inconsistency");
            }

            updated_levels.insert(QuadTree::get_depth_of_node(mem_slot_updated->tile_id));
            update_physical_texture_blockwise(context_id, mem_slot_updated->pointer, mem_slot_updated->position);
        }

        for(auto position_slot_cleared : cut->get_front()->get_mem_slots_cleared())
        {
            const mem_slot_type* mem_slot_cleared = cut_db->read_mem_slot_at(position_slot_cleared.second, context_id);

            if(mem_slot_cleared == nullptr)
            {
                std::cerr << "Mem slot at " << position_slot_cleared.second << " is null" << std::endl;
            }

            updated_levels.insert(QuadTree::get_depth_of_node(position_slot_cleared.first));
        }

        for(uint16_t updated_level : updated_levels)
        {
            // std::cout << "Updated level:  "+std::to_string(updated_level) << std::endl;
            update_index_texture(Cut::get_dataset_id(cut_entry.first), context_id, updated_level, cut->get_front()->get_index(updated_level));
        }

        cut_db->stop_reading_cut(cut_entry.first);
    }

    _ctxt_res[context_id]->_render_context->sync();

#ifndef NDEBUG
    auto end = std::chrono::high_resolution_clock::now();
    _apply_time = std::chrono::duration<float, std::milli>(end - start).count();
#endif
}

void VTRenderer::update_index_texture(uint32_t data_id, uint16_t context_id, uint16_t level, const uint8_t* buf_cpu)
{
    using namespace scm::math;
    using namespace scm::gl;

    uint32_t size_index_texture = (uint32_t)QuadTree::get_tiles_per_row(level);

    vec3ui origin = vec3ui(0, 0, 0);
    vec3ui dimensions = vec3ui(size_index_texture, size_index_texture, 1);

    _ctxt_res[context_id]->_render_context->update_sub_texture(_data_res[data_id]->_index_hierarchy.at(level), texture_region(origin, dimensions), 0, FORMAT_RGBA_8UI, buf_cpu);
}

scm::gl::data_format VTRenderer::get_tex_format()
{
    switch(VTConfig::get_instance().get_format_texture())
    {
    case VTConfig::R8:
        return scm::gl::FORMAT_R_8;
    case VTConfig::RGB8:
        return scm::gl::FORMAT_RGB_8;
    case VTConfig::RGBA8:
    default:
        return scm::gl::FORMAT_RGBA_8;
    }
}

void VTRenderer::update_physical_texture_blockwise(uint16_t context_id, const uint8_t* buf_texel, size_t slot_position)
{
    using namespace scm::math;
    using namespace scm::gl;

    size_t slots_per_texture = VTConfig::get_instance().get_phys_tex_tile_width() * VTConfig::get_instance().get_phys_tex_tile_width();
    size_t layer = slot_position / slots_per_texture;
    size_t rel_slot_position = slot_position - layer * slots_per_texture;
    size_t x_tile = rel_slot_position % VTConfig::get_instance().get_phys_tex_tile_width();
    size_t y_tile = rel_slot_position / VTConfig::get_instance().get_phys_tex_tile_width();

    vec3ui origin = vec3ui((uint32_t)x_tile * VTConfig::get_instance().get_size_tile(), (uint32_t)y_tile * VTConfig::get_instance().get_size_tile(), (uint32_t)layer);
    vec3ui dimensions = vec3ui(VTConfig::get_instance().get_size_tile(), VTConfig::get_instance().get_size_tile(), 1);

    _ctxt_res[context_id]->_render_context->update_sub_texture(_ctxt_res[context_id]->_physical_texture, texture_region(origin, dimensions), 0, get_tex_format(), buf_texel);
}

VTRenderer::~VTRenderer()
{
    for(auto res : _ctxt_res)
    {
        res.second->_feedback_index_ib.reset();
        res.second->_feedback_index_vb.reset();
        res.second->_feedback_vao.reset();
        res.second->_feedback_inv_index.reset();
        res.second->_feedback_lod_storage.reset();

        res.second->_filter_nearest.reset();
        res.second->_filter_linear.reset();

        res.second->_ms_no_cull.reset();
        res.second->_ms_cull.reset();
        res.second->_dstate_less.reset();
        res.second->_blend_state.reset();

        res.second->_obj.reset();

        res.second->_shader_vt.reset();
        res.second->_shader_vt_feedback.reset();

        res.second->_device.reset();

#ifdef RASTERIZATION_COUNT
        res.second->_feedback_count_storage.reset();
#endif
        delete[] res.second->_feedback_lod_cpu_buffer;
#ifdef RASTERIZATION_COUNT
        delete[] res.second->_feedback_count_cpu_buffer;
#endif
        delete res.second;
    }

    for(auto res : _view_res)
    {
        delete res.second;
    }

    for(auto res : _data_res)
    {
        delete res.second;
    }
}
void VTRenderer::extract_debug_cut(uint64_t cut_id)
{
    CutDatabase* cut_db = &CutDatabase::get_instance();

    cut_db->start_reading_cut(cut_id);

    if(_cut_debug_outputs[cut_id] == nullptr)
    {
        _cut_debug_outputs[cut_id] = new debug_cut();
    }

    std::stringstream stream_cut;
    stream_cut << "Cut { ";
    for(id_type iter : (*cut_db->get_cut_map())[cut_id]->get_front()->get_cut())
    {
        stream_cut << iter << " ";
    }
    stream_cut << "}" << std::endl;

    _cut_debug_outputs[cut_id]->_string_cut = stream_cut.str();

    std::stringstream stream_index_string;

    uint16_t depth = (uint16_t)(*cut_db->get_cut_map())[cut_id]->get_atlas()->getDepth();

    uint16_t level = 0;

    while(level < depth - 7)
    {
        size_t size_index_texture = QuadTree::get_tiles_per_row(level);

        for(size_t y = 0; y < size_index_texture; ++y)
        {
            for(size_t x = 0; x < size_index_texture; ++x)
            {
                auto ptr = &(*cut_db->get_cut_map())[cut_id]->get_front()->get_index(level)[y * size_index_texture * 4 + x * 4];

                stream_index_string << (int)ptr[0] << "." << (int)ptr[1] << "." << (int)ptr[2] << "." << (int)ptr[3] << " ";
            }

            stream_index_string << std::endl;
        }

        stream_index_string << std::endl;

        level++;
    }

    _cut_debug_outputs[cut_id]->_string_index = stream_index_string.str();

    cut_db->stop_reading_cut(cut_id);
}
void VTRenderer::extract_debug_cut_context(uint64_t cut_id)
{
    CutDatabase* cut_db = &CutDatabase::get_instance();

    cut_db->start_reading_cut(cut_id);

    uint16_t context_id = Cut::get_context_id(cut_id);

    if(_ctxt_debug_outputs[context_id] == nullptr)
    {
        _ctxt_debug_outputs[context_id] = new debug_context();
    }

    _ctxt_debug_outputs[context_id]->_fps.push_back(ImGui::GetIO().Framerate);
    _ctxt_debug_outputs[context_id]->_fps.pop_front();

    size_t free_slots = 0;

    std::stringstream stream_mem_slots;
    for(size_t layer = 0; layer < VTConfig::get_instance().get_phys_tex_layers(); layer++)
    {
        for(size_t y = 0; y < cut_db->get_size_mem_y(); ++y)
        {
            for(size_t x = 0; x < cut_db->get_size_mem_x(); ++x)
            {
                mem_slot_type* mem_slot = cut_db->read_mem_slot_at(x + y * cut_db->get_size_mem_x() + layer * cut_db->get_size_mem_x() * cut_db->get_size_mem_y(), context_id);

                if(!(*mem_slot).locked)
                {
                    stream_mem_slots << "F ";
                    free_slots++;
                }
                else
                {
                    stream_mem_slots << (*mem_slot).tile_id << " ";
                }
            }
            stream_mem_slots << std::endl;
        }
        stream_mem_slots << std::endl;
    }
    _ctxt_debug_outputs[context_id]->_string_mem_slots = stream_mem_slots.str();
    _ctxt_debug_outputs[context_id]->_mem_slots_busy = ((float)cut_db->get_size_mem_interleaved() - cut_db->get_available_memory(context_id)) / (float)cut_db->get_size_mem_interleaved();

    std::stringstream stream_feedback;
    size_t phys_tex_tile_width = VTConfig::get_instance().get_phys_tex_tile_width();

    for(size_t layer = 0; layer < VTConfig::get_instance().get_phys_tex_layers(); layer++)
    {
        for(size_t y = 0; y < phys_tex_tile_width; ++y)
        {
            for(size_t x = 0; x < phys_tex_tile_width; ++x)
            {
                stream_feedback << _ctxt_res[context_id]->_feedback_lod_cpu_buffer[x + y * phys_tex_tile_width + layer * phys_tex_tile_width * phys_tex_tile_width] << " ";
            }

            stream_feedback << std::endl;
        }
        stream_feedback << std::endl;
    }

    _ctxt_debug_outputs[context_id]->_string_feedback = stream_feedback.str();

    _ctxt_debug_outputs[context_id]->_string_mem_slots = stream_mem_slots.str();

    _ctxt_debug_outputs[context_id]->_times_cut_dispatch.push_back(_cut_update->get_dispatch_time());
    _ctxt_debug_outputs[context_id]->_times_cut_dispatch.pop_front();

    _ctxt_debug_outputs[context_id]->_times_apply.push_back(_apply_time);
    _ctxt_debug_outputs[context_id]->_times_apply.pop_front();

    cut_db->stop_reading_cut(cut_id);
}
void VTRenderer::render_debug_cut(uint32_t data_id, uint16_t view_id, uint16_t context_id)
{
    _ctxt_res[context_id]->_render_context->apply();

    uint64_t cut_id = (((uint64_t)data_id) << 32) | ((uint64_t)view_id << 16) | ((uint64_t)context_id);

    ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);

    if(!ImGui::Begin(("Dataset: " + std::to_string(data_id) + ", view: " + std::to_string(view_id) + ", context: " + std::to_string(context_id)).c_str()))
    {
        ImGui::End();
        return;
    }

    ImGui::SetNextTreeNodeOpen(true);

    if(ImGui::CollapsingHeader("Cut state"))
    {
        ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0), _cut_debug_outputs[cut_id]->_string_cut.c_str());
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 1.0f, 1.0), _cut_debug_outputs[cut_id]->_string_index.c_str());
    }

    ImGui::End();
}
void VTRenderer::render_debug_context(uint16_t context_id)
{
    _ctxt_res[context_id]->_render_context->apply();

    ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);

    if(!ImGui::Begin(("Context")))
    {
        ImGui::End();
        return;
    }

    ImVec2 plot_dims(0, 160);

    ImGui::SetNextTreeNodeOpen(true);

    if(ImGui::CollapsingHeader("Performance metrics"))
    {
        auto max_fps = *std::max_element(_ctxt_debug_outputs[context_id]->_fps.begin(), _ctxt_debug_outputs[context_id]->_fps.end());
        auto min_fps = *std::min_element(_ctxt_debug_outputs[context_id]->_fps.begin(), _ctxt_debug_outputs[context_id]->_fps.end());

        std::stringstream stream_average;
        stream_average << "Application average " << 1000.0f / ImGui::GetIO().Framerate << " ms/frame (" << ImGui::GetIO().Framerate << " FPS)\"";

        ImGui::PlotHistogram("FPS", &_ctxt_debug_outputs[context_id]->_fps[0], debug_context::FPS_S, 0, stream_average.str().c_str(), min_fps, max_fps, plot_dims);

        std::stringstream stream_usage;
        stream_usage << "Physical texture slots usage: " << _ctxt_debug_outputs[context_id]->_mem_slots_busy * 100 << "%";

        ImGui::ProgressBar(_ctxt_debug_outputs[context_id]->_mem_slots_busy, ImVec2(0, 80), stream_usage.str().c_str());
    }

    ImGui::SetNextTreeNodeOpen(true);

    if(ImGui::CollapsingHeader("Cut update metrics"))
    {
        auto max_disp = *std::max_element(_ctxt_debug_outputs[context_id]->_times_cut_dispatch.begin(), _ctxt_debug_outputs[context_id]->_times_cut_dispatch.end());
        auto max_apply = *std::max_element(_ctxt_debug_outputs[context_id]->_times_apply.begin(), _ctxt_debug_outputs[context_id]->_times_apply.end());

        auto min_disp = *std::min_element(_ctxt_debug_outputs[context_id]->_times_cut_dispatch.begin(), _ctxt_debug_outputs[context_id]->_times_cut_dispatch.end());
        auto min_apply = *std::min_element(_ctxt_debug_outputs[context_id]->_times_apply.begin(), _ctxt_debug_outputs[context_id]->_times_apply.end());

        ImGui::Text(_ctxt_debug_outputs[context_id]->_string_mem_slots.c_str());
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 1.0f, 1.0), _ctxt_debug_outputs[context_id]->_string_feedback.c_str());

        std::stringstream stream_dispatch_max;
        stream_dispatch_max << "Max: " << max_disp << " msec";
        ImGui::PlotLines("Dispatch time, msec", &_ctxt_debug_outputs[context_id]->_times_cut_dispatch[0], debug_context::DISP_S, 0, stream_dispatch_max.str().c_str(), min_disp, max_disp, plot_dims);

        std::stringstream stream_apply_max;
        stream_apply_max << "Max: " << max_apply << " msec";
        ImGui::PlotLines("Apply time, msec", &_ctxt_debug_outputs[context_id]->_times_apply[0], debug_context::APPLY_S, 0, stream_apply_max.str().c_str(), min_apply, max_apply, plot_dims);
    }

    ImGui::End();
}

void VTRenderer::toggle_visualization(int toggle) { _toggle_visualization = toggle; }

void VTRenderer::enable_hierarchy(bool enable) { _enable_hierarchy = enable; }
void VTRenderer::update_feedback_layout(uint16_t context_id)
{
    auto allocated_slots = &_cut_update->get_context_feedback(context_id)->get_allocated_slot_index();

    if(allocated_slots->empty())
    {
        return;
    }

    std::vector<uint32_t> output(allocated_slots->size());
    std::copy(allocated_slots->begin(), allocated_slots->end(), output.begin());

    _ctxt_res[context_id]->_render_context->apply();

    using namespace scm::math;
    using namespace scm::gl;

    context_state_objects_guard csg(_ctxt_res[context_id]->_render_context);
    context_vertex_input_guard cvg(_ctxt_res[context_id]->_render_context);

    auto mapped_index = (uint32_t*)_ctxt_res[context_id]->_render_context->map_buffer_range(_ctxt_res[context_id]->_feedback_index_vb, 0, output.size() * sizeof(uint32_t), ACCESS_WRITE_ONLY);
    memcpy(&mapped_index[0], &output[0], output.size() * sizeof(uint32_t));
    _ctxt_res[context_id]->_render_context->unmap_buffer(_ctxt_res[context_id]->_feedback_index_vb);

    _ctxt_res[context_id]->_render_context->clear_buffer_data(_ctxt_res[context_id]->_feedback_inv_index, FORMAT_R_32UI, nullptr);
    _ctxt_res[context_id]->_render_context->sync();

    _ctxt_res[context_id]->_render_context->bind_storage_buffer(_ctxt_res[context_id]->_feedback_inv_index, 2);
    _ctxt_res[context_id]->_render_context->bind_index_buffer(_ctxt_res[context_id]->_feedback_index_ib, PRIMITIVE_POINT_LIST, TYPE_UINT);
    _ctxt_res[context_id]->_render_context->bind_vertex_array(_ctxt_res[context_id]->_feedback_vao);
    _ctxt_res[context_id]->_render_context->apply();

    _ctxt_res[context_id]->_shader_vt_feedback->uniform("feedback_index_size", (const unsigned int)allocated_slots->size());
    _ctxt_res[context_id]->_render_context->bind_program(_ctxt_res[context_id]->_shader_vt_feedback);

    _ctxt_res[context_id]->_render_context->apply();

    _ctxt_res[context_id]->_render_context->draw_elements((const unsigned int)allocated_slots->size());
}
} // namespace vt