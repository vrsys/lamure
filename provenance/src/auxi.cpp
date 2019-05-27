// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/prov/auxi.h>

#include <limits>

#include <sys/stat.h>
#include <fcntl.h>

#include <lamure/prov/aux_stream.h>
#include <lamure/prov/octree.h>

namespace lamure {
namespace prov {


auxi::
auxi()
: filename_("") {


} 

auxi::
auxi(const std::string& filename)
: filename_("") {

    std::string extension = filename.substr(filename.find_last_of(".") + 1);

    if (extension.compare("auxi") == 0) {
       load_aux_file(filename);
    }
    else {
       throw std::runtime_error(
          "lamure: auxi::Invalid file extension encountered.");
    }

};

void auxi::
load_aux_file(const std::string& filename) {

    filename_ = filename;

    aux_stream aux_stream;
    aux_stream.read_aux(filename, *this);
}


void auxi::
write_aux_file(const std::string& filename) {
    
    filename_ = filename;

    aux_stream aux_stream;
    aux_stream.write_aux(filename, *this);

}

const auxi::view& auxi::
get_view(const uint32_t view_id) const {
    assert(view_id >= 0 && view_id < views_.size());
    return views_[view_id];
}


const auxi::sparse_point& auxi::
get_sparse_point(const uint64_t point_id) const {
    assert(point_id >= 0 && point_id < sparse_points_.size());
    return sparse_points_[point_id];
}

const auxi::atlas_tile& auxi::
get_atlas_tile(const uint32_t tile_id) const {
    assert(tile_id >= 0 && tile_id < atlas_tiles_.size());
    return atlas_tiles_[tile_id];
}

void auxi::
add_view(const auxi::view& view) {
    views_.push_back(view);
}

void auxi::
add_sparse_point(const auxi::sparse_point& point) {
    sparse_points_.push_back(point);
}

void auxi::
add_atlas_tile(const auxi::atlas_tile& tile) {
    atlas_tiles_.push_back(tile);
}

void auxi::set_octree(const std::shared_ptr<octree> _octree) {
  octree_ = _octree;
}

const std::shared_ptr<octree> auxi::
get_octree() const {
  return octree_;
}

void auxi::set_atlas(const atlas& _atlas) {
  atlas_ = _atlas;
}

const auxi::atlas& auxi::
get_atlas() const {
  return atlas_;
}



} } // namespace lamure

