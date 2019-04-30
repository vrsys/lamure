#ifndef CHART_PACKING_H_
#define CHART_PACKING_H_


#include <lamure/mesh/triangle_chartid.h>

#include <scm/core.h>

struct rectangle {
  scm::math::vec2f min_;
  scm::math::vec2f max_;
  int id_;
  bool flipped_;
};


struct projection_info {
  scm::math::vec3f proj_centroid;
  scm::math::vec3f proj_normal;
  scm::math::vec3f proj_world_up;

  rectangle tex_space_rect;
  scm::math::vec2f tex_coord_offset;

  float largest_max;
};

struct chart {
  int id_;
  rectangle rect_;
  lamure::bounding_box box_;
  std::set<int> all_triangle_ids_;
  std::set<int> original_triangle_ids_;
  std::map<int, std::vector<scm::math::vec2f>> all_triangle_new_coods_;
  projection_info projection;
  double real_to_tex_ratio_old;
  double real_to_tex_ratio_new;
};



struct texture_info {
  std::string filename_;
  scm::math::vec2i size_;
};


static double get_area_of_triangle(scm::math::vec2f v0, scm::math::vec2f v1, scm::math::vec2f v2){
  return 0.5 * scm::math::length(scm::math::cross(scm::math::vec3f(v1-v0, 0.0), scm::math::vec3f(v2-v0, 0.0)));
}


static void calculate_chart_tex_space_sizes(std::map<uint32_t, chart>& chart_map, 
                                     const std::vector<lamure::mesh::Triangle_Chartid>& triangles, 
                                     std::map<uint32_t, texture_info>& texture_info_map) {

  //calculate average pixels per triangle for each chart, given texture size
  for(auto& chart_it : chart_map) {

    double pixels = 0.0;
    auto& chart = chart_it.second;

    for (auto& tri_id : chart.original_triangle_ids_) {

      //sample length  between vertices 0 and 1
      auto& tri = triangles[tri_id];

      int texture_id = tri.tex_id;

      //calculate areas in texture space
      double tri_area = get_area_of_triangle(tri.v0_.tex_, tri.v1_.tex_, tri.v2_.tex_);
      double pixel_area = (1.0 / texture_info_map[texture_id].size_.x) * (1.0 / texture_info_map[texture_id].size_.y);
      double tri_pixels = tri_area / pixel_area;

      pixels += tri_pixels;
    }

    double pixels_per_tri = pixels / chart.original_triangle_ids_.size();

    chart.real_to_tex_ratio_old = pixels_per_tri;
  }

}

static void calculate_new_chart_tex_space_sizes(std::map<uint32_t, std::map<uint32_t, chart>>& chart_map, 
                                                std::vector<lamure::mesh::Triangle_Chartid>& triangles, 
                                                const scm::math::vec2i& texture_dimensions) {
  for(auto& area_it : chart_map) {
  	for (auto& chart_it : area_it.second) {
  		auto& chart = chart_it.second;

    double pixels = 0.0;
    for (auto& tri_id : chart.original_triangle_ids_) {
      //sample length  between vertices 0 and 1
      auto& tri = triangles[tri_id];

      int texture_id = tri.tex_id;
       //calculate areas in texture space
       double tri_area = get_area_of_triangle(chart.all_triangle_new_coods_[tri_id][0], chart.all_triangle_new_coods_[tri_id][1], chart.all_triangle_new_coods_[tri_id][2]);
        //if we are calculating size of tris on output textures,pixel area is constant 
       double pixel_area = (1.0 / texture_dimensions.x) * (1.0 / texture_dimensions.y);
       double tri_pixels = tri_area / pixel_area;

      pixels += tri_pixels;
    }    

    double pixels_per_tri = pixels / chart.original_triangle_ids_.size();

    chart.real_to_tex_ratio_new = pixels_per_tri;
    
  }
}


}

