// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef REN_BVH_STREAM_H_
#define REN_BVH_STREAM_H_

#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <cstring>

#include <lamure/ren/platform.h>
#include <lamure/types.h>
#include <scm/gl_core/primitives/box.h>

#include <lamure/ren/bvh.h>

namespace lamure {
namespace ren {

class RENDERING_DLL BvhStream
{

public:
    BvhStream();
    ~BvhStream();


    enum BvhStreamType {
        BVH_STREAM_IN = 0,
        BVH_STREAM_OUT = 1
    };

    const BvhStreamType type() const { return type_; };
    const std::string filename() const { return filename_; };

    void ReadBvh(const std::string& filename, Bvh& bvh);
    void WriteBvh(const std::string& filename, Bvh& bvh);


protected:

    struct BvhVector {
        float x_;
        float y_;
        float z_;
    };
    struct BvhBoundingBox {
        BvhVector min_;
        BvhVector max_;
    };
    struct BvhDiskArray {
        uint32_t disk_access_ref_; //reference to OocDiskAccess
        uint32_t reserved_;
        uint64_t offset_;
        uint64_t length_; //length of zero defines empty BvhDiskArray
    };
    struct BvhString {
        uint64_t length_;
        std::string string_;
    };
    enum BvhNodeVisibility {
        BVH_NODE_VISIBLE = 0,
        BVH_NODE_INVISIBLE = 1
    };
    enum BvhTreeState {
        BVH_STATE_NULL            = 0, //null tree
        BVH_STATE_EMPTY           = 1, //initialized, but empty tree
        BVH_STATE_AFTER_DOWNSWEEP = 2, //after downsweep
        BVH_STATE_AFTER_UPSWEEP   = 3, //after upsweep
        BVH_STATE_SERIALIZED      = 4  //serialized surfel data
    };
  
    class BvhSerializable {
    public:
        ~BvhSerializable() {};
        size_t data_offset_;
    protected:
        friend class BvhStream;
        BvhSerializable() {};
        virtual const size_t size() const = 0;
        virtual void signature(char* signature) = 0;
        virtual void serialize(std::fstream& file) = 0;
        virtual void deserialize(std::fstream& file) = 0;

        void serialize_string(std::fstream& file, const BvhString& text) {
            if (!file.is_open()) {
                throw std::runtime_error(
                    "PLOD: BvhStream::Unable to serialize");
            }
            file.write((char*)&text.length_, 8);
            file.write(text.string_.c_str(), text.length_);

            size_t allocated_size = 8 + text.length_;
            size_t padding = 32 - (allocated_size % 32);
            while (padding--) {
                char c = 0;
                file.write(&c, 1);
            }
        }
        void deserialize_string(std::fstream& file, BvhString& text) {
            if (!file.is_open()) {
                throw std::runtime_error(
                    "PLOD: BvhStream::Unable to deserialize");
            }
            file.read((char*)&text.length_, 8);
            char* buffer = new char[text.length_];
            memset(buffer, 0, text.length_);
            file.read(buffer, text.length_);
            text.string_ = std::string(buffer);
            delete[] buffer;
            
            size_t allocated_size = 8 + text.length_;
            size_t padding = 32 - (allocated_size % 32);

            while (padding--) {
                char c = 0;
                file.read(&c, 1);
            }
            
            //deserialization fix:
            text.string_ = text.string_.substr(0, text.length_);
        }

    };

    class BvhSig : public BvhSerializable {
    public:
        BvhSig()
        : BvhSerializable() {};
        ~BvhSig() {};
        char signature_[8];
        size_t reserved_;
        size_t allocated_size_;
        size_t used_size_;
    protected:
        friend class BvhStream;
        const size_t size() const {
            return 8*sizeof(uint32_t);
        }
        void signature(char* signature) {};
        void serialize(std::fstream& file) {
             if (!file.is_open()) {
                 throw std::runtime_error(
                     "PLOD: BvhStream::Unable to serialize");
             }
             file.write(signature_, 8);
             file.write((char*)&reserved_, 8);
             file.write((char*)&allocated_size_, 8);
             file.write((char*)&used_size_, 8);
        }
        void deserialize(std::fstream& file) {
             if (!file.is_open()) {
                 throw std::runtime_error(
                     "PLOD: BvhStreamLLUnable to deserialize");
             }
             for (uint32_t i = 0; i < 8; ++i) {
                 file.read(&signature_[i], 1);
             }
             file.read((char*)&reserved_, 8);
             file.read((char*)&allocated_size_, 8);
             file.read((char*)&used_size_, 8);
        }
    };

