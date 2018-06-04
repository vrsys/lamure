//
// Created by moqe3167 on 29/05/18.
//

#include "vt_system.h"

vt_system::vt_system(const boost::shared_ptr<scm::gl::render_device> &device_,
                     const boost::shared_ptr<scm::gl::render_context> &context_,
                     const std::string &atlas_file_) :
        device_(device_),
        context_(context_),
        atlas_file_(atlas_file_) {
    init_vt_database();
    init_vt_system();
}

scm::gl::data_format vt_system::get_tex_format() {
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


void vt_system::init_vt_system() {
    enable_hierarchy_     = true;
    toggle_visualization_ = 0;

    // add_data
    uint16_t depth = (uint16_t) ((*vt::CutDatabase::get_instance().get_cut_map())[cut_id_]->get_atlas()->getDepth());
    uint16_t level = 0;

    while (level < depth) {
        uint32_t size_index_texture = (uint32_t) vt::QuadTree::get_tiles_per_row(level);

        auto index_texture_level_ptr = device_->create_texture_2d(
                scm::math::vec2ui(size_index_texture, size_index_texture), scm::gl::FORMAT_RGBA_8UI);

        device_->main_context()->clear_image_data(index_texture_level_ptr, 0, scm::gl::FORMAT_RGBA_8UI, 0);
        index_texture_hierarchy_.emplace_back(index_texture_level_ptr);

        level++;
    }

    // add_context
    context_ = device_->main_context();
    physical_texture_size_ = scm::math::vec2ui(vt::VTConfig::get_instance().get_phys_tex_tile_width(),
                                                   vt::VTConfig::get_instance().get_phys_tex_tile_width());

    auto physical_texture_size = scm::math::vec2ui(vt::VTConfig::get_instance().get_phys_tex_px_width(),
                                                   vt::VTConfig::get_instance().get_phys_tex_px_width());

    physical_texture_ = device_->create_texture_2d(physical_texture_size, get_tex_format(), 1,
                                                       vt::VTConfig::get_instance().get_phys_tex_layers() + 1);

    size_feedback_ = vt::VTConfig::get_instance().get_phys_tex_tile_width() *
                         vt::VTConfig::get_instance().get_phys_tex_tile_width() *
                         vt::VTConfig::get_instance().get_phys_tex_layers();

    feedback_lod_storage_   = device_->create_buffer(scm::gl::BIND_STORAGE_BUFFER, scm::gl::USAGE_STREAM_COPY,
                                                         size_feedback_ * size_of_format(scm::gl::FORMAT_R_32I));
    feedback_count_storage_ = device_->create_buffer(scm::gl::BIND_STORAGE_BUFFER, scm::gl::USAGE_STREAM_COPY,
                                                         size_feedback_ * size_of_format(scm::gl::FORMAT_R_32UI));

    feedback_lod_cpu_buffer_   = new int32_t[size_feedback_];
    feedback_count_cpu_buffer_ = new uint32_t[size_feedback_];

    for (size_t i = 0; i < size_feedback_; ++i) {
        feedback_lod_cpu_buffer_[i] = 0;
        feedback_count_cpu_buffer_[i] = 0;
    }

    vt_filter_linear_  = device_->create_sampler_state(scm::gl::FILTER_MIN_MAG_LINEAR , scm::gl::WRAP_CLAMP_TO_EDGE);
    vt_filter_nearest_ = device_->create_sampler_state(scm::gl::FILTER_MIN_MAG_NEAREST, scm::gl::WRAP_CLAMP_TO_EDGE);

    depth_state_less_ = device_->create_depth_stencil_state(true, true, scm::gl::COMPARISON_LESS);
    no_backface_culling_rasterizer_state_ = device_->create_rasterizer_state(scm::gl::FILL_SOLID, scm::gl::CULL_NONE, scm::gl::ORIENT_CCW, false, false, 0.0, false, false);
    color_no_blending_state_ = device_->create_blend_state(false);
}

void vt_system::init_vt_database() {
    vt::VTConfig::CONFIG_PATH = atlas_file_.substr(0, atlas_file_.size() - 5) + "ini";

    vt::VTConfig::get_instance().define_size_physical_texture(128, 8192);
    texture_id_ = vt::CutDatabase::get_instance().register_dataset(atlas_file_);
    context_id_ = vt::CutDatabase::get_instance().register_context();
    view_id_    = vt::CutDatabase::get_instance().register_view();
    cut_id_     = vt::CutDatabase::get_instance().register_cut(texture_id_, view_id_, context_id_);
    cut_update_ = &vt::CutUpdate::get_instance();
    cut_update_->start();
}

void vt_system::collect_vt_feedback() {
    int32_t *feedback_lod = (int32_t *) context_->map_buffer(feedback_lod_storage_, scm::gl::ACCESS_READ_ONLY);
    memcpy(feedback_lod_cpu_buffer_, feedback_lod, size_feedback_ * size_of_format(scm::gl::FORMAT_R_32I));
    context_->sync();

    context_->unmap_buffer(feedback_lod_storage_);
    context_->clear_buffer_data(feedback_lod_storage_, scm::gl::FORMAT_R_32I, nullptr);

    uint32_t *feedback_count = (uint32_t *) context_->map_buffer(feedback_count_storage_,
                                                                 scm::gl::ACCESS_READ_ONLY);
    memcpy(feedback_count_cpu_buffer_, feedback_count, size_feedback_ * size_of_format(scm::gl::FORMAT_R_32UI));
    context_->sync();

    cut_update_->feedback(feedback_lod_cpu_buffer_, feedback_count_cpu_buffer_);

    context_->unmap_buffer(feedback_count_storage_);
    context_->clear_buffer_data(feedback_count_storage_, scm::gl::FORMAT_R_32UI, nullptr);
}

void vt_system::apply_vt_cut_update() {
    auto *cut_db = &vt::CutDatabase::get_instance();

    for (vt::cut_map_entry_type cut_entry : (*cut_db->get_cut_map())) {
        vt::Cut *cut = cut_db->start_reading_cut(cut_entry.first);

        if (!cut->is_drawn()) {
            cut_db->stop_reading_cut(cut_entry.first);
            continue;
        }

        std::set<uint16_t> updated_levels;

        for (auto position_slot_updated : cut->get_front()->get_mem_slots_updated()) {
            const vt::mem_slot_type *mem_slot_updated = cut_db->read_mem_slot_at(position_slot_updated.second);

            if (mem_slot_updated == nullptr || !mem_slot_updated->updated
                || !mem_slot_updated->locked || mem_slot_updated->pointer == nullptr) {
                if (mem_slot_updated == nullptr) {
                    std::cerr << "Mem slot at " << position_slot_updated.second << " is null" << std::endl;
                } else {
                    std::cerr << "Mem slot at " << position_slot_updated.second << std::endl;
                    std::cerr << "Mem slot #" << mem_slot_updated->position << std::endl;
                    std::cerr << "Tile id: " << mem_slot_updated->tile_id << std::endl;
                    std::cerr << "Locked: " << mem_slot_updated->locked << std::endl;
                    std::cerr << "Updated: " << mem_slot_updated->updated << std::endl;
                    std::cerr << "Pointer valid: " << (mem_slot_updated->pointer != nullptr) << std::endl;
                }

                throw std::runtime_error("updated mem slot inconsistency");
            }

            updated_levels.insert(vt::QuadTree::get_depth_of_node(mem_slot_updated->tile_id));

            // update_physical_texture_blockwise
            size_t slots_per_texture = vt::VTConfig::get_instance().get_phys_tex_tile_width() *
                                       vt::VTConfig::get_instance().get_phys_tex_tile_width();
            size_t layer = mem_slot_updated->position / slots_per_texture;
            size_t rel_slot_position = mem_slot_updated->position - layer * slots_per_texture;

            size_t x_tile = rel_slot_position % vt::VTConfig::get_instance().get_phys_tex_tile_width();
            size_t y_tile = rel_slot_position / vt::VTConfig::get_instance().get_phys_tex_tile_width();

            scm::math::vec3ui origin = scm::math::vec3ui(
                    (uint32_t) x_tile * vt::VTConfig::get_instance().get_size_tile(),
                    (uint32_t) y_tile * vt::VTConfig::get_instance().get_size_tile(), (uint32_t) layer);
            scm::math::vec3ui dimensions = scm::math::vec3ui(vt::VTConfig::get_instance().get_size_tile(),
                                                             vt::VTConfig::get_instance().get_size_tile(), 1);

            context_->update_sub_texture(physical_texture_, scm::gl::texture_region(origin, dimensions), 0,
                                         get_tex_format(), mem_slot_updated->pointer);
        }


        for (auto position_slot_cleared : cut->get_front()->get_mem_slots_cleared()) {
            const vt::mem_slot_type *mem_slot_cleared = cut_db->read_mem_slot_at(position_slot_cleared.second);

            if (mem_slot_cleared == nullptr) {
                std::cerr << "Mem slot at " << position_slot_cleared.second << " is null" << std::endl;
            }

            updated_levels.insert(vt::QuadTree::get_depth_of_node(position_slot_cleared.first));
        }

        // update_index_texture
        for (uint16_t updated_level : updated_levels) {
            uint32_t size_index_texture = (uint32_t) vt::QuadTree::get_tiles_per_row(updated_level);

            scm::math::vec3ui origin = scm::math::vec3ui(0, 0, 0);
            scm::math::vec3ui dimensions = scm::math::vec3ui(size_index_texture, size_index_texture, 1);

            context_->update_sub_texture(index_texture_hierarchy_.at(updated_level),
                                         scm::gl::texture_region(origin, dimensions), 0, scm::gl::FORMAT_RGBA_8UI,
                                         cut->get_front()->get_index(updated_level));

        }

        cut_db->stop_reading_cut(cut_entry.first);
    }

    context_->sync();
}

void vt_system::render(lamure::ren::camera *camera, int32_t render_width, int32_t render_height, int32_t num_models,
                       selection selection) {
    context_->bind_program(vis_vt_shader_);

    uint64_t color_cut_id =
            (((uint64_t) texture_id_) << 32) | ((uint64_t) view_id_ << 16) | ((uint64_t) context_id_);
    uint32_t max_depth_level_color =
            (*vt::CutDatabase::get_instance().get_cut_map())[color_cut_id]->get_atlas()->getDepth() - 1;

    scm::math::mat4f view_matrix       = camera->get_view_matrix();
    scm::math::mat4f projection_matrix = scm::math::mat4f(camera->get_projection_matrix());

    vis_vt_shader_->uniform("model_view_matrix", view_matrix);
    vis_vt_shader_->uniform("projection_matrix", projection_matrix);

    vis_vt_shader_->uniform("physical_texture_dim", physical_texture_size_);
    vis_vt_shader_->uniform("max_level", max_depth_level_color);
    vis_vt_shader_->uniform("tile_size", scm::math::vec2((uint32_t) vt::VTConfig::get_instance().get_size_tile()));
    vis_vt_shader_->uniform("tile_padding", scm::math::vec2((uint32_t) vt::VTConfig::get_instance().get_size_padding()));

    vis_vt_shader_->uniform("enable_hierarchy", enable_hierarchy_);
    vis_vt_shader_->uniform("toggle_visualization", toggle_visualization_);

    for (uint32_t i = 0; i < index_texture_hierarchy_.size(); ++i) {
        std::string texture_string = "hierarchical_idx_textures";
        vis_vt_shader_->uniform(texture_string, i, int((i)));
    }

    vis_vt_shader_->uniform("physical_texture_array", 17);

    context_->set_viewport(
            scm::gl::viewport(scm::math::vec2ui(0, 0), 1 * scm::math::vec2ui(render_width, render_height)));

    context_->set_depth_stencil_state(depth_state_less_);
    context_->set_rasterizer_state(no_backface_culling_rasterizer_state_);
    context_->set_blend_state(color_no_blending_state_);

    context_->sync();

    apply_vt_cut_update();

    for (uint16_t i = 0; i < index_texture_hierarchy_.size(); ++i) {
        context_->bind_texture(index_texture_hierarchy_.at(i), vt_filter_nearest_, i);
    }

    context_->bind_texture(physical_texture_, vt_filter_linear_, 17);

    context_->bind_storage_buffer(feedback_lod_storage_, 0);
    context_->bind_storage_buffer(feedback_count_storage_, 1);

    context_->apply();

    for (int32_t model_id = 0; model_id < num_models; ++model_id) {
        if (selection.selected_model_ != -1) {
            model_id = selection.selected_model_;
        }

        auto t_res = image_plane_resources_[model_id];

        if (t_res.num_primitives_ > 0) {
            context_->bind_vertex_array(t_res.array_);
            context_->apply();
            if (selection.selected_views_.empty()) {
                context_->draw_arrays(scm::gl::PRIMITIVE_TRIANGLE_LIST, 0, t_res.num_primitives_);
            }
            else {
                for (const auto view : selection.selected_views_) {
                    context_->draw_arrays(scm::gl::PRIMITIVE_TRIANGLE_LIST, view * 3, 3);
                }
            }
        }

        if (selection.selected_model_ != -1) {
            break;
        }
    }
    context_->sync();

    collect_vt_feedback();
}



float vt_system::get_atlas_scale_factor() {
    auto atlas = new vt::pre::AtlasFile(atlas_file_.c_str());
    uint64_t image_width    = atlas->getImageWidth();
    uint64_t image_height   = atlas->getImageHeight();

    // tile's width and height without padding
    uint64_t tile_inner_width  = atlas->getInnerTileWidth();
    uint64_t tile_inner_height = atlas->getInnerTileHeight();

    // Quadtree depth counter, ranges from 0 to depth-1
    uint64_t depth = atlas->getDepth();

    double factor_u  = (double) image_width  / (tile_inner_width  * std::pow(2, depth-1));
    double factor_v  = (double) image_height / (tile_inner_height * std::pow(2, depth-1));

    return std::max(factor_u, factor_v);
}

void vt_system::set_shader_program(scm::gl::program_ptr shader_program) {
    vis_vt_shader_ = shader_program;
}

void vt_system::set_image_resources(map<uint32_t, resource> resource) {
    image_plane_resources_ = resource;
}


