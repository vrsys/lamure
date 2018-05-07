// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr


#include <iostream>
#include <cmath>
#include "preprocess_obj_transformer.h"

preprocess_obj_transformer::preprocess_obj_transformer(const std::string &obj_file, const std::string &atlas_file) {
    this->set_obj_path(obj_file);
    this->set_atlas(atlas_file);
}

preprocess_obj_transformer::~preprocess_obj_transformer() {
    delete atlas;
}

void preprocess_obj_transformer::set_obj_path(const std::string &obj_path) {
    this->obj_in_path  = obj_path;
    this->obj_out_path = this->obj_in_path.substr(0, this->obj_in_path.size()-4) + "_pre_vt.obj";
}

void preprocess_obj_transformer::set_atlas(const std::string &atlas_file) {
    try {
        this->atlas = new AtlasFile(atlas_file.c_str());
    } catch(std::runtime_error &error){
        std::cout << "Could not open file \"" << atlas_file << "\"." << std::endl;
    }
}

std::vector<std::string> preprocess_obj_transformer::split(const std::string &phrase, const std::string &delimiter) const{
    std::vector<std::string> ret;
    size_t start = 0;
    size_t end = 0;
    size_t len = 0;
    std::string token;
    do{
        end = phrase.find(delimiter,start);
        len = end - start;
        token = phrase.substr(start, len);
        ret.emplace_back( token );
        start += len + delimiter.length();
    }while ( end != std::string::npos );
    return ret;
}

std::string preprocess_obj_transformer::scale() const {
    uint64_t image_width    = atlas->getImageWidth();
    uint64_t image_height   = atlas->getImageHeight();

    uint64_t tile_inner_width  = atlas->getInnerTileWidth();
    uint64_t tile_inner_height = atlas->getInnerTileHeight();

    uint64_t depth = atlas->getDepth();

    double scale_u  = (double) image_width  / (tile_inner_width  * std::pow(2, depth-1));
    double scale_v  = (double) image_height / (tile_inner_height * std::pow(2, depth-1));

    // open streams
    std::ifstream obj_file_in(obj_in_path.c_str());

    if (!obj_file_in.is_open()) {
        std::cout << "could not open .obj file" << std::endl;
        exit(-1);
    }

    std::ofstream obj_file_out(obj_out_path.c_str(), std::ios::trunc);

    // read and write .obj files
    std::string line;
    while(std::getline(obj_file_in, line)) {
        if (line.substr(0,2) == "vt") {
            auto tokens = split(line, " ");
            line = tokens[0] + " "
                   + std::to_string(std::stod(tokens[1]) * scale_u) + " "
                   + std::to_string(std::stod(tokens[2]) * scale_v + (1 - scale_v));
        }
        obj_file_out << line << std::endl;
    }

    obj_file_in.close();
    obj_file_out.close();

    return obj_out_path;
}

