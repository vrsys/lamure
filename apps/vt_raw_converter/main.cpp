// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>
#include <string>
#include <list>
#include <FreeImage.h>



char* get_cmd_option(char** begin, char** end, const std::string & option) {
    char** it = std::find(begin, end, option);
    if (it != end && ++it != end)
        return *it;
    return 0;
}

bool cmd_option_exists(char** begin, char** end, const std::string& option) {
    return std::find(begin, end, option) != end;
}

struct pixel {
  BYTE r, g, b;
};

struct image {
  unsigned int w, h;
  pixel *data;  
};

image image_;

//read image
pixel *read_img(char const *name, unsigned int *width, unsigned int *height, float scale) {
    FIBITMAP *image;
    int i,j,pnum;
    RGBQUAD aPixel;
    pixel *data;
    
    FREE_IMAGE_FORMAT image_format = FreeImage_GetFileType(name,0);

    if((image = FreeImage_Load(image_format, name, 0)) == NULL) {
        return NULL;
    }

    image = FreeImage_Rescale(image, FreeImage_GetWidth(image)*scale, FreeImage_GetHeight(image)*scale);

    *width = FreeImage_GetWidth(image);
    *height = FreeImage_GetHeight(image);
    data = (pixel *)malloc((*height)*(*width)*sizeof(pixel *));
    pnum=0;
    
    for(i = 0 ; i < (*height) ; i++) {
        for(j = 0 ; j < (*width) ; j++) {
            FreeImage_GetPixelColor(image, j, (*height) - i, &aPixel);
            data[pnum].r = (aPixel.rgbRed);
            data[pnum].g = (aPixel.rgbGreen);
            data[pnum].b = (aPixel.rgbBlue);
            pnum++;
        }
    }
    FreeImage_Unload(image);
    return data;
}

int main(int argc, char *argv[]) {
    

  if (argc == 1 || 
    !cmd_option_exists(argv, argv+argc, "-f") ||
    !cmd_option_exists(argv, argv+argc, "-s")) {
    std::cout << "Usage: " << argv[0] << " -f <input_image> -s <scale>" << std::endl <<
     "INFO: argv[0] " << std::endl ;
    return 0;
  }

  std::string input_file = get_cmd_option(argv, argv+argc, "-f");
  float scale = atof(get_cmd_option(argv, argv+argc, "-s"));

  std::cout << "loading image..." << std::endl;
  std::cout << "Path: " << input_file << std::endl;

  unsigned int width, height;
  auto data = read_img(input_file.c_str(), &width, &height, scale);

  std::cout << "width: "  << width << ", " 
            << "height: " << height << std::endl;

  std::string output_file = input_file.substr(0, input_file.size()-4) + "_w" + std::to_string(width) + "_h" + std::to_string(height) + ".data";
  std::cout << output_file << std::endl;


  std::cout << "writing raw..." << std::endl;

  std::ofstream raw_file;
  raw_file.open(output_file.c_str(), std::ios::trunc | std::ios::binary);

  raw_file.write((char*)data, width*height*sizeof(char)*3);

  raw_file.close();

  std::cout << "done" << std::endl;

  return 0;

}



