// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr



#include "image_based_error_app.h"

bool replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

int main (int argc, char** argv)
{

    
	if(argc < 3) {
        std::cout<<  "Usage: " << argv[0] << 
                     "input_image_1.png input_image_2.png output_file.png" << std::endl;
        return -1;
	}


    std::string image_path_1   = argv[1];
    std::string image_path_2   = argv[2];
    std::string out_image_path = argv[3];

    std::string normal_image_path_1 = image_path_1;
    std::string normal_image_path_2 = image_path_2;
    replace(normal_image_path_1, "color", "normal");
    replace(normal_image_path_2, "color", "normal");


    lamure::app::image_based_error_app::initialize();
    lamure::app::image_based_error_app::compute_delta_E(image_path_1, 
                                                        image_path_2,
                                                        normal_image_path_1,
                                                        normal_image_path_2, 
                                                        out_image_path);


/*
    lamure::app::image_based_error_app::compute_normal_deviation(image_path_1, 
                                                                 image_path_2, 
                                                                 out_image_path);
*/
/*
    lamure::app::image_based_error_app::compute_image_based_overlap(image_path_1, 
                                                                    image_path_2, 
                                                                    out_image_path);
*/
    lamure::app::image_based_error_app::deinitialize();
    
	return 0;
}



