#include <iostream>
#include <chrono>
#include <algorithm>
#include <fstream>
#include <memory>
#include <ImageMagick-6/Magick++.h>
#include <boost/filesystem/path.hpp>
#include "SimpleIni.h"
#include "QuadTree.cpp"
#include "Config.cpp"

class Preprocessor
{
public:
    CSimpleIniA *config;

    Preprocessor(const char *dir_raster, const char *path_config)
    {
        Magick::InitializeMagick(dir_raster);
        config = new CSimpleIniA(true, false, false);

        if (config->LoadFile(path_config) < 0)
        {
            throw std::runtime_error("Configuration file parsing error");
        }
    }

    ~Preprocessor()
    {

    }

    bool prepare_single_raster(const char *name_raster)
    {
        Magick::Image image;
        try
        {
            image.read(name_raster);
            Magick::Geometry geometry = image.size();
            size_t side = (size_t) std::pow(2, std::floor(std::log(std::min(geometry.width(), geometry.height())) / std::log(2)));
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

    bool prepare_mipmap()
    {
        std::ifstream input;
        input.open(config->GetValue(Config::TEXTURE_MANAGEMENT, Config::FILE_PPM, Config::DEFAULT), std::ifstream::in | std::ifstream::binary);

        char *buf_file_version = new char[3];
        input.getline(&buf_file_version[0], 3, '\x0A');

        if (strcmp(buf_file_version, "P6"))
        {
            throw std::runtime_error("PPM file format not equal to P6");
        }

        char *buf_dim_x = new char[8];
        char *buf_dim_y = new char[8];
        char *buf_color_depth = new char[8];
        input.getline(&buf_dim_x[0], 8, '\x20');
        input.getline(&buf_dim_y[0], 8, '\x0A');
        input.getline(&buf_color_depth[0], 8, '\x0A');

        size_t dim_x = (size_t) atol(buf_dim_x);
        size_t dim_y = (size_t) atol(buf_dim_y);
        short color_depth = (short) atoi(buf_color_depth);

        if (color_depth != 255)
        {
            throw std::runtime_error("PPM color depth not equal to 255");
        }

        if (dim_x != dim_y || (dim_x & (dim_x - 1)) != 0)
        {
            throw std::runtime_error("PPM dimensions are not equal or not a power of 2");
        }

        short tile_size = (short) atoi(config->GetValue(Config::TEXTURE_MANAGEMENT, Config::TILE_SIZE, Config::DEFAULT));

        if ((tile_size & (tile_size - 1)) != 0)
        {
            throw std::runtime_error("Tile size is not a power of 2");
        }

        uint32_t tree_depth = QuadTree::calculate_depth(dim_x, tile_size);

        char *buf_tile = new char[tile_size * tile_size * 3];
        std::string buf_header = "P6\x0A" + std::to_string(tile_size) + "\x20" + std::to_string(tile_size) + "\x0A" + "255" + "\x0A";

        streamoff offset_header = input.tellg();

        for (uint32_t _depth = tree_depth; _depth > 0; _depth--)
        {
            size_t first_node = QuadTree::get_first_node_id_of_depth(_depth);
            size_t last_node = QuadTree::get_first_node_id_of_depth(_depth) + QuadTree::get_length_of_depth(_depth);

            for (size_t _id = first_node; _id < last_node; _id++)
            {
                size_t skip_rows = (_id - first_node) / QuadTree::get_tiles_per_row(_depth);
                size_t skip_cols = (_id - first_node) % QuadTree::get_tiles_per_row(_depth);

                input.seekg(offset_header);
                input.ignore(skip_rows * QuadTree::get_tiles_per_row(_depth) * tile_size * tile_size * 3);

                for (uint32_t _row = 0; _row < tile_size; _row++)
                {
                    input.ignore(skip_cols * tile_size * 3);
                    input.read(&buf_tile[_row * tile_size * 3], tile_size * 3);
                    input.ignore((QuadTree::get_tiles_per_row(_depth) - skip_cols - 1) * tile_size * 3);
                }

                std::ofstream output;
                output.open("id_" + std::to_string(_id) + ".ppm", std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
                output.write(&buf_header.c_str()[0], buf_header.size());
                output.write(&buf_tile[0], tile_size * tile_size * 3);
                output.close();
            }
        }

        input.close();

        // TODO: read tiles
//        const char *file_mipmap = config->GetValue(Config::TEXTURE_MANAGEMENT, Config::FILE_MIPMAP, Config::DEFAULT);
//        std::ofstream output;
//
//        output.open(file_mipmap, std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
//        output.close();

        delete buf_tile, buf_file_version, buf_dim_x, buf_dim_y, buf_color_depth;

        return false;
    }
};