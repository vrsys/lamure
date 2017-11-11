#include <iostream>
#include <list>
#include <chrono>
#include <algorithm>
#include <fstream>
#include <memory>
#include <ImageMagick-6/Magick++.h>
#include <boost/filesystem/path.hpp>
#include "SimpleIni.h"
#include "QuadTree.cpp"
#include "Config.cpp"
#include "morton.h"

class Preprocessor {
public:
  CSimpleIniA *config;

  explicit Preprocessor(const char *path_config) {
      config = new CSimpleIniA(true, false, false);

      if (config->LoadFile(path_config) < 0)
      {
          throw std::runtime_error("Configuration file parsing error");
      }

      boost::filesystem::path
          path_texture(config->GetValue(Config::TEXTURE_MANAGEMENT, Config::FILE_PPM, Config::DEFAULT));
      boost::filesystem::path dir_texture = path_texture.parent_path();
      Magick::InitializeMagick(dir_texture.c_str());
  }

  ~Preprocessor() = default;

  bool prepare_single_raster(const char *name_raster) {
      Magick::Image image;
      try
      {
          image.read(name_raster);
          Magick::Geometry geometry = image.size();
          size_t side =
              (size_t) std::pow(2, std::floor(std::log(std::min(geometry.width(), geometry.height())) / std::log(2)));
          geometry.aspect(false);
          geometry.width(side);
          geometry.height(side);
          image.crop(geometry);
          image.write(config->GetValue(Config::TEXTURE_MANAGEMENT, Config::FILE_PPM, Config::DEFAULT));
      }
      catch (Magick::Exception &error_)
      {
          std::cout << "Caught exception: " << error_.what() << std::endl;
          return false;
      };
      return true;
  }