bool is_output_texture_big_enough(std::map<uint32_t, std::map<uint32_t, chart>>& chart_map, double target_percentage_charts_with_enough_pixels){
  
  //check if at least a given percentage of charts have at least as many pixels now as they did in the original texture
  int charts_w_enough_pixels = 0;
  int num_charts = 0;

  for (auto& area_it : chart_map) {
  	for (auto& chart_it : chart_map[area_it.first]) {
  	  if (chart_it.second.all_triangle_ids_.size() > 0) {
  	  	++num_charts;
  	  
        if (chart_it.second.real_to_tex_ratio_new >= chart_it.second.real_to_tex_ratio_old){
          charts_w_enough_pixels++;
        }
      }
    }
  }

  const double percentage_big_enough = (double)charts_w_enough_pixels / (double)num_charts;

  //std::cout << "Percentage of charts big enough  = " << percentage_big_enough << std::endl;

  return (percentage_big_enough >= target_percentage_charts_with_enough_pixels);
}


static scm::math::vec2f project_to_plane(
  const scm::math::vec3f& v, scm::math::vec3f& plane_normal, 
  const scm::math::vec3f& centroid,
  const scm::math::vec3f& world_up) {

  scm::math::vec3f v_minus_p = v - centroid;

  auto plane_right = scm::math::cross(plane_normal, world_up);
  plane_right = scm::math::normalize(plane_right);
  auto plane_up = scm::math::cross(plane_normal, plane_right);
  plane_up = scm::math::normalize(plane_up);


  //project vertices to the plane
  scm::math::vec2f projected_v(
    scm::math::dot(v_minus_p, plane_right),
    scm::math::dot(v_minus_p, plane_up));

  return projected_v;

}

static void rotate_chart(chart& chart, const std::vector<lamure::mesh::Triangle_Chartid>& triangles) {
/*
  //compute min rect
  float optimal_angle = 0;

  float width = chart.rect_.max_.x - chart.rect_.min_.x;
  float height = chart.rect_.max_.y - chart.rect_.min_.y;
  float min_dim = width < height ? width : height;
  float min_area = width * height;


  for (float angle = 0; angle < 180.f; angle += 4.f) {
    
    rectangle candidate{
      scm::math::vec2f(std::numeric_limits<float>::max()), 
      scm::math::vec2f(std::numeric_limits<float>::lowest()), 
      chart.rect_.id_, false};

    float theta = 0.01745329f * angle;

    scm::math::mat4f rot = scm::math::make_rotation(angle, scm::math::vec3f(0, 0, 1));

    for (auto tri_id : chart.all_triangle_ids_) {
      for (uint32_t i = 0; i < 3; ++i) {
        auto tex = scm::math::vec4f(chart.all_triangle_new_coods_[tri_id][i].x,
          chart.all_triangle_new_coods_[tri_id][i].y, 0, 1);
        auto coord = rot * tex;

        candidate.min_.x = std::min(candidate.min_.x, coord.x);
        candidate.min_.y = std::min(candidate.min_.y, coord.y);

        candidate.max_.x = std::max(candidate.max_.x, coord.x);
        candidate.max_.y = std::max(candidate.max_.y, coord.y);

      }
    }

    width = candidate.max_.x - candidate.min_.x;
    height = candidate.max_.y - candidate.min_.y;
    float dim = width < height ? width : height;
    float area = width * height;

    //if (area < min_area) {
    if (dim < min_dim) {
      optimal_angle = angle;
      min_area = area;
      min_dim = dim;
      chart.rect_ = candidate;
    }

  }

  //apply best transform
  float theta = 0.01745329f * optimal_angle;

  scm::math::mat4f rot = scm::math::make_rotation(optimal_angle, scm::math::vec3f(0, 0, 1));

  for (auto tri_id : chart.all_triangle_ids_) {
    for (uint32_t i = 0; i < 3; ++i) {
      auto tex = scm::math::vec4f(chart.all_triangle_new_coods_[tri_id][i].x,
        chart.all_triangle_new_coods_[tri_id][i].y, 0, 1);
      auto coord = rot * tex;
      chart.all_triangle_new_coods_[tri_id][i] = coord;
    }
  }

*/

}

