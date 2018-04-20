
//run this to use: export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/ofl/no_freeimage_ofl/lib

#include <cstdint>

#if 0

#include <iostream>
#include <last/types.h>
#include <last/math.h>
#include <algorithm>
#include <FreeImage.h>
#include <tv_3/ofl/optical_flow_generator.h>
#include <tv_3/ofl/raw_image.h>

char* get_cmd_option(char** begin, char** end, const std::string & option) {
    char** it = std::find(begin, end, option);
    if (it != end && ++it != end)
        return *it;
    return 0;
}

bool cmd_option_exists(char** begin, char** end, const std::string& option) {
    return std::find(begin, end, option) != end;
}

tv_3::ofl::raw_image create_from_file(std::string const& image_path) { 
  //load with freeimage & create raw image from memory (other c'tor)

  auto image_c_string = image_path.c_str();

  FREE_IMAGE_FORMAT formato = FreeImage_GetFileType(image_c_string,0);
  FIBITMAP* imagen = FreeImage_Load(formato, image_c_string);

  FIBITMAP* temp = imagen;
  imagen = FreeImage_ConvertTo32Bits(imagen);
  FreeImage_Unload(temp);

  int w = FreeImage_GetWidth(imagen);
  int h = FreeImage_GetHeight(imagen);

  tv_3::ofl::raw_image_t image_data(4*w*h);
  char* unsorted_pixels = (char*)FreeImage_GetBits(imagen);

  //FreeImage loads in BGR format, so we need to swap some bytes(or use GL_BGR).
  for(int j= 0; j<w*h; j++){
    image_data[j*4+0]= unsorted_pixels[j*4+2];
    image_data[j*4+1]= unsorted_pixels[j*4+1];
    image_data[j*4+2]= unsorted_pixels[j*4+0];
    image_data[j*4+3]= unsorted_pixels[j*4+3];
  }

  return tv_3::ofl::raw_image(image_data, w, h, tv_3::ofl::pixel_layout_t::RGBA);
}

void write_to_file(tv_3::ofl::raw_image& image, std::string const& out_image_path) {

  FIBITMAP* bitmap = FreeImage_Allocate(image.get_width(), image.get_height(), 3 * 8);
  RGBQUAD rgba_color;
  if( !bitmap ) {
  	std::cout << "ERROR: Could not allocate memory for bitmap\n";
  }
 
  std::cout << image.get_width() << " *** " << image.get_height() << "\n";

  uint8_t* data = image.get_raw_data_ptr();

  int32_t col_offset = image.get_width(); 
  for( int32_t x_idx = 0; x_idx < image.get_width(); ++x_idx) {
    for ( int32_t y_idx = 0; y_idx < image.get_height(); ++y_idx) {
  	  int32_t pixel_start = ( (x_idx + y_idx * col_offset ) * 4);
  	  rgba_color.rgbRed      = data[ pixel_start ];
  	  rgba_color.rgbGreen    = data[ pixel_start + 1];
  	  rgba_color.rgbBlue     = data[ pixel_start + 2];
  	  //rgba_color.rgbReserved = data[ pixel_start + 3];
      FreeImage_SetPixelColor(bitmap, x_idx, y_idx, &rgba_color);
    } 
  }

  std::cout << "Before saving\n";
  if( FreeImage_Save(FIF_PNG, bitmap, out_image_path.c_str(), 0) ) {
  	std::cout << "Saved image to: " << out_image_path.c_str() << "\n";
  }

}


int main(int argc, char *argv[]) {
    
    if (argc == 1 || 
      cmd_option_exists(argv, argv+argc, "-h") ||
      !cmd_option_exists(argv, argv+argc, "-i0") ||
      !cmd_option_exists(argv, argv+argc, "-i1")) {
        
      std::cout << "Usage: " << argv[0] << "<flags> -i0 <input_file> -i1 <input_file>" << std::endl <<
         "INFO: argv[0] " << std::endl <<
         std::endl;
      return 0;
    }

    std::string i0 = get_cmd_option(argv, argv+argc, "-i0");
    std::string i1 = get_cmd_option(argv, argv+argc, "-i1");

    FIBITMAP* img0 = FreeImage_Load(FreeImage_GetFileType(i0.c_str(), 0), i0.c_str());
    uint32_t width0 = FreeImage_GetWidth(img0);
    uint32_t height0 = FreeImage_GetHeight(img0);
    FIBITMAP* img1 = FreeImage_Load(FreeImage_GetFileType(i1.c_str(), 0), i1.c_str());
    uint32_t width1 = FreeImage_GetWidth(img1);
    uint32_t height1 = FreeImage_GetHeight(img1);
    std::cout << "i0: " << width0 << ", " << height0 << std::endl;
    std::cout << "i1: " << width1 << ", " << height1 << std::endl;
  

    //for this example, use load both raw images from user specified paths
    auto raw_image_0 = create_from_file(i0);
    auto raw_image_1 = create_from_file(i1);

    //setup optic flow params
    tv_3::ofl::brox_descriptor brox;
    brox.smoothness_term_ = 0.197; //0.197
    brox.edge_term_ = 50.0; //50.0
    brox.scaling_factor_ = 0.95; //0.95
    brox.num_inner_iterations_ = 5; //5
    brox.num_outer_iterations_ = 150; //150
    brox.num_solver_iterations_ = 10; //10

    tv_3::ofl::flow_descriptor_t flow_desc;
    flow_desc.brox_descriptor_ = brox;

  
    //the following two lines are the essential ones for computing optic flow

    //prepare empty data container
    std::vector<float> of_2d_image;
    //use cuda to compute the optical flow (high res, lots of iterations, currently brox only)
    tv_3::ofl::compute_optical_flow(raw_image_0, raw_image_1,
                                  of_2d_image, flow_desc);
    //afterwards, of_2d_image contains 2*width*height float elements, x and y are interleaved

    auto flow_vis_image = tv_3::ofl::raw_image::create_flow_visualization(of_2d_image, raw_image_0.get_width(), raw_image_0.get_height());

    //save debug flow vis as image
    write_to_file(flow_vis_image, "out_debug_flow.png");
    
    for (uint32_t x = 0; x < width0; ++x) {
      for (uint32_t y = 0; y < height0; ++y) {
        //get the flow from flow image
        uint32_t iy = height0-y;
        last::math::vec2d_t flow = last::math::vec2d_t(
          of_2d_image[2*(width0*iy+x)], 
          of_2d_image[2*(height0*iy+x)+1]);
      }
    }

    return 0;

}

