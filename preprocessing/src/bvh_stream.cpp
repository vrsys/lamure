// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/bvh_stream.h>

#include <lamure/pre/serialized_surfel.h>

namespace lamure {
namespace pre {

BvhStream::
BvhStream()
: filename_(""),
  num_segments_(0) {


}

BvhStream::
~BvhStream() {
    CloseStream(false);
}

void BvhStream::
OpenStream(const std::string& bvh_filename,
           const BvhStreamType type) {

    CloseStream(false);
    
    num_segments_ = 0;
    filename_ = bvh_filename;
    type_ = type;

    std::ios::openmode mode = std::ios::binary;

    if (type_ == BvhStreamType::BVH_STREAM_IN) {
        mode |= std::ios::in;
    }
    if (type_ == BvhStreamType::BVH_STREAM_OUT) {
        mode |= std::ios::out;
        mode |= std::ios::trunc;
    }

    file_.open(filename_, mode);

    if (!file_.is_open()) {
        throw std::runtime_error(
            "PLOD: BvhStream::Unable to open stream: " + filename_);
    }
   
}

void BvhStream::
CloseStream(const bool remove_file) {
    
    if (file_.is_open()) {
        if (type_ == BvhStreamType::BVH_STREAM_OUT) {
            file_.flush();
        }
        file_.close();
        if (file_.fail()) {
            throw std::runtime_error(
                "PLOD: BvhStream::Unable to close stream: " + filename_);
        }

        if (type_ == BvhStreamType::BVH_STREAM_OUT) {
            if (remove_file) {
                if (std::remove(filename_.c_str())) {
                    throw std::runtime_error(
                        "PLOD: BvhStream::Unable to delete file: " + filename_);
                }

            }
        }
    }

}


void BvhStream::
Write(BvhStream::BvhSerializable& serializable) {

    if (!file_.is_open()) {
        throw std::runtime_error(
            "PLOD: BvhStream::Unable to serialize: " + filename_);
    }

    BvhSig sig;

    size_t allocated_size = sig.size() + serializable.size();
    size_t used_size = allocated_size;
    size_t padding = 0;
    while (allocated_size % 32 != 0) {
        ++allocated_size;
        ++padding;
    }
    
    serializable.signature(sig.signature_);

    sig.reserved_ = 0;
    sig.allocated_size_ = allocated_size - sig.size();
    sig.used_size_ = used_size - sig.size();

    sig.serialize(file_);
    serializable.serialize(file_);

    while (padding) {
        char c = 0;
        file_.write(&c, 1);
        --padding;
    }

}


void BvhStream::
ReadBvh(const std::string& filename, Bvh& bvh) {
 
    OpenStream(filename, BvhStreamType::BVH_STREAM_IN);

    if (type_ != BVH_STREAM_IN) {
        throw std::runtime_error(
            "PLOD: BvhStream::Failed to read bvh from: " + filename_);
    }
    if (!file_.is_open()) {
         throw std::runtime_error(
            "PLOD: BvhStream::Failed to read bvh from: " + filename_);
    }
   
    //scan stream
    file_.seekg(0, std::ios::end);
    size_t filesize = (size_t)file_.tellg();
    file_.seekg(0, std::ios::beg);

    num_segments_ = 0;

    BvhTreeSeg tree;
    BvhTreeExtensionSeg tree_ext;
    std::vector<BvhNodeSeg> nodes;
    std::vector<BvhNodeExtensionSeg> nodes_ext;
    uint32_t tree_id = 0;
    uint32_t tree_ext_id = 0;
    uint32_t node_id = 0;
    uint32_t node_ext_id = 0;


    //go through entire stream and fetch the segments
    while (true) {
        BvhSig sig;
        sig.deserialize(file_);
        if (sig.signature_[0] != 'B' ||
            sig.signature_[1] != 'V' ||
            sig.signature_[2] != 'H' ||
            sig.signature_[3] != 'X') {
             throw std::runtime_error(
                 "PLOD: BvhStream::Invalid magic encountered: " + filename_);
        }
            
        size_t anchor = (size_t)file_.tellg();

        switch (sig.signature_[4]) {

            case 'F': { //"BVHXFILE"
                BvhFileSeg seg;
                seg.deserialize(file_);
                break;
            }
            case 'T': { 
                switch (sig.signature_[5]) {
                    case 'R': { //"BVHXTREE"
                        tree.deserialize(file_);
                        ++tree_id;
                        break;
                    }
                    case 'E': { //"BVHXTEXT"
                        tree_ext.deserialize(file_);
                        ++tree_ext_id;
                        break;                     
                    }
                    default: {
                        throw std::runtime_error(
                            "PLOD: BvhStream::Stream corrupt -- Invalid segment encountered");
                        break;
                    }
                }
                break;
            }
            case 'N': { 
                switch (sig.signature_[5]) {
                    case 'O': { //"BVHXNODE"
                        BvhNodeSeg node;
                        node.deserialize(file_);
                        nodes.push_back(node);
                        if (node_id != node.node_id_) {
                            throw std::runtime_error(
                                "PLOD: BvhStream::Stream corrupt -- Invalid node order");
                        }
                        ++node_id;
                        break;
                    }
                    case 'E': { //"BVHXNEXT"
                        BvhNodeExtensionSeg node_ext;
                        node_ext.deserialize(file_);
                        nodes_ext.push_back(node_ext);
                        if (node_ext_id != node_ext.node_id_) {
                            throw std::runtime_error(
                                "PLOD: BvhStream::Stream corrupt -- Invalid node extension order");
                        }
                        ++node_ext_id;
                        break;
                    }
                    default: {
                        throw std::runtime_error(
                            "PLOD: BvhStream::Stream corrupt -- Invalid segment encountered");
                        break;
                    }
                }
                break;
            }
            default: {
                throw std::runtime_error(
                    "PLOD: BvhStream::File corrupt -- Invalid segment encountered");
                break;
            }
        }

        if (anchor + sig.allocated_size_ < filesize) {
            file_.seekg(anchor + sig.allocated_size_, std::ios::beg);
        }
        else {
            break;
        }

    }

    CloseStream(false);

    if (tree_id != 1) {
       throw std::runtime_error(
           "PLOD: BvhStream::Stream corrupt -- Invalid number of bvh segments");
    }   

    if (tree_ext_id > 1) {
       throw std::runtime_error(
           "PLOD: BvhStream::Stream corrupt -- Invalid number of bvh extensions");
    }    

    //Note: This is the preprocessing library version of the file reader!

    //setup bvh
    bvh.set_depth(tree.depth_);
    bvh.set_fan_factor(tree.fan_factor_);
    bvh.set_max_surfels_per_node(tree.max_surfels_per_node_);
    scm::math::vec3f translation(tree.translation_.x_,
                                 tree.translation_.y_,
                                 tree.translation_.z_);
    bvh.set_translation(vec3r(translation));
    if (tree.num_nodes_ != node_id) {
        throw std::runtime_error(
            "PLOD: BvhStream::Stream corrupt -- Invalid number of node segments");
    }
    if (tree_ext_id > 0) {
        if (tree.num_nodes_ != node_ext_id) {
            throw std::runtime_error(
                "PLOD: BvhStream::Stream corrupt -- Invalid number of node extensions");
        }
    }
    
    std::vector<SharedFile> level_temp_files;
    
    Bvh::State current_state = static_cast<Bvh::State>(tree.state_);
    
    //check if intermediate state
    bool interm_state = current_state == Bvh::State::AfterDownsweep
                        || current_state == Bvh::State::AfterUpsweep;

    boost::filesystem::path base_path;
    if (interm_state) {
        if (tree_ext_id != 1) {
            throw std::runtime_error(
                "PLOD: BvhStream::Stream corrupt -- Stream is missing tree extension");
        }
        //working_directory = tree_ext.working_directory_.string_;
        //basename_ = boost::filesystem::path(tree_ext.filename_.string_);
        base_path = boost::filesystem::path(tree_ext.filename_.string_);

        //setup level temp files
        for (uint32_t i = 0; i < tree_ext.num_disk_accesses_; ++i) {
             level_temp_files.push_back(std::make_shared<File>());
             level_temp_files.back()->Open(tree_ext.disk_accesses_[i].string_, false);
        }
    }
    else {
        base_path = boost::filesystem::canonical(boost::filesystem::path(filename));
        base_path.replace_extension("");
    }
    bvh.set_base_path(base_path);

    //setup nodes
    std::vector<BvhNode> bvh_nodes(tree.num_nodes_);

    for (uint32_t i = 0; i < tree.num_nodes_; ++i) {
       const auto& node = nodes[i];

       if (i != node.node_id_) {
           throw std::runtime_error(
               "PLOD: BvhStream::Stream corrupt -- Invalid node ordering");
       }

       scm::math::vec3f centroid(node.centroid_.x_,
                                 node.centroid_.y_,
                                 node.centroid_.z_);
       scm::math::vec3f box_min(node.bounding_box_.min_.x_,
                                node.bounding_box_.min_.y_,
                                node.bounding_box_.min_.z_);
       scm::math::vec3f box_max(node.bounding_box_.max_.x_,
                                node.bounding_box_.max_.y_,
                                node.bounding_box_.max_.z_);

       if (interm_state) {
           const auto& node_ext = nodes_ext[i];
           if (i != node_ext.node_id_) {
               throw std::runtime_error(
                   "PLOD: BvhStream::Stream corrupt -- Invalid extension ordering");
           }
           
           if (node_ext.empty_ == 1) {
               bvh_nodes[i] = BvhNode(node.node_id_, node.depth_, BoundingBox(vec3r(box_min), vec3r(box_max)));
           }
           else {
               const auto& disk_array = node_ext.disk_array_;
               SurfelDiskArray surfel_disk_array = SurfelDiskArray(level_temp_files[disk_array.disk_access_ref_],
                                                                   disk_array.offset_,
                                                                   disk_array.length_);
               bvh_nodes[i] = BvhNode(node.node_id_, node.depth_, BoundingBox(vec3r(box_min), vec3r(box_max)), surfel_disk_array);
           }
       }
       else {
           //init empty nodes. We don't use SurfelDIskArray
           //because we deal with serialized data
           bvh_nodes[i] = BvhNode(node.node_id_, node.depth_, BoundingBox(vec3r(box_min), vec3r(box_max)));
       }

       //set node params
       bvh_nodes[i].set_reduction_error(node.reduction_error_);
       bvh_nodes[i].set_centroid(vec3r(centroid));
       bvh_nodes[i].set_avg_surfel_radius(node.avg_surfel_radius_);
       bvh_nodes[i].set_visibility((BvhNode::NodeVisibility)node.visibility_);

    }

    bvh.set_first_leaf(tree.num_nodes_ - std::pow(tree.fan_factor_, tree.depth_));
    bvh.set_state(current_state);
    bvh.set_nodes(bvh_nodes);

}

void BvhStream::
WriteBvh(const std::string& filename, Bvh& bvh, const bool intermediate) {

   OpenStream(filename, BvhStreamType::BVH_STREAM_OUT);

   if (type_ != BVH_STREAM_OUT) {
       throw std::runtime_error(
           "PLOD: BvhStream::Failed to append tree to: " + filename_);
   }
   if (!file_.is_open()) {
       throw std::runtime_error(
           "PLOD: BvhStream::Failed to append tree to: " + filename_);
   }
   
   file_.seekp(0, std::ios::beg);

   BvhFileSeg seg;
   seg.major_version_ = 0;
   seg.minor_version_ = 1;
   seg.reserved_ = 0;

   Write(seg);

   //Note: This is the preprocessing library version of the file writer!

   BvhTreeSeg tree;
   tree.segment_id_ = num_segments_++;
   tree.depth_ = bvh.depth();
   tree.num_nodes_ = bvh.nodes().size();
   tree.fan_factor_ = bvh.fan_factor();
   tree.max_surfels_per_node_ = bvh.max_surfels_per_node();
   tree.serialized_surfel_size_ = SerializedSurfel::GetSize();
   tree.reserved_0_ = 0;
   tree.state_ = (BvhStream::BvhTreeState)bvh.state();
   tree.reserved_1_ = 0;
   tree.reserved_2_ = 0;
   tree.translation_.x_ = bvh.translation().x;
   tree.translation_.y_ = bvh.translation().y;
   tree.translation_.z_ = bvh.translation().z;
   tree.reserved_3_ = 0;

   Write(tree);

   const auto& bvh_nodes = bvh.nodes();
   for (uint32_t i = 0; i < bvh_nodes.size(); ++i) {
       const auto& bvh_node = bvh_nodes[i];
       BvhNodeSeg node;
       node.segment_id_ = num_segments_++;
       node.node_id_ = i;
       node.centroid_.x_ = bvh_node.centroid().x;
       node.centroid_.y_ = bvh_node.centroid().y;
       node.centroid_.z_ = bvh_node.centroid().z;
       node.depth_ = bvh_node.depth();
       node.reduction_error_ = bvh_node.reduction_error();
       node.avg_surfel_radius_ = bvh_node.avg_surfel_radius();
       node.visibility_ = (BvhNodeVisibility)bvh_node.visibility();
       node.reserved_ = 0;
       node.bounding_box_.min_.x_ = bvh_node.bounding_box().min().x;
       node.bounding_box_.min_.y_ = bvh_node.bounding_box().min().y;
       node.bounding_box_.min_.z_ = bvh_node.bounding_box().min().z;
       node.bounding_box_.max_.x_ = bvh_node.bounding_box().max().x;
       node.bounding_box_.max_.y_ = bvh_node.bounding_box().max().y;
       node.bounding_box_.max_.z_ = bvh_node.bounding_box().max().z;

       Write(node);
   }


   if (intermediate) {
       BvhTreeExtensionSeg tree_ext;
       tree_ext.segment_id_ = num_segments_++;
       tree_ext.working_directory_.string_ = "DEADBEEF";
       tree_ext.working_directory_.length_ = tree_ext.working_directory_.string_.length();
       tree_ext.filename_.string_ = bvh.base_path().string();
       tree_ext.filename_.length_ = tree_ext.filename_.string_.length();
       tree_ext.num_disk_accesses_ = 0;

       for (uint32_t i = 0; i < bvh_nodes.size(); ++i) {
           const auto& bvh_node = bvh_nodes[i];
           BvhNodeExtensionSeg node_ext;
           node_ext.segment_id_ = num_segments_++;
           node_ext.node_id_ = bvh_node.node_id();
           node_ext.empty_ = 1;
           node_ext.reserved_ = 0;
           node_ext.disk_array_.disk_access_ref_ = 0;
           node_ext.disk_array_.reserved_ = 0;
           node_ext.disk_array_.offset_ = 0;
           node_ext.disk_array_.length_ = 0;

           //only OOC nodes are saved, IC nodes are considered empty

           if (bvh_node.IsOOC()) {
               node_ext.empty_ = 0;

               bool disk_access_found = false;
               for (uint32_t k = 0; k < tree_ext.num_disk_accesses_; ++k) {
                   if (tree_ext.disk_accesses_.size() < k) {
                       throw std::runtime_error(
                           "PLOD: BvhStream::Stream corrupt");
                   }
                   if (tree_ext.disk_accesses_[k].string_ == bvh_node.disk_array().file()->file_name()) {
                       node_ext.disk_array_.disk_access_ref_ = k;
                       disk_access_found = true;
                       break;
                   }
               }
               
               if (!disk_access_found) {
                  BvhString disk_access;
                  disk_access.string_ = bvh_node.disk_array().file()->file_name();
                  disk_access.length_ = disk_access.string_.length();
                  tree_ext.disk_accesses_.push_back(disk_access);
                  node_ext.disk_array_.disk_access_ref_ = tree_ext.num_disk_accesses_;
                  ++tree_ext.num_disk_accesses_;
               }

               node_ext.disk_array_.offset_ = bvh_node.disk_array().offset();
               node_ext.disk_array_.length_ = bvh_node.disk_array().length();
               
           }

           Write(node_ext);
       }

       Write(tree_ext);
   }
   else {
       bvh.set_state(Bvh::State::Serialized);
   }

   CloseStream(false);

}



} } // namespace lamure