static void project_charts(std::map<uint32_t, chart>& chart_map, const std::vector<lamure::mesh::Triangle_Chartid>& triangles) {

  //keep a record of the longest chart edge
  float largest_max = 0.f;

  //iterate all charts
  for (auto& chart_it : chart_map) {
  	uint32_t chart_id = chart_it.first;
    chart& chart = chart_it.second;

    scm::math::vec3f avg_normal(0.f, 0.f, 0.f);
    scm::math::vec3f centroid(0.f, 0.f, 0.f);

    // compute average normal and centroid
    for (auto tri_id : chart.original_triangle_ids_) {

      auto& tri = triangles[tri_id];

      avg_normal += tri.v0_.nml_;
      avg_normal += tri.v1_.nml_;
      avg_normal += tri.v2_.nml_;

      centroid += tri.v0_.pos_;
      centroid += tri.v1_.pos_;
      centroid += tri.v2_.pos_;
    }

    avg_normal /= (float)(chart.original_triangle_ids_.size()*3);
    centroid /= (float)(chart.original_triangle_ids_.size()*3);

    avg_normal = scm::math::normalize(avg_normal);
    
    scm::math::vec3f world_up(0.f, 1.f, 0.f);
    if (std::abs(scm::math::dot(world_up, avg_normal)) > 0.8f) {
      world_up = scm::math::vec3f(0.f, 0.f, 1.f);
    }

    //record centroid, projection normal and world up for calculating inner triangle UVs later on
    chart.projection.proj_centroid = centroid;
    chart.projection.proj_normal = avg_normal;
    chart.projection.proj_world_up = world_up;

    //project all vertices into that plane
    for (auto tri_id : chart.all_triangle_ids_) {

      auto& tri = triangles[tri_id];

      scm::math::vec2f projected_v0 = project_to_plane(tri.v0_.pos_, avg_normal, centroid, world_up);
      scm::math::vec2f projected_v1 = project_to_plane(tri.v1_.pos_, avg_normal, centroid, world_up);
      scm::math::vec2f projected_v2 = project_to_plane(tri.v2_.pos_, avg_normal, centroid, world_up);

      
      std::vector<scm::math::vec2f> coords = {projected_v0, projected_v1, projected_v2};
      chart.all_triangle_new_coods_.insert(std::make_pair(tri_id, coords));

    }

    //compute rectangle for the current chart
    //initialize rectangle min and max
    chart.rect_.id_ = chart_id;
    chart.rect_.flipped_ = false;
    chart.rect_.min_ = scm::math::vec2f(std::numeric_limits<float>::max());
    chart.rect_.max_ = scm::math::vec2f(std::numeric_limits<float>::lowest());

    //compute the bounding rectangle for each chart
    for (auto tri_id : chart.all_triangle_ids_) {
      auto& tri = triangles[tri_id];

      for (uint32_t i = 0; i < 3; ++i) {

        chart.rect_.min_.x = std::min(chart.rect_.min_.x, chart.all_triangle_new_coods_[tri_id][i].x);
        chart.rect_.min_.y = std::min(chart.rect_.min_.y, chart.all_triangle_new_coods_[tri_id][i].y);

        chart.rect_.max_.x = std::max(chart.rect_.max_.x, chart.all_triangle_new_coods_[tri_id][i].x);
        chart.rect_.max_.y = std::max(chart.rect_.max_.y, chart.all_triangle_new_coods_[tri_id][i].y);

      }
    }

    //TODO save chart rotation angle per chart, then apply to interior LODs
    //rotate_chart(chart, triangles);


    //record offset for rendering from texture
    chart.projection.tex_coord_offset = chart.rect_.min_;

    //shift min to the origin
    chart.rect_.max_ -= chart.rect_.min_;

    //update largest chart
    largest_max = std::max(largest_max, chart.rect_.max_.x);
    largest_max = std::max(largest_max, chart.rect_.max_.y);


    // shift projected coordinates to min = 0
    for (auto tri_id : chart.all_triangle_ids_) {
      auto& tri = triangles[tri_id];

      chart.all_triangle_new_coods_[tri_id][0] -= chart.rect_.min_;
      chart.all_triangle_new_coods_[tri_id][1] -= chart.rect_.min_;
      chart.all_triangle_new_coods_[tri_id][2] -= chart.rect_.min_;

    }

    chart.rect_.min_ = scm::math::vec2f(0.f);

  }

  //Normalize charts but keep relative size
  for (auto& chart_it : chart_map) {
    chart& chart = chart_it.second;
    int32_t chart_id = chart_it.first;
    
    // normalize largest_max to 1
    for (auto tri_id : chart.all_triangle_ids_) {

      chart.all_triangle_new_coods_[tri_id][0] /= largest_max;
      chart.all_triangle_new_coods_[tri_id][1] /= largest_max;
      chart.all_triangle_new_coods_[tri_id][2] /= largest_max;

    }

    chart.rect_.max_ /= largest_max;
    chart.projection.largest_max = largest_max;

  }


}


