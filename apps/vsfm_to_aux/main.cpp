// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <iostream>
#include <sstream>

#include <lamure/prov/common.h>
#include <lamure/prov/dense_cache.h>
#include <lamure/prov/dense_stream.h>
#include <lamure/prov/sparse_cache.h>
#include <lamure/prov/sparse_octree.h>

#include <lamure/prov/3rd_party/exif.h>

#include <lamure/prov/aux.h>

#include <scm/core/math.h>
#include <scm/gl_core/math.h>

using namespace std;

char *get_cmd_option(char **begin, char **end, const string &option) {
    char **it = find(begin, end, option);
    if(it != end && ++it != end)
        return *it;
    return 0;
}

bool cmd_option_exists(char **begin, char **end, const string &option) { 
    return find(begin, end, option) != end; 
}

bool check_file_extensions(string name_file, const char *pext) {
    string ext(pext);
    if(name_file.substr(name_file.size() - ext.size()).compare(ext) != 0)
    {
        cout << "Please specify " + ext + " file as input" << endl;
        return true;
    }
    return false;
}

int main(int argc, char *argv[]) {
    if (argc == 1 || 
      cmd_option_exists(argv, argv+argc, "-h") ||
      !cmd_option_exists(argv, argv+argc, "-nvm") ||
      !cmd_option_exists(argv, argv+argc, "-fotos") 
      //||
      //!cmd_option_exists(argv, argv+argc, "-patch") ||
      //!cmd_option_exists(argv, argv+argc, "-ply")
      ) {
        
      std::cout << "Usage: " << argv[0] << std::endl <<
         "INFO: bvh_leaf_extractor " << std::endl <<
         "\t-nvm: .nvm input file" << std::endl <<
         "\t-fotos: fotos directory" << std::endl <<
         //"\t-patch: .patch input file" << std::endl <<
         //"\t-ply: .ply input file" << std::endl <<
         "\t-aux: .aux output file" << std::endl <<
         std::endl;
      return 0;
    }

    std::string nvm_file = string(get_cmd_option(argv, argv + argc, "-nvm"));
    std::string fotos_directory = string(get_cmd_option(argv, argv + argc, "-fotos"));
    //std::string patch_file = string(get_cmd_option(argv, argv + argc, "-patch"));
    //std::string ply_file = string(get_cmd_option(argv, argv + argc, "-ply"));
    std::string aux_file = string(get_cmd_option(argv, argv + argc, "-aux"));

    if (check_file_extensions(nvm_file, ".nvm") && 
        //check_file_extensions(ply_file, ".ply") && 
        //check_file_extensions(patch_file, ".patch") && 
        check_file_extensions(aux_file, ".aux")) {
        throw std::runtime_error("File format is incompatible");
    }

    lamure::prov::aux aux;

    //parse nvm file
    std::ifstream nvm(nvm_file.c_str(), std::ios::in);
    
    if (!nvm.is_open()) {
      std::cout << "could not open nvm file" << std::endl;
      exit(-1);
    }

    std::string line;

    //nvm format: http://ccwu.me/vsfm/doc.html#nvm

    
    std::getline(nvm, line); //nvm
    std::getline(nvm, line);
    std::getline(nvm, line); //num views

    uint32_t num_views = 0;
    {
      std::istringstream line_ss(line);
      line_ss >> num_views;
    }
    std::cout << num_views << " views" << std::endl;

    for (uint32_t i = 0; i < num_views; ++i) {
      std::getline(nvm, line);
      std::istringstream line_ss(line);
      
      lamure::prov::aux::view v;
      v.camera_id_ = i;
      line_ss >> v.image_file_;
      line_ss >> v.focal_length_;
      scm::math::quatf quat;
      line_ss >> quat.w;
      line_ss >> quat.x;
      line_ss >> quat.y;
      line_ss >> quat.z;
      line_ss >> v.position_.x;
      line_ss >> v.position_.y;
      line_ss >> v.position_.z;
      
      quat = scm::math::normalize(quat);

      scm::math::mat3f m = scm::math::mat3f::identity();
      float qw = quat.w;
      float qx = quat.i;
      float qy = quat.j;
      float qz = quat.k;
      m[0] = (qw * qw + qx * qx - qz * qz - qy * qy);
      m[1] = (2 * qx * qy - 2 * qz * qw);
      m[2] = (2 * qy * qw + 2 * qz * qx);
      m[3] = (2 * qx * qy + 2 * qw * qz);
      m[4] = (qy * qy + qw * qw - qz * qz - qx * qx);
      m[5] = (2 * qz * qy - 2 * qx * qw);
      m[6] = (2 * qx * qz - 2 * qy * qw);
      m[7] = (2 * qy * qz + 2 * qw * qx);
      m[8] = (qz * qz + qw * qw - qy * qy - qx * qx);      

      quat = scm::math::quatf::from_matrix(m) * scm::math::quatf::from_axis(180, scm::math::vec3f(1.0, 0.0, 0.0));

      v.transform_ = scm::math::make_translation(v.position_) * quat.to_matrix();
      line_ss >> v.distortion_;
      
      std::cout << "Reading image " << fotos_directory+v.image_file_ << std::endl;

      FILE *fp = fopen((fotos_directory+v.image_file_).c_str(), "rb");
      if(!fp) {
        std::cout << "can't read image" << std::endl;
        exit(-1);
      }
      fseek(fp, 0, SEEK_END);
      size_t fsize = (size_t)ftell(fp);
      rewind(fp);
      unsigned char *buf = new unsigned char[fsize];
      if (fread(buf, 1, fsize, fp) != fsize) {
        delete[] buf;
        std::cout << "can't read image" << std::endl;
        exit(-1);
      }
      fclose(fp);

      easyexif::EXIFInfo result;
      result.parseFrom(buf, (unsigned int)fsize);

      v.image_height_ = result.ImageHeight;
      v.image_width_ = result.ImageWidth;
      //v.focal_length_ = result.FocalLength * 0.001;
 
      v.tex_atlas_id_ = 0;

      aux.add_view(v);

    }

    std::getline(nvm, line);
    std::getline(nvm, line); //num points

    uint32_t num_points = 0;
    {
      std::istringstream line_ss(line);
      line_ss >> num_points;
    }
    std::cout << num_points << " points" << std::endl;

    for (uint32_t i = 0; i < num_points; ++i) {
      std::getline(nvm, line);
      std::istringstream line_ss(line);
    
      lamure::prov::aux::sparse_point p;
      line_ss >> p.pos_.x;
      line_ss >> p.pos_.y;
      line_ss >> p.pos_.z;
      uint32_t r, g, b;
      line_ss >> r;
      line_ss >> g;
      line_ss >> b;
      p.r_ = (uint8_t)r;
      p.g_ = (uint8_t)g;
      p.b_ = (uint8_t)b;
      p.a_ = (uint8_t)255;
      uint32_t num_measurements = 0;
      
      line_ss >> num_measurements;
      for (uint32_t j = 0; j < num_measurements; ++j) {
        lamure::prov::aux::feature f;
        line_ss >> f.camera_id_;
        uint32_t feature_index; //ignore
        line_ss >> feature_index;
        line_ss >> f.coords_.x;
        line_ss >> f.coords_.y;
        f.error_ = scm::math::vec2f(0.f, 0.f);

        p.features_.push_back(f);
      }

      aux.add_sparse_point(p);
      
    }

    nvm.close();

    aux.write_aux_file(aux_file);

    std::cout << "Done" << std::endl;

    return 0;
}




























