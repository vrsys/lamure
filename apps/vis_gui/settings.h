//
// Created by moqe3167 on 30/05/18.
//

#ifndef LAMURE_SETTINGS_H
#define LAMURE_SETTINGS_H

#include <scm/core/math.h>
#include <lamure/prov/octree.h>
#include <lamure/ren/config.h>

#include <cstdint>
#include <map>

#include "utils.h"

using namespace vis_utils;

struct settings {
    void load(std::string const& vis_file_name) {

        std::ifstream vis_file(vis_file_name.c_str());

        if (!vis_file.is_open()) {
            std::cout << "could not open vis file" << std::endl;
            exit(-1);
        }
        else {
            lamure::model_t model_id = 0;

            std::string line;
            while(std::getline(vis_file, line)) {
                if(line.length() >= 2) {
                    if (line[0] == '#') {
                        continue;
                    }
                    auto colon = line.find_first_of(':');
                    if (colon == std::string::npos) {
                        scm::math::mat4d transform = scm::math::mat4d::identity();
                        std::string model;

                        std::istringstream line_ss(line);
                        line_ss >> model;

                        this->models_.push_back(model);
                        this->transforms_[model_id] = scm::math::mat4d::identity();
                        this->aux_[model_id] = "";
                        ++model_id;

                    }
                    else {
                        std::string key = line.substr(0, colon);

                        if (key[0] == '@') {
                            auto ws = line.find_first_of(' ');
                            uint32_t address = atoi(strip_whitespace(line.substr(1, ws-1)).c_str());
                            key = strip_whitespace(line.substr(ws+1, colon-(ws+1)));
                            std::string value = strip_whitespace(line.substr(colon+1));

                            if (key == "tf") {
                                this->transforms_[address] = load_matrix(value);
                                std::cout << "found transform for model id " << address << std::endl;
                            }
                            else if (key == "aux") {
                                this->aux_[address] = value;
                                std::cout << "found aux data for model id " << address << std::endl;
                            }
                            else {
                                std::cout << "unrecognized key: " << key << std::endl;
                                exit(-1);
                            }
                            continue;
                        }

                        key = strip_whitespace(key);
                        std::string value = strip_whitespace(line.substr(colon+1));

                        if (key == "width") {
                            this->width_ = std::max(atoi(value.c_str()), 64);
                        }
                        else if (key == "height") {
                            this->height_ = std::max(atoi(value.c_str()), 64);
                        }
                        else if (key == "frame_div") {
                            this->frame_div_ = std::max(atoi(value.c_str()), 1);
                        }
                        else if (key == "vram") {
                            this->vram_ = std::max(atoi(value.c_str()), 8);
                        }
                        else if (key == "ram") {
                            this->ram_ = std::max(atoi(value.c_str()), 8);
                        }
                        else if (key == "upload") {
                            this->upload_ = std::max(atoi(value.c_str()), 8);
                        }
                        else if (key == "near") {
                            this->near_plane_ = std::max(atof(value.c_str()), 0.0);
                        }
                        else if (key == "far") {
                            this->far_plane_ = std::max(atof(value.c_str()), 0.1);
                        }
                        else if (key == "fov") {
                            this->fov_ = std::max(atof(value.c_str()), 9.0);
                        }
                        else if (key == "splatting") {
                            this->splatting_ = (bool)std::max(atoi(value.c_str()), 0);
                        }
                        else if (key == "gamma_correction") {
                            this->gamma_correction_ = (bool)std::max(atoi(value.c_str()), 0);
                        }
                        else if (key == "gui") {
                            this->gui_ = std::max(atoi(value.c_str()), 0);
                        }
                        else if (key == "speed") {
                            this->travel_speed_ = std::min(std::max(atof(value.c_str()), 0.0), 400.0);
                        }
                        else if (key == "pvs_culling") {
                            this->pvs_culling_ = (bool)std::max(atoi(value.c_str()), 0);
                        }
                        else if (key == "use_pvs") {
                            this->use_pvs_ = (bool)std::max(atoi(value.c_str()), 0);
                        }
                        else if (key == "lod_point_scale") {
                            this->lod_point_scale_ = std::min(std::max(atof(value.c_str()), 0.0), 10.0);
                        }
                        else if (key == "aux_point_size") {
                            this->aux_point_size_ = std::min(std::max(atof(value.c_str()), 0.001), 1.0);
                        }
                        else if (key == "aux_focal_length") {
                            this->aux_focal_length_ = std::min(std::max(atof(value.c_str()), 0.001), 10.0);
                        }
                        else if (key == "lod_error") {
                            this->lod_error_ = std::min(std::max(atof(value.c_str()), 0.0), 10.0);
                        }
                        else if (key == "provenance") {
                            this->provenance_ = (bool)std::max(atoi(value.c_str()), 0);
                        }
                        else if (key == "show_normals") {
                            this->show_normals_ = std::max(atoi(value.c_str()), 0);
                        }
                        else if (key == "show_accuracy") {
                            this->show_accuracy_ = (bool)std::max(atoi(value.c_str()), 0);
                        }
                        else if (key == "show_radius_deviation") {
                            this->show_radius_deviation_ = (bool)std::max(atoi(value.c_str()), 0);
                        }
                        else if (key == "show_output_sensitivity") {
                            this->show_output_sensitivity_ = (bool)std::max(atoi(value.c_str()), 0);
                        }
                        else if (key == "show_sparse") {
                            this->show_sparse_ = (bool)std::max(atoi(value.c_str()), 0);
                        }
                        else if (key == "show_views") {
                            this->show_views_ = (bool)std::max(atoi(value.c_str()), 0);
                        }
                        else if (key == "show_photos") {
                            this->show_photos_ = (bool)std::max(atoi(value.c_str()), 0);
                        }
                        else if (key == "show_octrees") {
                            this->show_octrees_ = (bool)std::max(atoi(value.c_str()), 0);
                        }
                        else if (key == "channel") {
                            this->channel_ = std::max(atoi(value.c_str()), 0);
                        }
                        else if (key == "enable_lighting") {
                            this->enable_lighting_ = (bool)std::min(std::max(atoi(value.c_str()), 0), 1);
                        }
                        else if (key == "use_material_color") {
                            this->use_material_color_ = (bool)std::min(std::max(atoi(value.c_str()), 0), 1);
                        }
                        else if (key == "material_diffuse_r") {
                            this->material_diffuse_.x = std::max(atof(value.c_str()), 0.0);
                        }
                        else if (key == "material_diffuse_g") {
                            this->material_diffuse_.y = std::max(atof(value.c_str()), 0.0);
                        }
                        else if (key == "material_diffuse_b") {
                            this->material_diffuse_.z = std::max(atof(value.c_str()), 0.0);
                        }
                        else if (key == "material_specular_r") {
                            this->material_specular_.x = std::max(atof(value.c_str()), 0.0);
                        }
                        else if (key == "material_specular_g") {
                            this->material_specular_.y = std::max(atof(value.c_str()), 0.0);
                        }
                        else if (key == "material_specular_b") {
                            this->material_specular_.z = std::max(atof(value.c_str()), 0.0);
                        }
                        else if (key == "material_specular_exponent") {
                            this->material_specular_.w = std::min(std::max(atof(value.c_str()), 0.0), 10000.0);
                        }
                        else if (key == "ambient_light_color_r") {
                            this->ambient_light_color_.r = std::min(std::max(atof(value.c_str()), 0.0), 1.0);
                        }
                        else if (key == "ambient_light_color_g") {
                            this->ambient_light_color_.g = std::min(std::max(atof(value.c_str()), 0.0), 1.0);
                        }
                        else if (key == "ambient_light_color_b") {
                            this->ambient_light_color_.b = std::min(std::max(atof(value.c_str()), 0.0), 1.0);
                        }
                        else if (key == "point_light_color_r") {
                            this->point_light_color_.r = std::min(std::max(atof(value.c_str()), 0.0), 1.0);
                        }
                        else if (key == "point_light_color_g") {
                            this->point_light_color_.g = std::min(std::max(atof(value.c_str()), 0.0), 1.0);
                        }
                        else if (key == "point_light_color_b") {
                            this->point_light_color_.b = std::min(std::max(atof(value.c_str()), 0.0), 1.0);
                        }
                        else if (key == "point_light_intensity") {
                            this->point_light_color_.w = std::min(std::max(atof(value.c_str()), 0.0), 10000.0);
                        }
                        else if (key == "background_color_r") {
                            this->background_color_.x = std::min(std::max(atoi(value.c_str()), 0), 255)/255.f;
                        }
                        else if (key == "background_color_g") {
                            this->background_color_.y = std::min(std::max(atoi(value.c_str()), 0), 255)/255.f;
                        }
                        else if (key == "background_color_b") {
                            this->background_color_.z = std::min(std::max(atoi(value.c_str()), 0), 255)/255.f;
                        }
                        else if (key == "heatmap") {
                            this->heatmap_ = (bool)std::max(atoi(value.c_str()), 0);
                        }
                        else if (key == "heatmap_min") {
                            this->heatmap_min_ = std::max(atof(value.c_str()), 0.0);
                        }
                        else if (key == "heatmap_max") {
                            this->heatmap_max_ = std::max(atof(value.c_str()), 0.0);
                        }
                        else if (key == "heatmap_min_r") {
                            this->heatmap_color_min_.x = std::min(std::max(atoi(value.c_str()), 0), 255)/255.f;
                        }
                        else if (key == "heatmap_min_g") {
                            this->heatmap_color_min_.y = std::min(std::max(atoi(value.c_str()), 0), 255)/255.f;
                        }
                        else if (key == "heatmap_min_b") {
                            this->heatmap_color_min_.z = std::min(std::max(atoi(value.c_str()), 0), 255)/255.f;
                        }
                        else if (key == "heatmap_max_r") {
                            this->heatmap_color_max_.x = std::min(std::max(atoi(value.c_str()), 0), 255)/255.f;
                        }
                        else if (key == "heatmap_max_g") {
                            this->heatmap_color_max_.y = std::min(std::max(atoi(value.c_str()), 0), 255)/255.f;
                        }
                        else if (key == "heatmap_max_b") {
                            this->heatmap_color_max_.z = std::min(std::max(atoi(value.c_str()), 0), 255)/255.f;
                        }
                        else if (key == "atlas_file") {
                            this->atlas_file_ = value;
                        }
                        else if (key == "json") {
                            this->json_ = value;
                        }
                        else if (key == "pvs") {
                            this->pvs_ = value;
                        }
                        else if (key == "background_image") {
                            this->background_image_ = value;
                        }
                        else if (key == "use_view_tf") {
                            this->use_view_tf_ = std::max(atoi(value.c_str()), 0);
                        }
                        else if (key == "view_tf") {
                            this->view_tf_ = load_matrix(value);
                        }
                        else {
                            std::cout << "unrecognized key: " << key << std::endl;
                            exit(-1);
                        }
                        //std::cout << key << " : " << value << std::endl;
                    }
                }
            }
            vis_file.close();
        }

        //assertions
        if (this->provenance_ != 0) {
            if (this->json_.size() > 0) {
                if (this->json_.substr(this->json_.size()-5) != ".json") {
                    std::cout << "unsupported json file" << std::endl;
                    exit(-1);
                }
            }
        }
        if (this->models_.empty()) {
            std::cout << "error: no model filename specified" << std::endl;
            exit(-1);
        }
        if (this->pvs_.size() > 0) {
            if (this->pvs_.substr(this->pvs_.size()-4) != ".pvs") {
                std::cout << "unsupported pvs file" << std::endl;
                exit(-1);
            }
        }
    }

