#ifndef FEM_VIS_SSBO_APP_SETTINGS_H_
#define FEM_VIS_SSBO_APP_SETTINGS_H_

#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <vector>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <algorithm>
#include <list>
#include <sstream>

#include <scm/core.h>
#include <scm/core/math.h>
#include <scm/core/io/tools.h>
#include <scm/core/pointer_types.h>
#include <scm/gl_core/gl_core_fwd.h>
#include <scm/gl_util/primitives/primitives_fwd.h>
#include <scm/core/platform/platform.h>
#include <scm/core/utilities/platform_warning_disable.h>
#include <scm/gl_util/primitives/quad.h>
#include <scm/gl_util/font/font_face.h>
#include <scm/gl_util/font/text.h>
#include <scm/gl_util/font/text_renderer.h>
#include <scm/core/time/accum_timer.h>
#include <scm/core/time/high_res_timer.h>
#include <scm/gl_util/data/imaging/texture_loader.h>
#include <scm/gl_util/primitives/geometry.h>
#include <scm/gl_util/primitives/box.h>

//boost
#include <boost/assign/list_of.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

//lamure
#include <lamure/types.h>
#include <lamure/ren/config.h>
#include <lamure/ren/model_database.h>
#include <lamure/ren/cut_database.h>
#include <lamure/ren/dataset.h>
#include <lamure/ren/policy.h>
#include <lamure/ren/controller.h>

#include <lamure/ren/ray.h>
#include <lamure/prov/auxi.h>
#include <lamure/prov/octree.h>
#include <lamure/vt/VTConfig.h>
#include <lamure/vt/ren/CutDatabase.h>
#include <lamure/vt/ren/CutUpdate.h>
#include <lamure/vt/pre/AtlasFile.h>

#include <lamure/ren/3rd_party/json.h>

struct settings {
  int32_t width_ {1920};
  int32_t height_ {1080};
  int32_t frame_div_ {1};
  int32_t vram_ {4096};
  int32_t ram_ {16096};
  int32_t upload_ {32};
  bool provenance_ {0};
  bool create_aux_resources_ {1};
  float near_plane_ {1.0f};
  float far_plane_ {5000.0f};
  float fov_ {30.0f};
  bool splatting_ {1};
  bool gamma_correction_ {0};
  int32_t gui_ {1};
  int32_t travel_ {2};
  float travel_speed_ {100.5f};
  int32_t max_brush_size_{4096};
  bool lod_update_ {1};
  bool use_pvs_ {1};
  bool pvs_culling_ {0};
  float lod_point_scale_ {1.0f};
  float aux_point_size_ {1.0f};
  float aux_point_distance_ {0.5f};
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
  bool show_bvhs_ {0};
  bool show_pvs_ {0};
  int32_t channel_ {0};
  int32_t fem_result_ {0};
  int32_t fem_vis_mode_ {0};
  float fem_deform_factor_ {1.0};
  std::string fem_value_mapping_file_;
  float lod_error_ {LAMURE_DEFAULT_THRESHOLD};
  bool enable_lighting_ {0};
  bool use_material_color_ {0};
  scm::math::vec3f material_diffuse_ {0.6f, 0.6f, 0.6f};
  scm::math::vec4f material_specular_ {0.4f, 0.4f, 0.4f, 1000.0f};
  scm::math::vec3f ambient_light_color_ {0.1f, 0.1f, 0.1f};
  scm::math::vec4f point_light_color_ {1.0f, 1.0f, 1.0f, 1.2f};
  scm::math::mat4f fem_to_pcl_transform_ {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
  bool heatmap_ {0};
  float heatmap_min_ {0.0f};
  float heatmap_max_ {1.0f};
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
  std::map<uint32_t, std::vector<lamure::prov::auxi::view>> views_;
  std::map<uint32_t, std::string> aux_;
  std::map<uint32_t, std::string> textures_;
  std::map<uint32_t, int> min_lod_depths_;
  std::string selection_ {""};
  float max_radius_ {std::numeric_limits<float>::max()};
  int color_rendering_mode {0};
};


#endif //FEM_VIS_SSBO_APP_SETTINGS_H_