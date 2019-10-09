// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <chrono>
#include <lamure/prov/common.h>
#include <lamure/prov/dense_cache.h>
#include <lamure/prov/dense_stream.h>
#include <lamure/prov/sparse_cache.h>
#include <lamure/prov/sparse_octree.h>
#include <lamure/prov/octree.h>

#include <lamure/prov/auxi.h>

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
      !cmd_option_exists(argv, argv+argc, "-s") ||
      //!cmd_option_exists(argv, argv+argc, "-d") ||
      !cmd_option_exists(argv, argv+argc, "-f") ||
      !cmd_option_exists(argv, argv+argc, "-o")) {
        
      std::cout << "Usage: " << argv[0] << std::endl <<
         "INFO: bvh_leaf_extractor " << std::endl <<
         "\t-s: sparse.prov file" << std::endl <<
         //"\t-d: dense.prov file" << std::endl <<
         "\t-f: fotos directory" << std::endl <<
         "\t-o: output.auxi file" << std::endl <<
         std::endl;
      return 0;
    }


    std::string sparse_file = string(get_cmd_option(argv, argv + argc, "-s"));
    std::string dense_file = string(get_cmd_option(argv, argv + argc, "-d"));
    std::string aux_file = string(get_cmd_option(argv, argv + argc, "-o"));
    std::string fotos_directory = string(get_cmd_option(argv, argv + argc, "-f"));

    if (check_file_extensions(sparse_file, ".prov") && 
        //check_file_extensions(dense_file, ".prov") && 
        check_file_extensions(aux_file, ".auxi")) {
        throw std::runtime_error("File format is incompatible");
    }

    std::ifstream in_sparse(sparse_file, std::ios::in | std::ios::binary);
    std::ifstream in_sparse_meta(sparse_file + ".meta", std::ios::in | std::ios::binary);
    //std::ifstream in_dense(dense_file, std::ios::in | std::ios::binary);
    //std::ifstream in_dense_meta(dense_file + ".meta", std::ios::in | std::ios::binary);

    lamure::prov::SparseCache cache_sparse(in_sparse, in_sparse_meta);
    //lamure::prov::DenseCache cache_dense(in_dense, in_dense_meta);
    
    if(in_sparse.is_open()) {
      std::cout << "Caching sparse..." << std::endl;
      cache_sparse.cache(true, fotos_directory);
      in_sparse.close();
    }
    else {
      std::cout << "Sparse file not found" << std::endl;
      exit(0);
    }
/*
    if(in_dense.is_open()) {
      std::cout << "Caching dense..." << std::endl;
      cache_dense.cache();
      in_dense.close();
    }
    else {
      std::cout << "Dense file not found" << std::endl;
      exit(0);
    }
*/
    lamure::prov::auxi aux;

    //sparse points
    std::cout << "Converting sparse_points..." << std::endl;
    
    const std::vector<lamure::prov::SparsePoint>& feature_points = cache_sparse.get_points();
    for (uint64_t i = 0; i < feature_points.size(); ++i) {
      const auto& point = feature_points[i];
      lamure::prov::auxi::sparse_point p;
      p.pos_ = point.get_position();
      p.r_ = (uint8_t)point.get_color().x;
      p.g_ = (uint8_t)point.get_color().y;
      p.b_ = (uint8_t)point.get_color().z;
      p.a_ = (uint8_t)255;

      const std::vector<lamure::prov::SparsePoint::Measurement>& measurements = point.get_measurements();
      for (uint64_t j = 0; j < measurements.size(); ++j) {
        const auto& measurement = measurements[j];

        lamure::prov::auxi::feature f;
        f.camera_id_ = measurement.get_camera();
        f.using_count_ = 1;
        f.coords_ = measurement.get_occurence();
        f.error_ = scm::math::vec2f(0.f, 0.f);

        p.features_.push_back(f);

      }

      aux.add_sparse_point(p);

    }

    //views
    std::vector<lamure::prov::Camera> cameras = cache_sparse.get_cameras();
    std::cout << "Converting " << cameras.size() << " views..." << std::endl;

    //std::vector<lamure::prov::MetaData> metadata = cache_sparse.get_cameras_metadata();
    for (uint64_t i = 0; i < cameras.size(); ++i) {
      auto& camera = cameras[i];

      lamure::prov::auxi::view v;
      v.camera_id_ = camera.get_index();
      v.position_ = camera.get_translation();

      auto translation = scm::math::make_translation(camera.get_translation());
      auto rotation = scm::math::mat4f(camera.get_orientation().to_matrix());
      v.transform_ = translation * rotation;

      v.focal_value_x_ = camera.get_focal_length();
      v.focal_value_y_ = 0.f;
      v.center_x_ = 0.f;
      v.center_y_ = 0.f;
      //uint32_t distortion = 0;
      //for (uint32_t j = 0; j < 4; ++j) {
      //  distortion |= ((uint32_t)metadata[i].get_metadata()[j]) << (j*8);
      //}
      //v.distortion_ = *(float*)&distortion;
      v.distortion_ = 0.f;
      v.image_width_ = camera.get_image_width();
      v.image_height_ = camera.get_image_height();
      v.atlas_tile_id_ = 0;
      v.image_file_ = camera.get_image_file();

      aux.add_view(v);
    }


    std::cout << "create octree " << std::endl;

    //create octree
    auto octree = std::make_shared<lamure::prov::octree>();
    octree->create(aux.get_sparse_points());
    aux.set_octree(octree);

    std::cout << "Writing auxi file..." << std::endl;

    aux.write_aux_file(aux_file);

    std::cout << "Done" << std::endl;

    return 0;
}