    class BvhFileSeg : public BvhSerializable {
    public:
        BvhFileSeg()
        : BvhSerializable() {};
        ~BvhFileSeg() {};

        uint32_t major_version_;
        uint32_t minor_version_;
        size_t reserved_;

    protected:
        friend class BvhStream;
        const size_t size() const {
            return 4*sizeof(uint32_t);
        }
        void signature(char* signature) {
            signature[0] = 'B';
            signature[1] = 'V';
            signature[2] = 'H';
            signature[3] = 'X';
            signature[4] = 'F';
            signature[5] = 'I';
            signature[6] = 'L';
            signature[7] = 'E';
        }
        void serialize(std::fstream& file) {
            if (!file.is_open()) {
                throw std::runtime_error(
                    "PLOD: BvhStream::Unable to serialize");
            }
            file.write((char*)&major_version_, 4);
            file.write((char*)&minor_version_, 4);
            file.write((char*)&reserved_, 8);
        }
        void deserialize(std::fstream& file) {
            if (!file.is_open()) {
                throw std::runtime_error(
                    "PLOD: BvhStream::Unable to deserialize");
            }
            file.read((char*)&major_version_, 4);
            file.read((char*)&minor_version_, 4);
            file.read((char*)&reserved_, 8);
        }
    };

    class BvhTreeSeg : public BvhSerializable {
    public:
        BvhTreeSeg()
        : BvhSerializable() {};
        ~BvhTreeSeg() {};

        uint32_t segment_id_;
        uint32_t depth_;
        uint32_t num_nodes_;
        uint32_t fan_factor_;

        uint32_t max_surfels_per_node_;
        uint32_t serialized_surfel_size_;
        uint64_t reserved_0_;

        BvhTreeState state_;
        uint32_t reserved_1_;
        uint64_t reserved_2_;

        BvhVector translation_;
        uint32_t reserved_3_;

    protected:
        friend class BvhStream;
        const size_t size() const {
            return 16*sizeof(uint32_t);
        }
        void signature(char* signature) {
            signature[0] = 'B';
            signature[1] = 'V';
            signature[2] = 'H';
            signature[3] = 'X';
            signature[4] = 'T';
            signature[5] = 'R';
            signature[6] = 'E';
            signature[7] = 'E';
        }
        void serialize(std::fstream& file) {
            if (!file.is_open()) {
                throw std::runtime_error(
                    "PLOD: BvhStream::Unable to serialize");
            }
            file.write((char*)&segment_id_, 4);
            file.write((char*)&depth_, 4);
            file.write((char*)&num_nodes_, 4);
            file.write((char*)&fan_factor_, 4);
            file.write((char*)&max_surfels_per_node_, 4);
            file.write((char*)&serialized_surfel_size_, 4);
            file.write((char*)&reserved_0_, 8);
            file.write((char*)&state_, 4);
            file.write((char*)&reserved_1_, 4);
            file.write((char*)&reserved_2_, 8);
            file.write((char*)&translation_.x_, 4);
            file.write((char*)&translation_.y_, 4);
            file.write((char*)&translation_.z_, 4);
            file.write((char*)&reserved_3_, 4);
        } 
        void deserialize(std::fstream& file) {
            if (!file.is_open()) {
                throw std::runtime_error(
                    "PLOD: BvhStream::Unable to deserialize");
            }
            file.read((char*)&segment_id_, 4);
            file.read((char*)&depth_, 4);
            file.read((char*)&num_nodes_, 4);
            file.read((char*)&fan_factor_, 4);
            file.read((char*)&max_surfels_per_node_, 4);
            file.read((char*)&serialized_surfel_size_, 4);
            file.read((char*)&reserved_0_, 8);
            file.read((char*)&state_, 4);
            file.read((char*)&reserved_1_, 4);
            file.read((char*)&reserved_2_, 8);
            file.read((char*)&translation_.x_, 4);
            file.read((char*)&translation_.y_, 4);
            file.read((char*)&translation_.z_, 4);
            file.read((char*)&reserved_3_, 4);
        }
        
    };

