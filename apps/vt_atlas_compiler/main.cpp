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
      !cmd_option_exists(argv, argv+argc, "-o") ||
      !cmd_option_exists(argv, argv+argc, "-s") ) {
      std::cout << "Usage: " << argv[0] << " -d <input_dir/> -s <scale> -o <output_file.jpg>" << std::endl <<
         "INFO: argv[0] " << std::endl ;
      return 0;
    }
    
    std::string dir = get_cmd_option(argv, argv+argc, "-d");
    float scale = atof(get_cmd_option(argv, argv+argc, "-s"));
    std::string output_filename = get_cmd_option(argv, argv+argc, "-o");

    if(dir[dir.size() - 1] != '/'){
        dir+="/";
    }

    vector<string> files = vector<string>();
    getdir(dir,files);
    std::sort(files.begin(), files.end());


    int squareRootY = 0;
    int squareRootX = 0;
    double temp = sqrt(files.size());
    //squareRootY = ceil(temp);
    squareRootY = floor(temp);
    squareRootX = squareRootY;
    double decpart = temp - squareRootY;

    if(fmod(temp, squareRootY) != 0 ){
        squareRootY = floor(temp) + 1;
    }

    if(decpart > 0.5){
        squareRootX = floor(temp) + 1;
    }

    std::cout << "loading images..." << std::endl;

    std::vector<bool> rotate;

    for (int i=0; i<files.size(); i++) {
        string s1 = dir.c_str() + files[i];
        global.data = read_img(s1.c_str(), &global.w, &global.h, scale);
        pixelInfo.push_back(global);
        heightVector.push_back((int)global.h);
        widthVector.push_back((int)global.w);
        bool height_larger_width = global.h > global.w;
        rotate.push_back(height_larger_width);
        std::cout << "image " << i << ": "  << global.w << " " << global.h << " " << height_larger_width << std::endl;
        
    }


    std::cout << pixelInfo.size() << " images" << std::endl;
    
    int big1 = (int)heightVector[0];
    int big2 = (int)widthVector[0];
    
    for (int i=0; i<heightVector.size(); i++) {
        if(heightVector[i] > big1) // Compare biggest value with current element
        {
            big1 = heightVector[i];
        }
        if(widthVector[i] > big2) // Compare biggest value with current element
        {
            big2 = widthVector[i];
        }
    }

    int maxFrameW = squareRootX * big2;
    int maxFrameH = squareRootY * big1;

    // Create a bin to pack to, use the bin size from command line.
    MaxRectsBinPack bin;
    GuillotineBinPack gBin;
    
    bin.Init(maxFrameW, maxFrameH, false);
    
    vector<Rect> PackedRectangle = vector<Rect>(files.size());

    std::string log = "";
    log += std::to_string(maxFrameW) + ",";
    log += std::to_string(maxFrameH) + "\n";

    std::cout << log << std::endl;
    std::cout << "packing..." << std::endl;
    
    for(int i = 0; i < files.size(); i++){
        // Read next rectangle to pack.
        int rectWidth = pixelInfo[i].w;
        int rectHeight = pixelInfo[i].h;

        // Perform the packing.
        MaxRectsBinPack::FreeRectChoiceHeuristic heuristic = MaxRectsBinPack::RectBottomLeftRule; // This can be changed individually even for each rectangle packed.
        Rect packedRect = bin.Insert(rectWidth, rectHeight, heuristic);

        // Test success or failure.
        if (packedRect.height > 0){
            PackedRectangle[i] = packedRect;
        }
        else
            printf("Failed! Could not find a proper position to pack this rectangle into. Skipping this one.\n");

#if 1
        log += std::to_string(i) + ",";
        log += std::to_string(packedRect.x) + ",";
        log += std::to_string(packedRect.y) + ",";
        log += std::to_string(packedRect.width) + ",";
        log += std::to_string(packedRect.height) + ",";
        log += std::to_string(rotate[i]) + ",";
        log += files[i] + "\n";
#else
        log += std::to_string(i) + ",";
        log += std::to_string(packedRect.x) + ",";
        log += std::to_string(packedRect.y) + ",";
        log += std::to_string(packedRect.width) + ",";
        log += std::to_string(packedRect.height) + ",";
        log += std::to_string(rotate[i]) + ",";
        std::string filename = std::to_string(i);
        while (filename.size() < 8) {
            filename = "0" + filename;
        }
        log += filename + ".jpg\n";
#endif


    }
    std::cout << "packing done" << std::endl;
    //std::cout << log << std::endl;

    
    std::cout << "writing atlas..." << std::endl;

    size_t offset_startX = 0;
    size_t offset_startY = 0;
    