#endif

/*#include <iostream>
#include <algorithm>
#include <FreeImage.h>
#include <fstream>

void create_from_file(const char *in_image_path, const char *out_image_path) {
  //load with freeimage & create raw image from memory (other c'tor)

  std::cout << in_image_path << std::endl;

  std::ofstream out_file(out_image_path);

  size_t bufferSize = 3 * 1024 * 1024;
  uint8_t *buffer = new uint8_t[bufferSize];

  FREE_IMAGE_FORMAT formato = FreeImage_GetFileType(in_image_path,0);
  FIBITMAP* imagen = FreeImage_Load(formato, in_image_path);

  FIBITMAP* temp = imagen;
  imagen = FreeImage_ConvertTo32Bits(imagen);
  FreeImage_Unload(temp);

  int w = FreeImage_GetWidth(imagen);
  int h = FreeImage_GetHeight(imagen);

  std::cout << w << std::endl;
  std::cout << h << std::endl;

  char* unsorted_pixels = (char*)FreeImage_GetBits(imagen);

  uint64_t px_count = w * h;
  //FreeImage loads in BGR format, so we need to swap some bytes(or use GL_BGR).
  for(int j = 0; j < px_count; j++){
    buffer[(j * 3 + 0) % bufferSize] = unsorted_pixels[j * 3 + 2];
    buffer[(j * 3 + 1) % bufferSize] = unsorted_pixels[j * 3 + 1];
    buffer[(j * 3 + 2) % bufferSize] = unsorted_pixels[j * 3 + 0];

    if(((j * 3 + 3) % bufferSize) == 0){
      out_file.write((char*)buffer, bufferSize);
    }
  }

  out_file.write((char*)buffer, (px_count * 3) % bufferSize);

  delete buffer;
}

/*void write_to_file(tv_3::ofl::raw_image& image, std::string const& out_image_path) {

  FIBITMAP* bitmap = FreeImage_Allocate(image.get_width(), image.get_height(), 3 * 8);
  RGBQUAD rgba_color;
  if( !bitmap ) {
    std::cout << "ERROR: Could not allocate memory for bitmap\n";
  }

  std::cout << image.get_width() << " *** " << image.get_height() << "\n";

  uint8_t* data = image.get_raw_data_ptr();

  int32_t col_offset = image.get_width();
  for( int32_t x_idx = 0; x_idx < image.get_width(); ++x_idx) {
    for ( int32_t y_idx = 0; y_idx < image.get_height(); ++y_idx) {
      int32_t pixel_start = ( (x_idx + y_idx * col_offset ) * 4);
      rgba_color.rgbRed      = data[ pixel_start ];
      rgba_color.rgbGreen    = data[ pixel_start + 1];
      rgba_color.rgbBlue     = data[ pixel_start + 2];
      //rgba_color.rgbReserved = data[ pixel_start + 3];
      FreeImage_SetPixelColor(bitmap, x_idx, y_idx, &rgba_color);
    }
  }

  std::cout << "Before saving\n";
  if( FreeImage_Save(FIF_PNG, bitmap, out_image_path.c_str(), 0) ) {
    std::cout << "Saved image to: " << out_image_path.c_str() << "\n";
  }

}

int main() {
  create_from_file("~/Downloads/Lunar_LRO_LROC-WAC_Mosaic_global_100m_June2013.tif", "/mnt/terabytes_of_textures/FINAL_DEMO_DATA/loonar_maps_sources/test.data");
  return 0;
}*/




int main(){
    return 0;
}

