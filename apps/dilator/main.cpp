// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr



//external relative
#include "GuillotineBinPack.h"
#include "MaxRectsBinPack.h"
#include "GuillotineBinPack.h"

//external std search paths
#include <dirent.h>
#include <FreeImage.h>
#include <sys/types.h>

//c standard headers
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <list>
#include <string>
#include <vector>


using namespace std;
typedef std::vector<std::string> stringvec;

char* get_cmd_option(char** begin, char** end, const std::string & option) {
    char** it = std::find(begin, end, option);
    if (it != end && ++it != end)
        return *it;
    return 0;
}

bool cmd_option_exists(char** begin, char** end, const std::string& option) {
    return std::find(begin, end, option) != end;
}

//the pixel structure
typedef struct {
    BYTE r, g, b;
} pixel;

//the global structure
typedef struct {
    pixel *data;
    int w, h;
    //float scale;
} glob;
glob global; //buffer work is performed on and wthat is displayed

int copyToBuffer = 0;

//read image
pixel *read_img(char const *name, int *width, int *height, float scale) {
    FIBITMAP *image;
    int i,j,pnum;
    RGBQUAD aPixel;
    pixel *data;
    
    FREE_IMAGE_FORMAT image_format = FreeImage_GetFileType(name, 0);

    if((image = FreeImage_Load(image_format, name, 0)) == NULL) {
        return NULL;
    }
    *width = ((float)FreeImage_GetWidth(image)) * scale;
    *height = ((float)FreeImage_GetHeight(image)) * scale;

    if (scale < 1.0 && scale > 0.0) {
      image = FreeImage_Rescale(image, *width, *height);
    }

    data = (pixel *)malloc((*height)*(*width)*sizeof(pixel *));
    pnum=0;
    
    for(i = 0 ; i < (*height) ; i++) {
        for(j = 0 ; j < (*width) ; j++) {
            FreeImage_GetPixelColor(image, j, i, &aPixel);
            data[pnum].r = (aPixel.rgbRed);
            data[pnum].g = (aPixel.rgbGreen);
            data[pnum++].b = (aPixel.rgbBlue);
        }
    }
    FreeImage_Unload(image);
    return data;
}//read_img

//write_img
void write_img(char const *name, pixel *data, int width, int height) {
    FIBITMAP *image;
    RGBQUAD aPixel;
    int i,j;
    
    image = FreeImage_Allocate(width, height, 24, 0, 0, 0);
    if(!image) {
        perror("FreeImage_Allocate");
        return;
    }
    for(i = 0 ; i < height ; i++) {
        for(j = 0 ; j < width ; j++) {
            aPixel.rgbRed = data[i*width+j].r;
            aPixel.rgbGreen = data[i*width+j].g;
            aPixel.rgbBlue = data[i*width+j].b;
            FreeImage_SetPixelColor(image, j, i, &aPixel);
        }
    }
    if(!FreeImage_Save(FIF_JPEG, image, name, 0)) {
        perror("FreeImage_Save");
    }
    FreeImage_Unload(image);
}//write_img


int getdir (string dir, vector<string> &files){
    DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(dir.c_str())) == NULL) {
        cout << "Error(" << errno << ") opening " << dir << endl;
        return errno;
    }

    while ((dirp = readdir(dp)) != NULL) {
        if (strcmp(string(dirp->d_name).c_str(), ".") != 0 && strcmp(string(dirp->d_name).c_str(), "..") != 0 ){
            std::string name = std::string(dirp->d_name);
            if ((name.substr(name.size()-3)).compare("jpg") == 0 || 
                (name.substr(name.size()-3)).compare("png") == 0 ||
                (name.substr(name.size()-3)).compare("tif") == 0) {
              files.push_back(string(dirp->d_name));
            }
        }
    }
    closedir(dp);
    return 0;
}

int main(int argc, char *argv[]) {
    using namespace rbp;
    vector<glob> pixelInfo = vector<glob>();
    vector<int> widthVector = vector<int>();
    vector<int> heightVector = vector<int>();

    // For taking command line arguments from terminal
    
    if (argc == 1 || 
      cmd_option_exists(argv, argv+argc, "-h") ||
      !cmd_option_exists(argv, argv+argc, "-d") ||
      !cmd_option_exists(argv, argv+argc, "-f")) {
      std::cout << "Usage: " << argv[0] << " -f <input_file> -o <output_file.jpg>" << std::endl;
      return 0;
    }
    
    std::string input_file = get_cmd_option(argv, argv+argc, "-f");
    std::string output_filename = get_cmd_option(argv, argv+argc, "-o");


    std::cout << "loading image..." << std::endl;


    const float scale = 1.0;
    global.data = read_img(input_file.c_str(), &global.w, &global.h, scale);
    std::cout << "image " << ": "  << global.w << " " << global.h << " " << std::endl;        

    


    FIBITMAP *image;
    RGBQUAD aPixel;
    
    std::cout << "writing out texture..." << std::endl;
    image = FreeImage_Allocate(global.w, global.h, 24, 0, 0, 0);

    for (int y = 0; y < global.h; ++y) {
        for (int x = 0; x < global.w; ++x) {
            aPixel.rgbRed = global.data[y * global.w + x].r;
            aPixel.rgbGreen = global.data[y * global.w + x].g;
            aPixel.rgbBlue = global.data[y * global.w + x].b;
            FreeImage_SetPixelColor(image, x, y, &aPixel);
        }
    }
    
    if(!FreeImage_Save(FIF_PNG, image, output_filename.c_str(), 0)) {
        perror("FreeImage_Save");
    }

    std::cout << "Image saved to " << output_filename << std::endl;
    
    FreeImage_Unload(image);


    return 0;

}



