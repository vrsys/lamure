
#include <iostream>
#include <memory>
#include <vector>
#include <scm/core/math.h>
#include <cmath>
#include <algorithm>


#include "lodepng.h"

struct rectangle{
	scm::math::vec2f min_;
	scm::math::vec2f max_;
	int id_;

};


void save_image(std::string filename, std::vector<uint8_t> image, int width, int height) {
  int texerror = lodepng::encode(filename, image, width, height);
  if (texerror) {
    std::cout << "unable to save image file " << filename << std::endl;
  }
  std::cout << "image " << filename << " saved" << std::endl;

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

//entry point
int main(int argc, char **argv) {

  std::vector<rectangle> input;

  for(int i=0; i<9; i++){
    float rand_x = (int)((float)rand()/(float)RAND_MAX)*8+i*6;
    float rand_y = (int)((float)rand()/(float)RAND_MAX)*8+i*5;
    rectangle rect{scm::math::vec2f(0, 0), scm::math::vec2f(rand_x, rand_y), i};
    input.push_back(rect);
  }
  input.push_back(rectangle{scm::math::vec2f(0, 0), scm::math::vec2f(50, 50), 24});
  
  //make sure all rectangles stand on the shorter side
  for(int i=0; i< input.size(); i++){
    auto& rect=input[i];
    if ((rect.max_.x-rect.min_.x) > (rect.max_.y - rect.min_.y)){
      float temp= rect.max_.y;
      rect.max_.y=rect.max_.x;
      rect.max_.x=temp;
    }
  }

  //sort by height
  std::sort (input.begin(),input.end(),sort_by_height);

  //calc the size of the texture
  float dim = sqrtf(input.size());
  std::cout << dim << " -> " << std::ceil(dim) << std::endl;
  dim = std::ceil(dim);


  //get the largest rect
  std::cout << input[0].max_.y-input[0].min_.y << std::endl;
  float max_height = input[0].max_.y-input[0].min_.y;

  //compute the average height of all rects
  float sum = 0.f;
  for (int i=0; i<input.size(); i++){
    sum+= input[i].max_.y-input[i].min_.y; 
  }
  float average_height = sum/((float)input.size());

  //heuristically take half
  dim *= 0.9f;
  

  rectangle texture{scm::math::vec2f(0,0), scm::math::vec2f((int)((dim+1)*average_height),(int)((dim+1)*average_height)),0};

  std::cout << "texture size: " << texture.max_.x << " " << texture.max_.y<< std::endl;



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
	


  }

  //done

  //print the result
  for (int i=0; i< input.size(); i++){
  	auto& rect = input[i];
  	std::cout<< "rectangle["<< rect.id_<< "]"<<"  min("<< rect.min_.x<<" ,"<< rect.min_.y<<")"<<std::endl;
  	std::cout<< "rectangle["<< rect.id_<< "]"<<"  max_("<< rect.max_.x<< " ,"<<rect.max_.y<<")"<<std::endl;
  }



/*
  struct color {
    unsigned char r_;
    unsigned char g_;
    unsigned char b_;
    unsigned char a_;
  };*/
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

  save_image("texture.png", image, texture.max_.x, texture.max_.y);

  return 0;
}

