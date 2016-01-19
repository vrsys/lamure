// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr



#include "image_based_error_app.h"


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

    lamure::app::image_based_error_app::initialize();
    lamure::app::image_based_error_app::compute_image_differences(image_path_1, 
                                                     image_path_2, 
                                                     out_image_path);
    lamure::app::image_based_error_app::deinitialize();
    
	return 0;
}