//comparison function to sort the rectangles by height 
bool sort_by_height (rectangle i, rectangle j){
  bool i_smaller_j = false;
  float height_of_i = i.max_.y-i.min_.y;
  float height_of_j = j.max_.y-j.min_.y;
  if (height_of_i > height_of_j) {
    i_smaller_j = true;
  }
  return i_smaller_j;
}

rectangle pack(std::vector<rectangle>& input, float scale_factor = 0.9f) {

  if (input.size() == 0) {
    std::cout << "ERROR: no rectangles received in pack() function" << std::endl; 
    exit(1);
  }

  std::vector<rectangle> starting_rects = input;

  //make sure all rectangles stand on the shorter side
  for(uint32_t i=0; i< input.size(); i++){
    auto& rect=input[i];
    if ((rect.max_.x-rect.min_.x) > (rect.max_.y - rect.min_.y)){
      float temp = rect.max_.y;
      rect.max_.y=rect.max_.x;
      rect.max_.x=temp;
      rect.flipped_ = true;
    }
  }

  //sort by height
  std::sort(input.begin(), input.end(), sort_by_height);

  //calc the size of the texture
  float dim = sqrtf(input.size());
  dim = std::ceil(dim);


  //get the largest rect
  float max_height = input[0].max_.y-input[0].min_.y;

  //compute the average height of all rects
  float sum = 0.f;
  for (uint32_t i=0; i<input.size(); i++){

    float height = input[i].max_.y-input[i].min_.y;
    if (height < 0){
      std::cout << "ERROR: rect [" << i << "] invalid height in pack(): " << height << std::endl;
      exit(1);
    }
    sum+= (height); 
  }

  float average_height = sum/((float)input.size());


  if (average_height <= 0)
  {
    std::cout << "ERROR: average height error" << std::endl; 
    exit(1);
  }

  //heuristically adjust texture size
  dim *= scale_factor;
  
  rectangle texture{scm::math::vec2f(0,0), scm::math::vec2f((int)((dim)*average_height),(int)((dim)*average_height)),0};

  //pack the rects
  int offset_x =0;
  int offset_y =0;
  float highest_of_current_line = input[0].max_.y-input[0].min_.y;
  for(int i=0; i< input.size(); i++){
    auto& rect = input[i];
    if ((offset_x+ (rect.max_.x - rect.min_.x)) > texture.max_.x){

      offset_y += highest_of_current_line;
      offset_x =0;
      highest_of_current_line = rect.max_.y - rect.min_.y;
      
    }
    
    rect.max_.x= offset_x + (rect.max_.x - rect.min_.x);
    rect.min_.x= offset_x;
    offset_x+= rect.max_.x - rect.min_.x;

    rect.max_.y = offset_y + (rect.max_.y - rect.min_.y);
    rect.min_.y = offset_y;
  
    if (rect.max_.y > texture.max_.y)
    {
      //recursive call until all rectangles fit on texture
      //std::cout << "Repacking....(" << scale_factor*1.1 << ")\n";
      rectangle rtn_tex_rect = pack(starting_rects, scale_factor * 1.1);
      input = starting_rects;

      return rtn_tex_rect;

    }

  }


  return texture;
}




#endif