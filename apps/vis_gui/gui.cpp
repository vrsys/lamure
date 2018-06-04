//
// Created by moqe3167 on 30/05/18.
//

#include <lamure/pvs/pvs_database.h>
#include <lamure/ren/data_provenance.h>
#include "gui.h"
#include "imgui.h"
#include "imgui_impl_glfw_gl3.h"
#include "settings.h"

#include "window.h"
#include "input.h"
#include "selection.h"

gui::gui() :
        view_settings_{false},
        lod_settings_{false},
        visual_settings_{false},
        provenance_settings_{false} {}

void gui::view_settings(settings &setting, input &input) {
    ImGui::SetNextWindowPos(ImVec2(20, 390));
    ImGui::SetNextWindowSize(ImVec2(500.0f, 180.0f));
    ImGui::Begin("View Settings", &view_settings_, ImGuiWindowFlags_MenuBar);
    if (ImGui::SliderFloat("Near Plane", &setting.near_plane_, 0, 1.0f, "%.4f", 4.0f)) {
        input.gui_lock_ = true;
    }
    if (ImGui::SliderFloat("Far Plane", &setting.far_plane_, 0, 1000.0f, "%.4f", 4.0f)) {
        input.gui_lock_ = true;
    }
    if (ImGui::SliderFloat("FOV", &setting.fov_, 18, 60.0f)) {
        input.gui_lock_ = true;
    }
    if (ImGui::SliderFloat("Travel Speed", &setting.travel_speed_, 0.5f, 300.0f, "%.4f", 4.0f)) {
        input.gui_lock_ = true;
    }

    ImGui::End();
}


void gui::lod_settings(settings &setting, input &input) {

    ImGui::SetNextWindowPos(ImVec2(20, 590));
    ImGui::SetNextWindowSize(ImVec2(500.0f, 210.0f));

    ImGui::Begin("LOD Settings", &lod_settings_, ImGuiWindowFlags_MenuBar);

    ImGui::Checkbox("Lod Update", &setting.lod_update_);

    if (ImGui::SliderFloat("LOD Error", &setting.lod_error_, 1.0f, 10.0f, "%.4f", 2.5f)) {
        input.gui_lock_ = true;
    }
    if (ImGui::SliderFloat("LOD Point Scale", &setting.lod_point_scale_, 0.1f, 2.0f, "%.4f", 1.0f)) {
        input.gui_lock_ = true;
    }
    ImGui::Checkbox("Use PVS", &setting.use_pvs_);

    lamure::pvs::pvs_database::get_instance()->activate(setting.use_pvs_);


    ImGui::Checkbox("PVS Culling", &setting.pvs_culling_);


    ImGui::End();
}