    class BvhNodeSeg : public BvhSerializable {
    public:
        BvhNodeSeg()
        : BvhSerializable() {};
        ~BvhNodeSeg() {};
        
        uint32_t segment_id_;
        uint32_t node_id_;

        BvhVector centroid_;
        uint32_t depth_;
        
        float reduction_error_;
        float avg_surfel_radius_;
        
        BvhNodeVisibility visibility_;
        uint32_t reserved_;

        BvhBoundingBox bounding_box_;
        
    protected:
        friend class BvhStream;
        const size_t size() const {
            return 16*sizeof(uint32_t);
        };
        void signature(char* signature) {
            signature[0] = 'B';
            signature[1] = 'V';
            signature[2] = 'H';
            signature[3] = 'X';
            signature[4] = 'N';
            signature[5] = 'O';
            signature[6] = 'D';
            signature[7] = 'E';
        }
        void serialize(std::fstream& file) {
            if (!file.is_open()) {
                throw std::runtime_error(
                    "PLOD: BvhStream::Unable to serialize");
            }
            file.write((char*)&segment_id_, 4);
            file.write((char*)&node_id_, 4);
            file.write((char*)&centroid_.x_, 4);
            file.write((char*)&centroid_.y_, 4);
            file.write((char*)&centroid_.z_, 4);
            file.write((char*)&depth_, 4);
            file.write((char*)&reduction_error_, 4);
            file.write((char*)&avg_surfel_radius_, 4);
            file.write((char*)&visibility_, 4);
            file.write((char*)&reserved_, 4);
            file.write((char*)&bounding_box_.min_.x_, 4);
            file.write((char*)&bounding_box_.min_.y_, 4);
            file.write((char*)&bounding_box_.min_.z_, 4);
            file.write((char*)&bounding_box_.max_.x_, 4);
            file.write((char*)&bounding_box_.max_.y_, 4);
            file.write((char*)&bounding_box_.max_.z_, 4);
        }
        void deserialize(std::fstream& file) {
            if (!file.is_open()) {
                throw std::runtime_error(
                    "PLOD: BvhStream::Unable to deserialize");
            }
            file.read((char*)&segment_id_, 4);
            file.read((char*)&node_id_, 4);
            file.read((char*)&centroid_.x_, 4);
            file.read((char*)&centroid_.y_, 4);
            file.read((char*)&centroid_.z_, 4);
            file.read((char*)&depth_, 4);
            file.read((char*)&reduction_error_, 4);
            file.read((char*)&avg_surfel_radius_, 4);
            file.read((char*)&visibility_, 4);
            file.read((char*)&reserved_, 4);
            file.read((char*)&bounding_box_.min_.x_, 4);
            file.read((char*)&bounding_box_.min_.y_, 4);
            file.read((char*)&bounding_box_.min_.z_, 4);
            file.read((char*)&bounding_box_.max_.x_, 4);
            file.read((char*)&bounding_box_.max_.y_, 4);
            file.read((char*)&bounding_box_.max_.z_, 4);
        }

    };

    class BvhNodeExtensionSeg : public BvhSerializable {
    public:
        BvhNodeExtensionSeg()
        : BvhSerializable() {};
        ~BvhNodeExtensionSeg() {};

        uint32_t segment_id_;
        uint32_t node_id_;
        uint32_t empty_;
        uint32_t reserved_;
        BvhDiskArray disk_array_;

