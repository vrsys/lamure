// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include "image_based_error_app.h"

using cc = lamure::qm::color_converter;

namespace lamure{
namespace app {

std::string image_based_error_app::out_image_path_ = ""; // argv[3];

int32_t image_based_error_app::
image_width_1_ = -1;
int32_t image_based_error_app::
image_height_1_ = -1;

BYTE* image_based_error_app::
image_data_1_ = nullptr;

BYTE* image_based_error_app::
image_data_2_ = nullptr;

void image_based_error_app::
initialize() {
	FreeImage_Initialise();
}

void image_based_error_app::
deinitialize() {
	FreeImage_DeInitialise();
}


void image_based_error_app::
free_image_data(BYTE* raw_image_data) {

    if(raw_image_data != 0) {
        free(raw_image_data);
    }
}


double image_based_error_app::
map_range(double value, 
		  double old_min, double old_max, 
		  double new_min, double new_max) {
    if(old_max == old_min)
        return new_min;
    else
        return (((value - old_min) * (new_max - new_min)) / (old_max - old_min)) + new_min;
}

double image_based_error_app::
calc_green(double percentage) {
    return 255.0 * (percentage > 0.5 ? 0 : 0.25 - std::abs(0.25 - percentage) ) /0.25;
}

double image_based_error_app::
calc_blue(double percentage) {
    return 255.0 * (percentage < 0.25 || percentage > 0.75 ? 0 : 0.25 - std::abs(0.50 - percentage) ) /0.25;
}

double image_based_error_app::
calc_red(double percentage) {
    return 255.0 * (percentage < 0.5 ? 0 : ( (percentage > 0.75) ?  0.25 : 0.25 - std::abs(0.75 - percentage) )  ) /0.25;
}


void image_based_error_app::
color_pixel(double percentage, BYTE* pixel) {
    pixel[0] = calc_green(percentage);
    pixel[1] = calc_blue(percentage);
    pixel[2] = calc_red(percentage);
}



	double  color_gamma        =   0.80;
    double const intensity_max = 255;

