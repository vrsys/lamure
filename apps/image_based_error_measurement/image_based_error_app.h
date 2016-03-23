// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef IMAGE_BASED_ERROR_APP_H
#define IMAGE_BASED_ERROR_APP_H

#include <lamure/qm/color_converter.h>

#include <FreeImagePlus.h>
#include <iostream>

#include <limits.h>

#include <boost/filesystem.hpp>

#include <cfloat>

#include <fstream>
#include <sstream>

namespace lamure{
namespace app {

class image_based_error_app {

public:

	static void initialize();
	static void deinitialize();

	static bool compute_delta_E(std::string const& image_path_1, 
						  		std::string const& image_path_2,
						  		std::string const& normal_image_path_1,
						  		std::string const& normal_image_path_2,
						  		std::string const& out_image_path);

	static bool compute_normal_deviation(std::string const& image_path_1, 
						  				 std::string const& image_path_2, 
						  				 std::string const& out_image_path);
/*
    static bool compute_image_based_overlap(std::string const& image_path_1, 
						  				 	std::string const& image_path_2, 
						  					std::string const& out_image_path);
*/
private:

	static bool load_image_to_memory(std::string const& image_path, 
						  		 	 BYTE*&   out_image_data,
						  		 	 int32_t& out_image_width,
						  		 	 int32_t& out_image_height);


	static void free_image_data(BYTE* raw_image_data);

	static double map_range(double value, 
			  				double old_min, double old_max, 
			  				double new_min, double new_max);

	static double calc_red(double percentage);
	static double calc_green(double percentage);
	static double calc_blue(double percentage);

	static void color_pixel(double percentage, BYTE* pixel);
	static unsigned char adjust(const double color, const double factor);
	static void wavelength_to_RGB(const double wavelength, 
								  unsigned char& R, unsigned char& G, unsigned char& B);

	static double get_wavelength_from_data_point(double value, 
												 double min_value, double max_value);

	static void write_delta_E_image(double* dE_arr, 
									unsigned int im_w, unsigned im_h, 
									double min_err, double max_err, 
									unsigned int im_num, std::string const& out_file_path);

	static void 
	calc_pixel_color(BYTE* pixel, 
					 double value, 
					 double min_value, double max_value);

	static void
	write_heatmap_key(unsigned int width_in_px, unsigned int height_in_px, 
					  std::string const& path);

    static std::string out_image_path_; // argv[3];

    static int32_t image_width_1_;// = -1;
    static int32_t image_height_1_;// = -1;

    static BYTE* image_data_1_;// = nullptr;
    static BYTE* image_data_2_;// = nullptr;
};

}
}

#endif