    protected:
        friend class BvhStream;
        const size_t size() const {
            return 10*sizeof(uint32_t);
        };
        void signature(char* signature) {
            signature[0] = 'B';
            signature[1] = 'V';
            signature[2] = 'H';
            signature[3] = 'X';
            signature[4] = 'N';
            signature[5] = 'E';
            signature[6] = 'X';
            signature[7] = 'T';
        }
        void serialize(std::fstream& file) {
            if (!file.is_open()) {
               throw std::runtime_error(
                   "PLOD: BvhStream::Unable to serialize");
            }
            file.write((char*)&segment_id_, 4);
            file.write((char*)&node_id_, 4);
            file.write((char*)&empty_, 4);
            file.write((char*)&reserved_, 4);
            file.write((char*)&disk_array_.disk_access_ref_, 4);
            file.write((char*)&disk_array_.reserved_, 4);
            file.write((char*)&disk_array_.offset_, 8);
            file.write((char*)&disk_array_.length_, 8);
        }
        void deserialize(std::fstream& file) {
            if (!file.is_open()) {
               throw std::runtime_error(
                   "PLOD: BvhStream::Unable to deserialize");
            }
            file.read((char*)&segment_id_, 4);
            file.read((char*)&node_id_, 4);
            file.read((char*)&empty_, 4);
            file.read((char*)&reserved_, 4);
            file.read((char*)&disk_array_.disk_access_ref_, 4);
            file.read((char*)&disk_array_.reserved_, 4);
            file.read((char*)&disk_array_.offset_, 8); 
            file.read((char*)&disk_array_.length_, 8);
        }
        
    };


    class BvhTreeExtensionSeg : public BvhSerializable {
    public:
        BvhTreeExtensionSeg()
        : BvhSerializable() {};
        ~BvhTreeExtensionSeg() {};

        uint32_t segment_id_;
        BvhString working_directory_;
        BvhString filename_;
        uint32_t num_disk_accesses_;
        std::vector<BvhString> disk_accesses_;

    protected:
        friend class BvhStream;
        const size_t size() const {
            //this takes some effort to compute since strings have arbitray length
            size_t size = 8;
            for (const auto& text : disk_accesses_) {
                size += 8 + text.length_ + (32 - ((8 + text.length_) % 32));
            }
            size += 8 + working_directory_.length_ + (32 - ((8 + working_directory_.length_) % 32));
            size += 8 + filename_.length_ + (32 - ((8 + filename_.length_) % 32));
            return size;
        };
        void signature(char* signature) {
            signature[0] = 'B';
            signature[1] = 'V';
            signature[2] = 'H';
            signature[3] = 'X';
            signature[4] = 'T';
            signature[5] = 'E';
            signature[6] = 'X';
            signature[7] = 'T';
        }
        void serialize(std::fstream& file) {
            if (!file.is_open()) {
               throw std::runtime_error(
                   "PLOD: BvhStream::Unable to serialize");
            }
            file.write((char*)&segment_id_, 4);
            serialize_string(file, working_directory_);
            serialize_string(file, filename_);
            file.write((char*)&num_disk_accesses_, 4);
            for (unsigned int i = 0; i < num_disk_accesses_; ++i) {
                const BvhString& disk_access = disk_accesses_[i];
                serialize_string(file, disk_access);
            }
        }
        void deserialize(std::fstream& file) {
            if (!file.is_open()) {
               throw std::runtime_error(
                   "PLOD: BvhStream::Unable to deserialize");
            }
            file.read((char*)&segment_id_, 4);
            deserialize_string(file, working_directory_);
            deserialize_string(file, filename_);
            file.read((char*)&num_disk_accesses_, 4);
            for (uint32_t i = 0; i < num_disk_accesses_; ++i) {
                 BvhString disk_access;
                 deserialize_string(file, disk_access);
                 disk_accesses_.push_back(disk_access);
            }
        }
        
    };
    
    void OpenStream(const std::string& bvh_filename,
                    const BvhStreamType type);
    void CloseStream(const bool remove_file);    
 
    void Write(BvhSerializable& serializable);


private:
    BvhStreamType type_;    
    std::string filename_;
    std::fstream file_;
    uint32_t num_segments_;
    

};

    


} } // namespace lamure


#endif // REN_BVH_STREAM__