void gui::visual_settings(settings &setting, input &input, lamure::ren::Data_Provenance &data_provenance) {
    const char* vis_values[] = {
            "Color", "Normals", "Accuracy",
            "Radius Deviation", "Output Sensitivity",
            "Provenance 1", "Provenance 2", "Provenance 3",
            "Provenance 4", "Provenance 5", "Provenance 6", "Provenance 7" };
    static int it = setting.vis_;

    ImGui::SetNextWindowPos(ImVec2(setting.width_-520, 20));
    ImGui::SetNextWindowSize(ImVec2(500.0f, 305.0f));
    ImGui::Begin("Visual Settings", &visual_settings_, ImGuiWindowFlags_MenuBar);

    uint32_t num_vis_entries = (5 + data_provenance.get_size_in_bytes()/sizeof(float));
    ImGui::Combo("Vis", &it, vis_values, num_vis_entries);
    setting.vis_ = it;

    if(setting.vis_ > num_vis_entries) {
        setting.vis_ = 0;
    }
    setting.show_normals_ = (setting.vis_ == 1);
    setting.show_accuracy_ = (setting.vis_ == 2);
    setting.show_radius_deviation_ = (setting.vis_ == 3);
    setting.show_output_sensitivity_ = (setting.vis_ == 4);
    if (setting.vis_ > 4) {
        setting.channel_ = (setting.vis_-4);
    }
    else {
        setting.channel_ = 0;
    }


    ImGui::Checkbox("Splatting", &setting.splatting_);
    ImGui::Checkbox("Gamma Correction", &setting.gamma_correction_);

    ImGui::Checkbox("Enable Lighting", &setting.enable_lighting_);
    ImGui::Checkbox("Use Material Color", &setting.use_material_color_);

    static ImVec4 color_mat_diff = ImColor(0.6f, 0.6f, 0.6f, 1.0f);
    ImGui::Text("Material Diffuse");
    ImGui::ColorEdit3("Diffuse", (float*)&color_mat_diff, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_NoAlpha);
    setting.material_diffuse_.x = color_mat_diff.x;
    setting.material_diffuse_.y = color_mat_diff.y;
    setting.material_diffuse_.z = color_mat_diff.z;
/*
    static ImVec4 color_mat_spec = ImColor(0.4f, 0.4f, 0.4f, 1.0f);
    ImGui::Text("Material Specular");
    ImGui::ColorEdit3("Specular", (float*)&color_mat_spec, ImGuiColorEditFlags_Float);
    setting.material_specular_.x = color_mat_spec.x;
    setting.material_specular_.y = color_mat_spec.y;
    setting.material_specular_.z = color_mat_spec.z;

    static ImVec4 color_ambient_light = ImColor(0.1f, 0.1f, 0.1f, 1.0f);
    ImGui::Text("Ambient Light Color");
    ImGui::ColorEdit3("Ambient", (float*)&color_ambient_light, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_NoAlpha);
    setting.ambient_light_color_.x = color_ambient_light.x;
    setting.ambient_light_color_.y = color_ambient_light.y;
    setting.ambient_light_color_.z = color_ambient_light.z;

    static ImVec4 color_point_light = ImColor(1.0f, 1.0f, 1.0f, 1.0f);
    ImGui::Text("Point Light Color");
    ImGui::ColorEdit3("Point", (float*)&color_point_light, ImGuiColorEditFlags_Float);
    setting.point_light_color_.x = color_point_light.x;
    setting.point_light_color_.y = color_point_light.y;
    setting.point_light_color_.z = color_point_light.z;
*/
    static ImVec4 background_color = ImColor(0.5f, 0.5f, 0.5f, 1.0f);
    ImGui::Text("Background Color");
    ImGui::ColorEdit3("Background", (float*)&background_color, ImGuiColorEditFlags_Float);
    setting.background_color_.x = background_color.x;
    setting.background_color_.y = background_color.y;
    setting.background_color_.z = background_color.z;

    ImGui::End();
}