	double round(double d)
	{
		return floor(d + 0.5);
	}

unsigned char image_based_error_app::
adjust(const double color, const double factor){
	if (color == 0.0){
		return 0;
	}
	else{
		int res = round(intensity_max * std::pow(color * factor, color_gamma));
		return std::min(255, std::max(0, res));
	}
}

void image_based_error_app::
wavelength_to_RGB(const double wavelength, unsigned char& R, unsigned char& G, unsigned char& B)
{

    double factor = std::numeric_limits<double>::lowest();

	double blue = std::numeric_limits<double>::lowest();
	double green = std::numeric_limits<double>::lowest();
	double red = std::numeric_limits<double>::lowest();

	if(380 <= wavelength && wavelength <= 440){
        red   = -(wavelength - 440) / (440 - 380);
        green = 0.0;
		blue  = 1.0;
	}
	else if(440 < wavelength && wavelength <= 490){
		red   = 0.0;
		green = (wavelength - 440) / (490 - 440);
		blue  = 1.0;
	}
	else if(490 < wavelength && wavelength <= 510){
		red   = 0.0;
		green = 1.0;
		blue  = -(wavelength - 510) / (510 - 490);
	}
	else if(510 < wavelength && wavelength <= 580){
		red   = (wavelength - 510) / (580 - 510);
		green = 1.0;
		blue  = 0.0;
	}
	else if(580 < wavelength && wavelength <= 645){
		red   = 1.0;
		green = -(wavelength - 645) / (645 - 580);
		blue  = 0.0;
	}
	else if(645 < wavelength && wavelength <= 780){
		red   = 1.0;
		green = 0.0;
		blue  = 0.0;
	}
	else{
		red   = 0.0;
		green = 0.0;
		blue  = 0.0;
	}


	if(380 <= wavelength && wavelength <= 420){
		factor = 0.3 + 0.7*(wavelength - 380) / (420 - 380);
	}
	else if(420 < wavelength && wavelength <= 701){
		factor = 1.0;
	}
	else if(701 < wavelength && wavelength <= 780){
		factor = 0.3 + 0.7*(780 - wavelength) / (780 - 701);
	}
	else{
		factor = 0.0;
	}
	R = adjust(red,   factor);
	G = adjust(green, factor);
	B = adjust(blue,  factor);
}


double image_based_error_app::
get_wavelength_from_data_point(double value, double min_value, double max_value) {
	const double min_visible_wavelength = 380.0;//350.0;
	const double max_visible_wavelength = 780.0;//650.0;
	//Convert data value in the range of min_values..max_values to the
	//range 380..780
	return (value - min_value) / (max_value - min_value) * (max_visible_wavelength - min_visible_wavelength) + min_visible_wavelength;
}


void image_based_error_app::
calc_pixel_color(BYTE* pixel, double value, double min_value, double max_value) {
unsigned char r = 0;
unsigned char g = 0;
unsigned char b = 0;
double wavelength = std::numeric_limits<double>::lowest();

wavelength = get_wavelength_from_data_point(value, min_value, max_value);
wavelength_to_RGB(wavelength, r, g, b);

pixel[2] = r;
pixel[1] = g;
pixel[0] = b;
}


void image_based_error_app::
write_heatmap_key(unsigned int width_in_px, unsigned int height_in_px, std::string const& path)
{
    BYTE* pixels = new BYTE[3*width_in_px*height_in_px];

    for(unsigned int w = 0; w != width_in_px; ++w)
    {
        for(unsigned int h = 0; h != height_in_px; ++h)
            calc_pixel_color( (pixels +3*(w + h*width_in_px)), w, 0.0, (double) width_in_px );
    }


    FIBITMAP* image = FreeImage_ConvertFromRawBits(pixels, width_in_px, height_in_px, 3 * width_in_px, 24, 0x0000FF, 0xFF0000, 0x00FF00, false);
    FreeImage_Save(FIF_PNG, image, (path+"heatmap_key.png").c_str(), 0);

    FreeImage_Unload(image);
}

void image_based_error_app::
write_delta_E_image(double* dE_arr, unsigned int im_w, unsigned im_h, double min_err, double max_err, unsigned int im_num, std::string const& out_file_path)
{
                // Make the BYTE array, factor of 3 because it's RBG.
                BYTE* pixels = new BYTE[ 3 * im_w * im_h];

                for(uint32_t i = 0; i < im_w*im_h; ++i ) {
                    calc_pixel_color( (pixels +3*i), dE_arr[i], min_err, max_err );
                }



                std::string diffpath = out_file_path;
/*
                if(! boost::filesystem::exists(diffpath))
                {
                    std::cout<<"Creating Folder.\n\n";
                    boost::filesystem::create_directories(diffpath);
                }
*/
/*
                std::string file_name;

                file_name =  ( (im_num < 10) ? (std::string("000") ) : (im_num < 100 ?  std::string("00") : (im_num < 1000 ? std::string("0") : std::string("") ) ) )
                            + std::to_string(im_num) + std::string(".png");
*/
                // Convert to FreeImage format & save to file
                FIBITMAP* image = FreeImage_ConvertFromRawBits(pixels, im_w, im_h, 3 * im_w, 24, 0x0000FF, 0xFF0000, 0x00FF00, false);
                FreeImage_Save(FIF_PNG, image, (out_file_path).c_str(), 0);

                // Free resources
                FreeImage_Unload(image);
}




bool image_based_error_app::
compute_image_differences(std::string const& image_path_1, 
						  std::string const& image_path_2, 
						  std::string const& out_image_path) {

  try {
    load_image_to_memory(image_path_1.c_str(), image_data_1_, image_width_1_, image_height_1_);

    int32_t image_width_2 = -1;
    int32_t image_height_2 = -1;

    load_image_to_memory(image_path_2.c_str(), image_data_2_, image_width_2, image_height_2);

    if (image_width_1_ != image_width_2 || image_height_1_ != image_height_2) {
      std::cout << "Dimensions of the images does not match\n";
    }

    double** diff_values_local = (double**)malloc(sizeof(double*));


    double* avg_delta_E_errors_local = (double*)malloc(sizeof(double));
    double* min_delta_E_errors_local = (double*)malloc(sizeof(double));
    double* max_delta_E_errors_local = (double*)malloc(sizeof(double));

    double* valid_pixel_array = (double*)malloc(sizeof(double));

    double max_global_error = std::numeric_limits<double>::lowest();
    double min_global_error = std::numeric_limits<double>::max();


    diff_values_local[0] = (double*)malloc(image_width_1_ * image_height_1_ * sizeof(double));

    // double* diffValues = (double*) malloc(w_im_1*h_im_1 * sizeof(double));

    double error_sum = 0.0;
    double max_local_error = std::numeric_limits<double>::lowest();
    double min_local_error = std::numeric_limits<double>::max();


    unsigned int valid_pixel_count = 0;

    std::cout << "Reading & Converting\n";

    for (int i = 0; i < image_width_1_* image_height_1_; ++i) {
      //std::cout << "Reading first component\n";
      unsigned char r1 = image_data_1_[i * 4 + 0];
      //std::cout << "Reading second component\n";
      unsigned char g1 = image_data_1_[i * 4 + 1];
      unsigned char b1 = image_data_1_[i * 4 + 2];

      unsigned char r2 = image_data_2_[i * 4 + 0];
      unsigned char g2 = image_data_2_[i * 4 + 1];
      unsigned char b2 = image_data_2_[i * 4 + 2];
      //std::cout << "Read colors\n";

      lamure::qm::col3 rgb1 = { r1, g1, b1 };
      lamure::qm::col3 rgb2 = { r2, g2, b2 };

      lamure::qm::col3 lab1 = cc::rgb_to_xyz(cc::xyz_to_lab(rgb1));
      lamure::qm::col3 lab2 = cc::rgb_to_xyz(cc::xyz_to_lab(rgb2));

      double delta_E = cc::calc_delta_E(lab1, lab2);

      if (delta_E > max_local_error) {
        max_local_error = delta_E;
      }
      if (delta_E < min_local_error) {
        min_local_error = delta_E;
      }

      if (delta_E > max_global_error) {
        max_global_error = delta_E;
      }

      if (delta_E < min_global_error) {
        min_global_error = delta_E;
      }

      //std::cout << "About to write diff val\n";
      diff_values_local[0][i] = delta_E;


      error_sum += delta_E;

      ++valid_pixel_count;
    }

    std::cout << "Read & Converted\n";

    valid_pixel_array[0] = (double)valid_pixel_count / (image_width_1_*image_height_1_);

    if (valid_pixel_count != 0) {
      error_sum /= valid_pixel_count;//w_im_1*h_im_1;
    }
    else {
      error_sum = 0;
    }

    avg_delta_E_errors_local[0] = error_sum;

    min_delta_E_errors_local[0] = min_local_error;
    max_delta_E_errors_local[0] = max_local_error;

    std::cout << "Average pixel error sum of images [" << out_image_path << "]: " << error_sum << "\n";

    free_image_data(image_data_1_);
    free_image_data(image_data_2_);


    std::cout << "Write key\n";
    write_heatmap_key(1000, 30, out_image_path + "_key.png");



    std::string splat_count_low;
    std::string splat_count_high;


    std::string currImFilename = ((0 < 10) ? (std::string("000")) : (0 < 100 ? std::string("00") : (0 < 1000 ? std::string("0") : std::string("")))) + std::to_string(0) + ".png";

    double valid_pixel_in_percent = valid_pixel_array[0] * 100;
    unsigned int valid_pixel_pre_point = valid_pixel_in_percent;
    unsigned int valid_pixel_after_point = ((valid_pixel_in_percent - valid_pixel_pre_point) + 0.005) * 100;


    write_delta_E_image(diff_values_local[0], image_width_1_, image_height_1_, min_delta_E_errors_local[0], max_delta_E_errors_local[0], 0, out_image_path);
    //write_delta_E_image(diff_values_local[0], image_width_1, image_height_1_, min_global_error, max_global_error, 0, arg1_as_string, out);

    bool min_local_equals_max_global = (min_delta_E_errors_local[0] == min_global_error);
    bool max_local_equals_max_global = (max_delta_E_errors_local[0] == max_global_error);


    std::cout << "min delta_E_erros_local: " << min_delta_E_errors_local[0] << "\n";
    std::cout << "max delta_E_erros_local: " << max_delta_E_errors_local[0] << "\n";

    std::cout << "De-Init FreeImage\n";
    FreeImage_DeInitialise();




    free(diff_values_local[0]);

    free(diff_values_local);

    free(avg_delta_E_errors_local);
    free(min_delta_E_errors_local);
    free(max_delta_E_errors_local);
    free(valid_pixel_array);

    return true;
  }
  catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
    return false;
  }


}

bool image_based_error_app::
load_image_to_memory(std::string const& filepath, BYTE*& out_raw_image_data, int& image_width, int& image_height) {
    bool executed_successfully = true;
    char const* c_filepath = filepath.c_str();

    FIBITMAP* bitmap = FreeImage_Load(
        FreeImage_GetFileType(c_filepath, 0),
        c_filepath);

    if ( !bitmap ) {
        std::cout<<"Unable to load file: "<<filepath<<"\n";
        executed_successfully = false;
    }

    FIBITMAP* thirty_two_bit_version = FreeImage_ConvertTo32Bits(bitmap);
    FreeImage_Unload(bitmap);

    image_width  = FreeImage_GetWidth(thirty_two_bit_version);
    image_height = FreeImage_GetHeight(thirty_two_bit_version);

    out_raw_image_data = (BYTE*)malloc(image_width * image_height * 4);

    BYTE* pixels = (BYTE*) FreeImage_GetBits(thirty_two_bit_version);

    if(!pixels) {
        std::cout << "Unable to allocate pixel array\n";
        executed_successfully = false;
    }

    for(int32_t pixel_idx; pixel_idx < image_width * image_height; ++pixel_idx) {
        out_raw_image_data[pixel_idx * 4 + 3] = pixels[pixel_idx * 4 + 3];
        out_raw_image_data[pixel_idx * 4 + 2] = pixels[pixel_idx * 4 + 0];
        out_raw_image_data[pixel_idx * 4 + 1] = pixels[pixel_idx * 4 + 1];
        out_raw_image_data[pixel_idx * 4 + 0] = pixels[pixel_idx * 4 + 2];
    }

    return executed_successfully;
}


}
}