    int32_t width_ {1920};
    int32_t height_ {1080};
    int32_t frame_div_ {1};
    int32_t vram_ {2048};
    int32_t ram_ {4096};
    int32_t upload_ {32};
    bool provenance_ {0};
    float near_plane_ {0.001f};
    float far_plane_ {1000.0f};
    float fov_ {30.0f};
    bool splatting_ {1};
    bool gamma_correction_ {1};
    int32_t gui_ {1};
    int32_t travel_ {2};
    float travel_speed_ {20.5f};
    bool lod_update_ {1};
    bool use_pvs_ {1};
    bool pvs_culling_ {0};
    float lod_point_scale_ {1.0f};
    float aux_point_size_ {1.0f};
    float aux_point_scale_ {1.0f};
    float aux_focal_length_ {1.0f};
    int32_t vis_ {0};
    int32_t show_normals_ {0};
    bool show_accuracy_ {0};
    bool show_radius_deviation_ {0};
    bool show_output_sensitivity_ {0};
    bool show_sparse_ {0};
    bool show_views_ {0};
    bool show_photos_ {0};
    bool show_octrees_ {0};
    int32_t channel_ {0};
    float lod_error_ {LAMURE_DEFAULT_THRESHOLD};
    bool enable_lighting_ {1};
    bool use_material_color_ {0};
    scm::math::vec3f material_diffuse_ {0.6f, 0.6f, 0.6f};
    scm::math::vec4f material_specular_ {0.4f, 0.4f, 0.4f, 1000.0f};
    scm::math::vec3f ambient_light_color_ {0.1f, 0.1f, 0.1f};
    scm::math::vec4f point_light_color_ {1.0f, 1.0f, 1.0f, 1.2f};
    bool heatmap_ {0};
    float heatmap_min_ {0.0f};
    float heatmap_max_ {0.05f};
    scm::math::vec3f background_color_ {LAMURE_DEFAULT_COLOR_R, LAMURE_DEFAULT_COLOR_G, LAMURE_DEFAULT_COLOR_B};
    scm::math::vec3f heatmap_color_min_ {68.0f/255.0f, 0.0f, 84.0f/255.0f};
    scm::math::vec3f heatmap_color_max_ {251.f/255.f, 231.f/255.f, 35.f/255.f};
    std::string atlas_file_ {""};
    std::string json_ {""};
    std::string pvs_ {""};
    std::string background_image_ {""};
    int32_t use_view_tf_ {0};
    scm::math::mat4d view_tf_ {scm::math::mat4d::identity()};
    std::vector<std::string> models_;
    std::map<uint32_t, scm::math::mat4d> transforms_;
    std::map<uint32_t, std::shared_ptr<lamure::prov::octree>> octrees_;
    std::map<uint32_t, std::string> aux_;
};

#endif //LAMURE_SETTINGS_H