#if 0


    FIBITMAP *image;
    RGBQUAD aPixel;

    image = FreeImage_Allocate(maxFrameW, maxFrameH, 24, 0, 0, 0);

    for (int k=0; k<pixelInfo.size(); k++) {
        if (k!=0) {
            offset_startX = PackedRectangle[k].x;
            offset_startY = PackedRectangle[k].y;
        }
        for (int y = 0; y < pixelInfo[k].h; ++y) {
            for (int x = 0; x < pixelInfo[k].w; ++x) {
                aPixel.rgbRed = pixelInfo[k].data[y * pixelInfo[k].w + x].r;
                aPixel.rgbGreen = pixelInfo[k].data[y * pixelInfo[k].w + x].g;
                aPixel.rgbBlue = pixelInfo[k].data[y * pixelInfo[k].w + x].b;
                FreeImage_SetPixelColor(image, x+offset_startX, y+offset_startY, &aPixel);
            }
        }
    }
    
    if(!FreeImage_Save(FIF_JPEG, image, output_filename.c_str(), 0)) {
        perror("FreeImage_Save");
    }

    std::cout << "Image saved to " << output_filename << std::endl;

    FreeImage_Unload(image);

#else

    std::vector<uint8_t> pixels;

    //concatenate all area images to one big texture
    for (size_t k=0; k<pixelInfo.size(); k++) {
        if (k!=0) {
            offset_startX = PackedRectangle[k].x;
            offset_startY = PackedRectangle[k].y;
        }
        std::cout << "k = " << k << std::endl;
        for (size_t y = 0; y < pixelInfo[k].h; ++y) {
            for (size_t x = 0; x < pixelInfo[k].w; ++x) {
                uint8_t r = pixelInfo[k].data[y * pixelInfo[k].w + x].r;
                uint8_t g = pixelInfo[k].data[y * pixelInfo[k].w + x].g;
                uint8_t b = pixelInfo[k].data[y * pixelInfo[k].w + x].b;

                //allocation and deletion is incremental to allow bigger images
                while (pixels.size() < (3UL * ((x+offset_startX) + maxFrameW*(y+offset_startY)) + 3UL)) {
                    pixels.push_back((uint8_t)0);
                }

                pixels[3UL * ((x+offset_startX) + maxFrameW*(y+offset_startY))] = r;
                pixels[3UL * ((x+offset_startX) + maxFrameW*(y+offset_startY)) + 1UL] = g;
                pixels[3UL * ((x+offset_startX) + maxFrameW*(y+offset_startY)) + 2UL] = b;

            }
        }

        //allocation and deletion is incremental to allow bigger images
        if (pixelInfo[k].data) {
          free(pixelInfo[k].data);
          pixelInfo[k].data = nullptr;
        }

    }


    size_t size = pixels.size();
    pixels.clear();

    std::cout << "current size: " << size << std::endl;

    size_t filled_size = (3UL * maxFrameW * maxFrameH);

    std::cout << "filled size: " << filled_size << std::endl;

    std::cout << "Filling empty space..." << std::endl;

    while (size < filled_size) {
      pixels.push_back((uint8_t)0);
      ++size;
      if (size % 1000 == 0) {
        //std::cout << (double)(size) / (double)(3 * maxFrameW * maxFrameH) << std::endl;
        std::cout << "TODO: " << (filled_size - size) << std::endl;
      }
    }

    std::cout << "Invert image..." << std::endl;

    //read one line at a time
    int64_t one_line = (int64_t)(maxFrameW * 3UL);

    std::vector<uint8_t> inv_pixels;

    for (int64_t y = maxFrameH-1; y >= 0; --y) {
        
      std::cout << "Y = " << y << std::endl;
      int64_t begin = y * one_line;
      for (int64_t x = 0; x < one_line; ++x) {
        inv_pixels.push_back(pixels[begin + x]);
      }

    }

    pixels.clear();

    std::cout << "Atlas packing complete" << endl;
    
    std::cout << "Writing raw file..." << std::endl;


    std::ofstream raw_file;
    std::string image_filename =
        output_filename.substr(0, output_filename.size() - 4) + "_rgb_w" + std::to_string(maxFrameW) + "_h" + std::to_string(maxFrameH) + ".data";
    raw_file.open(image_filename, std::ios::out | std::ios::trunc | std::ios::binary);

    raw_file.write((char*)&inv_pixels[0], inv_pixels.size());

    std::cout << "Closing file..." << std::endl;

    raw_file.close();

    std::cout << "Image saved to " << image_filename << std::endl;

#endif

    std::string log_filename = output_filename.substr(0, output_filename.size()-4)+".log";

    std::ofstream log_file(log_filename.c_str(), std::ios::out | std::ios::trunc);
    log_file << log;
    log_file.close();

    std::cout << "log file written" << std::endl;

    return 0;

}