void gui::provenance_settings(settings &setting, input &input) {

    ImGui::SetNextWindowPos(ImVec2(setting.width_-520, 345));
    ImGui::SetNextWindowSize(ImVec2(500.0f, 450.0f));
    ImGui::Begin("Provenance Settings", &provenance_settings_, ImGuiWindowFlags_MenuBar);

    if (ImGui::SliderFloat("AUX Point Size", &setting.aux_point_size_, 0.1f, 10.0f, "%.4f", 4.0f)) {
        input.gui_lock_ = true;
    }
    if (ImGui::SliderFloat("AUX Point Scale", &setting.aux_point_scale_, 0.1f, 2.0f, "%.4f", 4.0f)) {
        input.gui_lock_ = true;
    }
    if (ImGui::SliderFloat("AUX Focal Length", &setting.aux_focal_length_, 0.1f, 2.0f, "%.4f", 4.0f)) {
        input.gui_lock_ = true;
    }


    ImGui::Checkbox("Show Sparse", &setting.show_sparse_);
    if (setting.show_sparse_) {
        setting.enable_lighting_ = false;
        setting.splatting_ = false;
    }

    ImGui::Checkbox("Show Views", &setting.show_views_);
    if (setting.show_views_) {
        //setting.enable_lighting_ = false;
        setting.splatting_ = false;
    }

    ImGui::Checkbox("Show Photos", &setting.show_photos_);
    if (setting.show_views_) {
        //setting.enable_lighting_ = false;
        setting.splatting_ = false;
    }

    ImGui::Checkbox("Show Octrees", &setting.show_octrees_);
    if (setting.show_octrees_) {
        //setting.enable_lighting_ = false;
        setting.splatting_ = false;
    }

    ImGui::Checkbox("Heatmap", &setting.heatmap_);


    ImGui::InputFloat("Heatmap MIN", &setting.heatmap_min_);
    ImGui::InputFloat("Heatmap MAX", &setting.heatmap_max_);

    static ImVec4 color_heatmap_min = ImColor(68.0f/255.0f, 0.0f, 84.0f/255.0f, 1.0f);
    ImGui::Text("Heatmap Color Min");
    ImGui::ColorEdit3("Heatmap Min", (float*)&color_heatmap_min, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_NoAlpha);
    setting.heatmap_color_min_.x = color_heatmap_min.x;
    setting.heatmap_color_min_.y = color_heatmap_min.y;
    setting.heatmap_color_min_.z = color_heatmap_min.z;

    static ImVec4 color_heatmap_max = ImColor(251.f/255.f, 231.f/255.f, 35.f/255.f, 1.0f);
    ImGui::Text("Heatmap Color Max");
    ImGui::ColorEdit3("Heatmap Max", (float*)&color_heatmap_max, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_NoAlpha);
    setting.heatmap_color_max_.x = color_heatmap_max.x;
    setting.heatmap_color_max_.y = color_heatmap_max.y;
    setting.heatmap_color_max_.z = color_heatmap_max.z;


    ImGui::End();
}


void
gui::status_screen(settings &setting, input &input, selection &selection, lamure::ren::Data_Provenance &data_provenance, int32_t num_models, double fps, uint64_t rendered_splats, uint64_t rendered_nodes) {
    static bool status_screen = false;
    static int dataset = 0;

    char* vis_values[num_models+1] = { };
    for (int i=0; i<num_models+1; i++) {
        char buffer [32];
        snprintf(buffer, sizeof(buffer), "%s%d", "Dataset ", i);
        if(i==num_models){
            snprintf(buffer, sizeof(buffer), "%s", "All");
        }
        vis_values[i] = strdup(buffer);
    }

    ImGui::SetNextWindowPos(ImVec2(20, 20));
    ImGui::SetNextWindowSize(ImVec2(500.0f, 350.0f));
    ImGui::Begin("lamure_vis_gui", &status_screen, ImGuiWindowFlags_MenuBar);
    ImGui::Text("fps %d", (int32_t)fps);

    double f = (rendered_splats / 1000000.0);

    std::stringstream stream;
    stream << std::setprecision(2) << f;
    std::string s = stream.str();

    ImGui::Text("# points %s mio.", s.c_str());
    ImGui::Text("# nodes %d", (uint64_t)rendered_nodes);
    ImGui::Text("# models %d", num_models);

    ImGui::Combo("Dataset", &dataset, vis_values, IM_ARRAYSIZE(vis_values));

    if(dataset == (sizeof vis_values / sizeof vis_values[0]) - 1){
        selection.selected_model_ = -1;
    }else{
        selection.selected_model_ = dataset;
    }

    ImGui::Checkbox("View Settings", &view_settings_);
    ImGui::Checkbox("LOD Settings", &lod_settings_);
    ImGui::Checkbox("Visual Settings", &visual_settings_);
    if (setting.provenance_) {
        ImGui::Checkbox("Provenance Settings", &provenance_settings_);
    }
    ImGui::Checkbox("Brush", &input.brush_mode_);
    if (ImGui::Checkbox("Clear Brush", &input.brush_clear_)) {
        selection.brush_.clear();
        selection.selected_views_.clear();
        input.brush_clear_ = false;
    }

    if (view_settings_){
        view_settings(setting, input);
    }

    if (lod_settings_){
        lod_settings(setting, input);
    }

    if (visual_settings_){
        visual_settings(setting, input, data_provenance);
    }

    if (setting.provenance_ && provenance_settings_){
        provenance_settings(setting, input);
    }

    ImGui::End();
}