#ifndef CHART_PACKING_H_
#define CHART_PACKING_H_


#include <lamure/mesh/triangle_chartid.h>

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

static void project_charts(std::map<uint32_t, chart>& chart_map, const std::vector<lamure::mesh::Triangle_Chartid>& triangles) {

  //keep a record of the largest chart edge
  float largest_max = 0.f;

  //std::cout << "Projecting to plane:\n";

  //iterate all charts
  //for (uint32_t chart_id = 0; chart_id < charts.size(); ++chart_id) {
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

      chart.rect_.min_.x = std::min(chart.rect_.min_.x, chart.all_triangle_new_coods_[tri_id][0].x);
      chart.rect_.min_.y = std::min(chart.rect_.min_.y, chart.all_triangle_new_coods_[tri_id][0].y);
      chart.rect_.min_.x = std::min(chart.rect_.min_.x, chart.all_triangle_new_coods_[tri_id][1].x);
      chart.rect_.min_.y = std::min(chart.rect_.min_.y, chart.all_triangle_new_coods_[tri_id][1].y);
      chart.rect_.min_.x = std::min(chart.rect_.min_.x, chart.all_triangle_new_coods_[tri_id][2].x);
      chart.rect_.min_.y = std::min(chart.rect_.min_.y, chart.all_triangle_new_coods_[tri_id][2].y);

      chart.rect_.max_.x = std::max(chart.rect_.max_.x, chart.all_triangle_new_coods_[tri_id][0].x);
      chart.rect_.max_.y = std::max(chart.rect_.max_.y, chart.all_triangle_new_coods_[tri_id][0].y);
      chart.rect_.max_.x = std::max(chart.rect_.max_.x, chart.all_triangle_new_coods_[tri_id][1].x);
      chart.rect_.max_.y = std::max(chart.rect_.max_.y, chart.all_triangle_new_coods_[tri_id][1].y);
      chart.rect_.max_.x = std::max(chart.rect_.max_.x, chart.all_triangle_new_coods_[tri_id][2].x);
      chart.rect_.max_.y = std::max(chart.rect_.max_.y, chart.all_triangle_new_coods_[tri_id][2].y);
    }

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

  //std::cout << "largest max: " << largest_max << std::endl;

  //std::cout << "Normalize charts but keep relative size:\n";
  for (auto& chart_it : chart_map) {
    chart& chart = chart_it.second;
    int32_t chart_id = chart_it.first;
    
    // normalize largest_max to 1
    for (auto tri_id : chart.all_triangle_ids_) {
      //triangle& tri = triangles[tri_id];

      chart.all_triangle_new_coods_[tri_id][0] /= largest_max;
      chart.all_triangle_new_coods_[tri_id][1] /= largest_max;
      chart.all_triangle_new_coods_[tri_id][2] /= largest_max;

    }

    chart.rect_.max_ /= largest_max;
    chart.projection.largest_max = largest_max;

    //std::cout << "chart " << chart_id << " rect min: " << chart.rect_.min_.x << ", " << chart.rect_.min_.y << std::endl;
    //std::cout << "chart " << chart_id << " rect max: " << chart.rect_.max_.x << ", " << chart.rect_.max_.y << std::endl;
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
  //std::cout << dim << " -> " << std::ceil(dim) << std::endl;
  dim = std::ceil(dim);


  //get the largest rect
  //std::cout << input[0].max_.y-input[0].min_.y << std::endl;
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

  //heuristically take half
  dim *= scale_factor;
  
  rectangle texture{scm::math::vec2f(0,0), scm::math::vec2f((int)((dim)*average_height),(int)((dim)*average_height)),0};

  //std::cout << "texture size: " << texture.max_.x << " x " << texture.max_.y<< std::endl;

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
      //std::cout << "rect max y = " << rect.max_.y << ", tex max y = " << texture.max_.y << std::endl;
      //std::cout << "Rect " << i << "(" << rect.max_.x - rect.min_.x << " x " << rect.max_.y - rect.min_.y << ") doesn't fit on texture\n";

      //recursive call until all rectangles fir on texture
      //std::cout << "Repacking....(" << scale_factor*1.1 << ")\n";
      rectangle rtn_tex_rect = pack(starting_rects, scale_factor * 1.1);
      input = starting_rects;

      return rtn_tex_rect;

    }

  }

  //done

#if 0

  //print the result
  //for (int i=0; i< input.size(); i++){
  //  auto& rect = input[i];
  //  std::cout<< "rectangle["<< rect.id_<< "]"<<"  min("<< rect.min_.x<<" ,"<< rect.min_.y<<")"<<std::endl;
  //  std::cout<< "rectangle["<< rect.id_<< "]"<<"  max("<< rect.max_.x<< " ,"<<rect.max_.y<<")"<<std::endl;
  //}

  //output test image for rectangle packing
  std::vector<unsigned char> image;
  image.resize(texture.max_.x*texture.max_.y*4);
  for (int i = 0; i < image.size()/4; ++i) {
    image[i*4+0] = 255;
    image[i*4+1] = 0;
    image[i*4+2] = 0;
    image[i*4+3] = 255;
  }
  for (int i=0; i< input.size(); i++){
    auto& rect = input[i];
    int color = (255/input.size())*rect.id_;
    for (int x = rect.min_.x; x < rect.max_.x; x++) {
      for (int y = rect.min_.y; y < rect.max_.y; ++y) {
        int pixel = (x + texture.max_.x*y) * 4;
           image[pixel] = (char)color;
           image[pixel+1] = (char)color;
           image[pixel+2] = (char)color;
           image[pixel+3] = (char)255;
      }
    }
  }
  save_image("data/mesh_prepro_texture.png", image, texture.max_.x, texture.max_.y);

#endif

  return texture;
}




#endif