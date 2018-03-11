#include <boost/assign/list_of.hpp>
#include <queue>
#include <unordered_map>

#include "VTRenderer.h"
#include <lamure/vt/ren/CutDatabase.h>
#include <lamure/vt/ren/CutUpdate.h>

namespace vt
{
VTRenderer::VTRenderer(CutUpdate *cut_update) : _dataset_resources(), _context_resources(), _view_resources()
{
    this->_cut_update = cut_update;
    this->init();
}

void VTRenderer::init()
{
    _scm_core.reset(new scm::core(0, nullptr));

    std::string vs_vt, fs_vt;

    if(!scm::io::read_text_file(std::string(LAMURE_SHADERS_DIR) + "/virtual_texturing.glslv", vs_vt) || !scm::io::read_text_file(std::string(LAMURE_SHADERS_DIR) + "/virtual_texturing.glslf", fs_vt))
    {
        scm::err() << "error reading shader files" << scm::log::end;
        throw std::runtime_error("Error reading shader files");
    }

    _device.reset(new scm::gl::render_device());

    {
        using namespace scm::gl;
        using namespace boost::assign;

        _shader_vt = _device->create_program(list_of(_device->create_shader(STAGE_VERTEX_SHADER, vs_vt))(_device->create_shader(STAGE_FRAGMENT_SHADER, fs_vt)));
    }

    if(!_shader_vt)
    {
        scm::err() << "error creating shader program" << scm::log::end;
        throw std::runtime_error("Error creating shader program");
    }

    _dstate_less = _device->create_depth_stencil_state(true, true, scm::gl::COMPARISON_LESS);
    _blend_state = _device->create_blend_state(true, scm::gl::FUNC_SRC_COLOR, scm::gl::FUNC_ONE_MINUS_SRC_ALPHA, scm::gl::FUNC_SRC_ALPHA, scm::gl::FUNC_ONE_MINUS_SRC_ALPHA);

    _filter_nearest = _device->create_sampler_state(scm::gl::FILTER_MIN_MAG_NEAREST, scm::gl::WRAP_CLAMP_TO_EDGE);
    _filter_linear = _device->create_sampler_state(scm::gl::FILTER_MIN_MAG_LINEAR, scm::gl::WRAP_CLAMP_TO_EDGE);

    _ms_no_cull = _device->create_rasterizer_state(scm::gl::FILL_SOLID, scm::gl::CULL_NONE, scm::gl::ORIENT_CCW, true);
    _ms_cull = _device->create_rasterizer_state(scm::gl::FILL_SOLID, scm::gl::CULL_BACK, scm::gl::ORIENT_CCW, true);
}

void VTRenderer::add_data(uint64_t cut_id, uint32_t dataset_id, const string &file_geometry)
{
    uint32_t size_index_texture = (uint32_t)QuadTree::get_tiles_per_row((*CutDatabase::get_instance().get_cut_map())[cut_id]->get_atlas()->getDepth() -1);

    dataset_resource *resource = new dataset_resource();

    resource->_obj = scm::shared_ptr<scm::gl::wavefront_obj_geometry>(new scm::gl::wavefront_obj_geometry(_device, std::string(LAMURE_PRIMITIVES_DIR) + "/world_smooth.obj"));
    // resource->_obj = scm::shared_ptr(new scm::gl::wavefront_obj_geometry(_device, std::string(LAMURE_PRIMITIVES_DIR) + "/world_smooth_finer.obj"));
    resource->_index_texture_dimension = scm::math::vec2ui(size_index_texture, size_index_texture);
    resource->_index_texture = _device->create_texture_2d(resource->_index_texture_dimension, scm::gl::FORMAT_RGBA_8UI);

    _dataset_resources[dataset_id] = resource;
}
void VTRenderer::add_view(uint16_t view_id, uint32_t width, uint32_t height, float scale)
{
    view_resource *resource = new view_resource();

    resource->_width = width;
    resource->_height = height;
    resource->_start = std::chrono::system_clock::now();
    resource->_scale = scale;

    _view_resources[view_id] = resource;
}
void VTRenderer::add_context(uint16_t context_id)
{
    context_resource *resource = new context_resource();

    // TODO: create auxiliary contexts
    resource->_render_context = _device->main_context();
    resource->_physical_texture_dimension = scm::math::vec2ui(VTConfig::get_instance().get_phys_tex_tile_width(), VTConfig::get_instance().get_phys_tex_tile_width());
    resource->_physical_texture = _device->create_texture_2d(resource->_physical_texture_dimension, get_tex_format(), 0, VTConfig::get_instance().get_phys_tex_layers() + 1);

    size_t copy_buffer_len = VTConfig::get_instance().get_phys_tex_tile_width() * VTConfig::get_instance().get_phys_tex_tile_width() * VTConfig::get_instance().get_phys_tex_layers();

    resource->_size_copy_buf = copy_buffer_len * size_of_format(scm::gl::FORMAT_R_32UI);
    resource->_atomic_feedback_storage_ssbo = _device->create_buffer(scm::gl::BIND_STORAGE_BUFFER, scm::gl::USAGE_STREAM_COPY, resource->_size_copy_buf);
    resource->_copy_memory_new = new uint32_t[copy_buffer_len];

    for(size_t i = 0; i < copy_buffer_len; ++i)
    {
        resource->_copy_memory_new[i] = 0;
    }

    _context_resources[context_id] = resource;
}
void VTRenderer::update_view(uint16_t view_id, uint32_t width, uint32_t height, float scale, const scm::math::mat4f& view_matrix)
{
    _view_resources[view_id]->_width = width;
    _view_resources[view_id]->_height = height;
    _view_resources[view_id]->_scale = scale;
    _view_resources[view_id]->_view_matrix = view_matrix;
}

void VTRenderer::render(uint32_t data_id, uint16_t view_id, uint16_t context_id)
{
    uint64_t cut_id = (((uint64_t)data_id) << 32) | ((uint64_t)view_id << 16) | ((uint64_t)view_id);
    uint32_t max_depth_level = (*CutDatabase::get_instance().get_cut_map())[cut_id]->get_atlas()->getDepth();

    scm::math::mat4f projection_matrix = scm::math::mat4f::identity();
    scm::math::perspective_matrix(projection_matrix, 10.f + _view_resources[view_id]->_scale * 100.f, float(_view_resources[view_id]->_width) / float(_view_resources[view_id]->_height), 0.01f, 1000.0f);
    std::chrono::duration<double> elapsed_seconds = std::chrono::high_resolution_clock::now() - _view_resources[view_id]->_start;

    _context_resources[context_id]->_render_context->set_default_frame_buffer();

    scm::math::mat4f model_matrix = scm::math::mat4f::identity() * scm::math::make_rotation((float)elapsed_seconds.count(), 0.f, 1.f, 0.f);

    scm::math::mat4f model_view_matrix = _view_resources[view_id]->_view_matrix * model_matrix;
    _shader_vt->uniform("projection_matrix", projection_matrix);
    _shader_vt->uniform("model_view_matrix", model_view_matrix);

    // upload necessary information to vertex shader
    _shader_vt->uniform("in_physical_texture_dim", _context_resources[context_id]->_physical_texture_dimension);
    _shader_vt->uniform("in_index_texture_dim", _dataset_resources[data_id]->_index_texture_dimension);
    _shader_vt->uniform("in_max_level", max_depth_level);
    _shader_vt->uniform("in_toggle_view", false);

    _shader_vt->uniform("in_tile_size", (uint32_t)VTConfig::get_instance().get_size_tile());
    _shader_vt->uniform("in_tile_padding", (uint32_t)VTConfig::get_instance().get_size_padding());

    _context_resources[context_id]->_render_context->clear_default_color_buffer(scm::gl::FRAMEBUFFER_BACK, scm::math::vec4f(.2f, .0f, .0f, 1.0f));
    _context_resources[context_id]->_render_context->clear_default_depth_stencil_buffer();

    _context_resources[context_id]->_render_context->apply();

    scm::gl::context_state_objects_guard csg(_context_resources[context_id]->_render_context);
    scm::gl::context_texture_units_guard tug(_context_resources[context_id]->_render_context);
    scm::gl::context_framebuffer_guard fbg(_context_resources[context_id]->_render_context);

    _context_resources[context_id]->_render_context->set_viewport(scm::gl::viewport(scm::math::vec2ui(0, 0), 1 * scm::math::vec2ui(_view_resources[view_id]->_width, _view_resources[view_id]->_height)));

    _context_resources[context_id]->_render_context->set_depth_stencil_state(_dstate_less);
    _context_resources[context_id]->_render_context->set_rasterizer_state(_ms_cull);
    _context_resources[context_id]->_render_context->set_blend_state(_blend_state);

    _context_resources[context_id]->_render_context->bind_program(_shader_vt);

    _context_resources[context_id]->_render_context->sync();

    apply_cut_update(context_id);

    // bind our texture and tell the graphics card to filter the samples linearly
    _context_resources[context_id]->_render_context->bind_texture(_context_resources[context_id]->_physical_texture, _filter_linear, 0);
    _context_resources[context_id]->_render_context->bind_texture(_dataset_resources[data_id]->_index_texture, _filter_nearest, 1);

    // bind feedback
    _context_resources[context_id]->_render_context->bind_storage_buffer(_context_resources[context_id]->_atomic_feedback_storage_ssbo, 0);

    _context_resources[context_id]->_render_context->apply();

    _dataset_resources[data_id]->_obj->draw(_context_resources[context_id]->_render_context, scm::gl::geometry::MODE_SOLID);

    auto data =_context_resources[context_id]->_render_context->map_buffer(_context_resources[context_id]->_atomic_feedback_storage_ssbo, scm::gl::ACCESS_READ_ONLY);

    if(data)
    {
        memcpy(_context_resources[context_id]->_copy_memory_new, data, _context_resources[context_id]->_size_copy_buf);
    }

    _context_resources[context_id]->_render_context->unmap_buffer(_context_resources[context_id]->_atomic_feedback_storage_ssbo);

    _context_resources[context_id]->_render_context->clear_buffer_data(_context_resources[context_id]->_atomic_feedback_storage_ssbo, scm::gl::FORMAT_R_32UI, nullptr);

    _context_resources[context_id]->_render_context->sync();

    _cut_update->feedback(_context_resources[context_id]->_copy_memory_new);
}

void VTRenderer::apply_cut_update(uint16_t context_id)
{
    CutDatabase *cut_db = &CutDatabase::get_instance();

    cut_db->start_reading();

    for(cut_map_entry_type cut_entry : (*cut_db->get_cut_map()))
    {
        Cut *cut = cut_db->start_reading_cut(cut_entry.first);

        if(!cut->is_drawn())
        {
            cut_db->stop_reading_cut(cut_entry.first);
            continue;
        }

        update_index_texture(Cut::get_dataset_id(cut_entry.first), context_id, cut->get_front()->get_index());

        for(auto position_slot_updated : cut->get_front()->get_mem_slots_updated())
        {
            const mem_slot_type *mem_slot_updated = &cut_db->get_front()->at(position_slot_updated.second);

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

            update_physical_texture_blockwise(context_id, mem_slot_updated->pointer, mem_slot_updated->position);
        }

        cut_db->stop_reading_cut(cut_entry.first);
    }

    cut_db->stop_reading();

    _context_resources[context_id]->_render_context->sync();
}

void VTRenderer::update_index_texture(uint32_t dataset_id, uint16_t context_id, const uint8_t *buf_cpu)
{
    scm::math::vec3ui origin = scm::math::vec3ui(0, 0, 0);
    scm::math::vec3ui dimensions = scm::math::vec3ui(_dataset_resources[dataset_id]->_index_texture_dimension, 1);

    _context_resources[context_id]->_render_context->update_sub_texture(_dataset_resources[dataset_id]->_index_texture, scm::gl::texture_region(origin, dimensions), 0, scm::gl::FORMAT_RGBA_8UI, buf_cpu);
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

void VTRenderer::update_physical_texture_blockwise(uint16_t context_id, const uint8_t *buf_texel, size_t slot_position)
{
    size_t slots_per_texture = VTConfig::get_instance().get_phys_tex_tile_width() * VTConfig::get_instance().get_phys_tex_tile_width();
    size_t layer = slot_position / slots_per_texture;
    size_t rel_slot_position = slot_position - layer * slots_per_texture;
    size_t x_tile = rel_slot_position % VTConfig::get_instance().get_phys_tex_tile_width();
    size_t y_tile = rel_slot_position / VTConfig::get_instance().get_phys_tex_tile_width();

    scm::math::vec3ui origin = scm::math::vec3ui((uint32_t)x_tile * VTConfig::get_instance().get_size_tile(), (uint32_t)y_tile * VTConfig::get_instance().get_size_tile(), (uint32_t)layer);
    scm::math::vec3ui dimensions = scm::math::vec3ui(VTConfig::get_instance().get_size_tile(), VTConfig::get_instance().get_size_tile(), 1);

    _context_resources[context_id]->_render_context->update_sub_texture(_context_resources[context_id]->_physical_texture, scm::gl::texture_region(origin, dimensions), 0, get_tex_format(), buf_texel);
}

VTRenderer::~VTRenderer()
{
    _shader_vt.reset();

    _filter_nearest.reset();
    _filter_linear.reset();

    _ms_no_cull.reset();

    _device.reset();
    _scm_core.reset();
}
}