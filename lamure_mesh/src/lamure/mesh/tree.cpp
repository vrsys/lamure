
#include <lamure/mesh/tree.h>

namespace lamure {
namespace mesh {

tree_t::
tree_t(uint32_t fanout_factor)
    : fanout_factor_(fanout_factor), num_nodes_(0) {

}

tree_t::
~tree_t() {

}

const node_t tree_t::
get_child_id(const node_t node_id, const uint32_t child_index) const {
    return node_id*fanout_factor_ + 1 + child_index;
}

const node_t tree_t::
get_parent_id(const node_t node_id) const {
    if (node_id == 0) return invalid_node_t;

    if (node_id % fanout_factor_ == 0)
        return node_id/fanout_factor_ - 1;
    else
        return (node_id + fanout_factor_ - (node_id % fanout_factor_)) / fanout_factor_ - 1;
}

const uint32_t tree_t::
get_depth_of_node(const node_t node_id) const {
    return (uint32_t)(std::log((node_id+1) * (fanout_factor_-1)) / std::log(fanout_factor_));
}

const uint32_t tree_t::
get_length_of_depth(const uint32_t depth) const {
    return (uint32_t)(std::pow((double)fanout_factor_, (double)depth));
}


const node_t tree_t::
get_first_node_id_of_depth(const uint32_t depth) const {
    node_t id = 0;
    for (uint32_t i = 0; i < depth; ++i){
        id += (node_t)std::pow((double)fanout_factor_, (double)i);
    }

    return id;
}


void tree_t::
sort_indices(const tree_node_t& node, const triangle_t* triangles, uint32_t* indices, axis_t axis) {
    //size_t num_in_core = 4096; //todo: determine this as needed

    //std::string filename = stream.get_filename();
    //std::string temp_path = filename.substr(0, filename.size()-3) += "radix";

    uint32_t num_tris_in_node = node.end_ - node.begin_;
    //uint32_t* temp_indices = (uint32_t*)new char[num_tris_in_node * sizeof(uint32_t)];
    uint32_t* temp_indices = (uint32_t*)new char[num_tris_in_node * sizeof(uint32_t)];

    const uint32_t hist_length = 2048;
    signed long long* hist = new signed long long[hist_length * 3];
    memset(hist, 0, sizeof(uint64_t) * hist_length * 3);

    //fill histograms
    for (uint32_t s = node.begin_; s < node.end_; s++) {
        const triangle_t& tri = triangles[indices[s]];
        vec3r_t centroid = tri.get_centroid();

        float32_t fv = axis == AXIS_X ? centroid.x_ : axis == AXIS_Y ? centroid.y_ : centroid.z_;
        uint32_t v = *(uint32_t*)(&fv);

        v ^= -int32_t(v >> 31) | 0x80000000;

        hist[(v & 0x7ff)]++;
        hist[hist_length + (v >> 11 & 0x7ff)]++;
        hist[hist_length + hist_length + (v >> 22)]++;
    }


    //scan histograms
    signed long long temp;
    signed long long k0 = 0, k1 = 0, k2 = 0;
    for (int k = 0; k < hist_length; k++) {
        temp = hist[k] + k0;
        hist[k] = k0 - 1;
        k0 = temp;

        temp = hist[hist_length + k] + k1;
        hist[hist_length + k] = k1 - 1;
        k1 = temp;

        temp = hist[hist_length + hist_length + k] + k2;
        hist[hist_length + hist_length + k] = k2 - 1;
        k2 = temp;
    }

    //pass 0
    for (uint32_t s = node.begin_; s < node.end_; s++) {
        const triangle_t& tri = triangles[indices[s]];
        vec3r_t centroid = tri.get_centroid();

        float32_t fv = axis == AXIS_X ? centroid.x_ : axis == AXIS_Y ? centroid.y_ : centroid.z_;
        uint32_t v = *(uint32_t*)(&fv);


        v ^= -int32_t(v >> 31) | 0x80000000;
        size_t pos = ++hist[(v & 0x7ff)];

        //temp_access->Write(&data, s, pos, 1);
        temp_indices[pos] = indices[s];
    }

    //pass 1
    for (uint32_t s = 0; s < num_tris_in_node; s++) {
        const triangle_t& tri = triangles[temp_indices[s]];
        vec3r_t centroid = tri.get_centroid();

        float32_t fv = axis == AXIS_X ? centroid.x_ : axis == AXIS_Y ? centroid.y_ : centroid.z_;
        uint32_t v = *(uint32_t*)(&fv);

        v ^= -int32_t(v >> 31) | 0x80000000;
        size_t pos = ++hist[hist_length + (v >> 11 & 0x7ff)];

        //disk_access_->Write(&data, s, first_in_file_ + pos, 1);
        indices[node.begin_ + pos] = temp_indices[s];
    }

    //pass 2
    for (uint32_t s = node.begin_; s < node.end_; s++) {
        const triangle_t& tri = triangles[indices[s]];
        vec3r_t centroid = tri.get_centroid();

        float32_t fv = axis == AXIS_X ? centroid.x_ : axis == AXIS_Y ? centroid.y_ : centroid.z_;
        uint32_t v = *(uint32_t*)(&fv);


        v ^= -int32_t(v >> 31) | 0x80000000;
        size_t pos = ++hist[hist_length + hist_length + (v >> 22)];

        //temp_access->Write(&data, s, pos, 1);
        temp_indices[pos] = indices[s];
    }

    //copy from temp to file
    memcpy((char*)(indices+node.begin_), (char*)temp_indices, num_tris_in_node*sizeof(uint32_t));

    //cleanup
    delete[] temp_indices;
    delete[] hist;

}


}
}