  bool prepare_mipmap() {
      std::ifstream input;
      input.open(config->GetValue(Config::TEXTURE_MANAGEMENT, Config::FILE_PPM, Config::DEFAULT),
                 std::ifstream::in | std::ifstream::binary);

      size_t dim_x = 0, dim_y = 0;
      read_ppm_header(input, dim_x, dim_y);

      streamoff offset_header = input.tellg();

      auto tile_size = (size_t) atoi(config->GetValue(Config::TEXTURE_MANAGEMENT, Config::TILE_SIZE, Config::DEFAULT));
      if ((tile_size & (tile_size - 1)) != 0)
      {
          throw std::runtime_error("Tile size is not a power of 2");
      }

      auto *buf_tile = new char[tile_size * tile_size * 3];
      std::string buf_header =
          "P6\x0A" + std::to_string(tile_size) + "\x20" + std::to_string(tile_size) + "\x0A" + "255" + "\x0A";

      uint32_t tree_depth = QuadTree::calculate_depth(dim_x, tile_size);
      size_t first_node = QuadTree::get_first_node_id_of_depth(tree_depth);
      size_t
          last_node = QuadTree::get_first_node_id_of_depth(tree_depth) + QuadTree::get_length_of_depth(tree_depth) - 1;

      for (size_t _id = first_node; _id <= last_node; _id++)
      {
          uint_fast64_t skip_cols, skip_rows;
          morton2D_64_decode((uint_fast64_t) (_id - first_node), skip_cols, skip_rows);

          input.seekg(offset_header);
          input.ignore(skip_rows * QuadTree::get_tiles_per_row(tree_depth) * tile_size * tile_size * 3);

          for (uint32_t _row = 0; _row < tile_size; _row++)
          {
              input.ignore(skip_cols * tile_size * 3);
              input.read(&buf_tile[_row * tile_size * 3], tile_size * 3);
              input.ignore((QuadTree::get_tiles_per_row(tree_depth) - skip_cols - 1) * tile_size * 3);
          }

          std::ofstream output;
          output.open("id_" + std::to_string(_id) + ".ppm",
                      std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
          output.write(&buf_header.c_str()[0], buf_header.size());
          output.write(&buf_tile[0], tile_size * tile_size * 3);
          output.close();
      }

      input.close();

      for (uint32_t _depth = tree_depth - 1; _depth > 0; _depth--)
      {
          first_node = QuadTree::get_first_node_id_of_depth(_depth);
          last_node = QuadTree::get_first_node_id_of_depth(_depth) + QuadTree::get_length_of_depth(_depth) - 1;

          for (size_t _id = first_node; _id <= last_node; _id++)
          {
              std::list<Magick::Image> children_imgs;

              for (size_t i = 0; i < 4; i++)
              {
                  Magick::Image _child;
                  _child.read("id_" + std::to_string(QuadTree::get_child_id(_id, i)) + ".ppm");
                  children_imgs.push_back(_child);
              }

              write_stitched_tile(_id, tile_size, children_imgs);
          }
      }

      std::list<Magick::Image> children_imgs;

      for (size_t i = 0; i < 4; i++)
      {
          Magick::Image _child;
          _child.read("id_" + std::to_string(QuadTree::get_child_id(0, i)) + ".ppm");
          children_imgs.push_back(_child);
      }

      write_stitched_tile(0, tile_size, children_imgs);

      const char *file_mipmap = config->GetValue(Config::TEXTURE_MANAGEMENT, Config::FILE_MIPMAP, Config::DEFAULT);
      std::ofstream output;
      output.open(file_mipmap, std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);

      for (uint32_t _depth = 0; _depth <= tree_depth; _depth++)
      {
          first_node = QuadTree::get_first_node_id_of_depth(_depth);
          last_node = QuadTree::get_first_node_id_of_depth(_depth) + QuadTree::get_length_of_depth(_depth) - 1;

          for (size_t _id = first_node; _id <= last_node; _id++)
          {
              std::ifstream input_tile;
              input_tile.open("id_" + std::to_string(_id) + ".ppm", std::ifstream::in | std::ifstream::binary);

              read_ppm_header(input_tile, dim_x, dim_y);
              input_tile.read(&buf_tile[0], tile_size * tile_size * 3);

              input_tile.close();

              output.write(&buf_tile[0], tile_size * tile_size * 3);

              if (atoi(config->GetValue(Config::DEBUG, Config::KEEP_INTERMEDIATE_DATA, Config::DEFAULT)) != 1)
              {
                  std::remove(("id_" + std::to_string(_id) + ".ppm").c_str());
              }
          }
      }

      output.close();

      return false;
  }

private:

  void read_ppm_header(std::ifstream &_ifs, size_t &_dim_x, size_t &_dim_y) {
      auto *buf_file_version = new char[3];
      _ifs.getline(&buf_file_version[0], 3, '\x0A');

      if (strcmp(buf_file_version, "P6") != 0)
      {
          throw std::runtime_error("PPM file format not equal to P6");
      }

      auto *buf_dim_x = new char[8];
      auto *buf_dim_y = new char[8];
      auto *buf_color_depth = new char[8];

      _ifs.getline(&buf_dim_x[0], 8, '\x20');
      _ifs.getline(&buf_dim_y[0], 8, '\x0A');
      _ifs.getline(&buf_color_depth[0], 8, '\x0A');

      _dim_x = (size_t) atol(buf_dim_x);
      _dim_y = (size_t) atol(buf_dim_y);
      auto color_depth = (short) atoi(buf_color_depth);

      if (color_depth != 255)
      {
          throw std::runtime_error("PPM color depth not equal to 255");
      }

      if (_dim_x != _dim_y || (_dim_x & (_dim_x - 1)) != 0)
      {
          throw std::runtime_error("PPM dimensions are not equal or not a power of 2");
      }
  }

  void write_stitched_tile(size_t _id, size_t _tile_size, list<Magick::Image> &_child_imgs) {
      Magick::Montage montage_settings;
      montage_settings.tile(Magick::Geometry(2, 2));
      montage_settings.geometry(Magick::Geometry(_tile_size, _tile_size));

      std::list<Magick::Image> montage_list;
      Magick::montageImages(&montage_list, _child_imgs.begin(), _child_imgs.end(), montage_settings);
      Magick::writeImages(montage_list.begin(), montage_list.end(), "id_" + std::to_string(_id) + ".ppm");

      Magick::Image image;
      image.read("id_" + std::to_string(_id) + ".ppm");
      Magick::Geometry geometry = image.size();
      geometry.width(_tile_size);
      geometry.height(_tile_size);
      image.scale(geometry);
      image.depth(8);
      image.write("id_" + std::to_string(_id) + ".ppm");
  }
};