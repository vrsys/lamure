// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PROV_AUX_STREAM_H_
#define PROV_AUX_STREAM_H_

#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <cstring>

#include <lamure/types.h>
#include <lamure/prov/aux.h>

#include <scm/core/math.h>
#include <scm/gl_core/math.h>

namespace lamure {
namespace prov {

class aux_stream
{

public:
    aux_stream();
    ~aux_stream();


    enum aux_stream_type {
        AUX_STREAM_IN = 0,
        AUX_STREAM_OUT = 1
    };

    const aux_stream_type type() const { return type_; };
    const std::string filename() const { return filename_; };

    void read_aux(const std::string& filename, aux& aux);
    void write_aux(const std::string& filename, aux& aux);


protected:

    struct aux_vec {
      float x_ = 0.f;
      float y_ = 0.f;
      float z_ = 0.f;
    };

    struct aux_quat {
      float w_ = 1.f;
      float x_ = 0.f;
      float y_ = 0.f;
      float z_ = 0.f;
    };

    struct aux_string {
      uint64_t length_ = 0;
      std::string string_ = "";
    };

    struct aux_feature { //feature in image
      uint32_t camera_id_ = 0;
      uint32_t using_count_ = 0;
      float img_x_ = 0.f;
      float img_y_ = 0.f;
      float error_x_ = 0.f;
      float error_y_ = 0.f;
      uint32_t reserved_0_ = 0;
      uint32_t reserved_1_ = 0;
    };

    struct aux_sparse_point { //sparse world point
      float x_ = 0.f;
      float y_ = 0.f;
      float z_ = 0.f;
      uint8_t r_ = (uint8_t)0;
      uint8_t g_ = (uint8_t)0;
      uint8_t b_ = (uint8_t)0;
      uint8_t a_ = (uint8_t)255;
      float reserved_0_ = 0;
      float reserved_1_ = 0;
      float reserved_2_ = 0;
      float num_features_ = 0;
      std::vector<aux_feature> features_;
    };
  
    class aux_serializable {
    public:
        ~aux_serializable() {};
        size_t data_offset_;
    protected:
        friend class aux_stream;
        aux_serializable() {};
        virtual const size_t size() const = 0;
        virtual void signature(char* signature) = 0;
        virtual void serialize(std::fstream& file) = 0;
        virtual void deserialize(std::fstream& file) = 0;

        void serialize_string(std::fstream& file, const aux_string& text) {
            if (!file.is_open()) {
                throw std::runtime_error(
                    "PROV: aux_stream::Unable to serialize");
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
        void deserialize_string(std::fstream& file, aux_string& text) {
            if (!file.is_open()) {
                throw std::runtime_error(
                    "PROV: aux_stream::Unable to deserialize");
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

    class aux_sig : public aux_serializable {
    public:
        aux_sig()
        : aux_serializable() {};
        ~aux_sig() {};
        char signature_[8];
        size_t reserved_;
        size_t allocated_size_;
        size_t used_size_;
    protected:
        friend class aux_stream;
        const size_t size() const {
            return 8*sizeof(uint32_t);
        }
        void signature(char* signature) {};
        void serialize(std::fstream& file) {
             if (!file.is_open()) {
                 throw std::runtime_error(
                     "PROV: aux_stream::Unable to serialize");
             }
             file.write(signature_, 8);
             file.write((char*)&reserved_, 8);
             file.write((char*)&allocated_size_, 8);
             file.write((char*)&used_size_, 8);
        }
        void deserialize(std::fstream& file) {
             if (!file.is_open()) {
                 throw std::runtime_error(
                     "PROV: aux_stream::Unable to deserialize");
             }
             for (uint32_t i = 0; i < 8; ++i) {
                 file.read(&signature_[i], 1);
             }
             file.read((char*)&reserved_, 8);
             file.read((char*)&allocated_size_, 8);
             file.read((char*)&used_size_, 8);
        }
    };

    class aux_file_seg : public aux_serializable {
    public:
        aux_file_seg()
        : aux_serializable() {};
        ~aux_file_seg() {};

        uint32_t major_version_;
        uint32_t minor_version_;
        size_t reserved_;

    protected:
        friend class aux_stream;
        const size_t size() const {
            return 4*sizeof(uint32_t);
        }
        void signature(char* signature) {
            signature[0] = 'A';
            signature[1] = 'U';
            signature[2] = 'X';
            signature[3] = 'X';
            signature[4] = 'F';
            signature[5] = 'I';
            signature[6] = 'L';
            signature[7] = 'E';
        }
        void serialize(std::fstream& file) {
            if (!file.is_open()) {
                throw std::runtime_error(
                    "PROV: aux_stream::Unable to serialize");
            }
            file.write((char*)&major_version_, 4);
            file.write((char*)&minor_version_, 4);
            file.write((char*)&reserved_, 8);
        }
        void deserialize(std::fstream& file) {
            if (!file.is_open()) {
                throw std::runtime_error(
                    "PROV: aux_stream::Unable to deserialize");
            }
            file.read((char*)&major_version_, 4);
            file.read((char*)&minor_version_, 4);
            file.read((char*)&reserved_, 8);
        }
    };

    

    class aux_sparse_seg : public aux_serializable {
    public:
        aux_sparse_seg()
        : aux_serializable() {};
        ~aux_sparse_seg() {};

        uint32_t segment_id_;
        uint32_t reserved_0_;
        
        uint32_t reserved_1_;
        uint32_t reserved_2_;

        uint32_t reserved_3_;
        uint32_t reserved_4_;

        uint64_t num_points_;
        std::vector<aux_sparse_point> points_;

    protected:
        friend class aux_stream;
        const size_t size() const {
            size_t seg_size = 8*sizeof(uint32_t);
            for (uint64_t i = 0; i < num_points_; ++i) {
              seg_size += 8*sizeof(uint32_t) + points_[i].num_features_*sizeof(aux_feature);
            }
            return seg_size;
        }
        void signature(char* signature) {
            signature[0] = 'A';
            signature[1] = 'U';
            signature[2] = 'X';
            signature[3] = 'X';
            signature[4] = 'S';
            signature[5] = 'P';
            signature[6] = 'R';
            signature[7] = 'S';
        }
        void serialize(std::fstream& file) {
            if (!file.is_open()) {
                throw std::runtime_error(
                    "PROV: aux_stream::Unable to serialize");
            }
            file.write((char*)&segment_id_, 4);
            file.write((char*)&reserved_0_, 4);
            file.write((char*)&reserved_1_, 4);
            file.write((char*)&reserved_2_, 4);
            file.write((char*)&reserved_3_, 4);
            file.write((char*)&reserved_4_, 4);
            file.write((char*)&num_points_, 8);

            for (uint64_t i = 0; i < num_points_; ++i) {
              file.write((char*)&points_[i].x_, 4);
              file.write((char*)&points_[i].y_, 4);
              file.write((char*)&points_[i].z_, 4);
              file.write((char*)&points_[i].r_, 1);
              file.write((char*)&points_[i].g_, 1);
              file.write((char*)&points_[i].b_, 1);
              file.write((char*)&points_[i].a_, 1);
              file.write((char*)&points_[i].reserved_0_, 4);
              file.write((char*)&points_[i].reserved_1_, 4);
              file.write((char*)&points_[i].reserved_2_, 4);
              file.write((char*)&points_[i].num_features_, 4);
              
              for (uint32_t j = 0; j < points_[i].num_features_; ++j) {
                file.write((char*)&points_[i].features_[j].camera_id_, 4);
                file.write((char*)&points_[i].features_[j].using_count_, 4);
                file.write((char*)&points_[i].features_[j].img_x_, 4);
                file.write((char*)&points_[i].features_[j].img_y_, 4);
                file.write((char*)&points_[i].features_[j].error_x_, 4);
                file.write((char*)&points_[i].features_[j].error_y_, 4);
                file.write((char*)&points_[i].features_[j].reserved_0_, 4);
                file.write((char*)&points_[i].features_[j].reserved_1_, 4);
                
              }

            }
  
        } 
        void deserialize(std::fstream& file) {
            if (!file.is_open()) {
                throw std::runtime_error(
                    "PROV: aux_stream::Unable to deserialize");
            }
            file.read((char*)&segment_id_, 4);
            file.read((char*)&reserved_0_, 4);
            file.read((char*)&reserved_1_, 4);
            file.read((char*)&reserved_2_, 4);
            file.read((char*)&reserved_3_, 4);
            file.read((char*)&reserved_4_, 4);
            file.read((char*)&num_points_, 8);

            for (uint64_t i = 0; i < num_points_; ++i) {
              aux_sparse_point p;
              file.read((char*)&p.x_, 4);
              file.read((char*)&p.y_, 4);
              file.read((char*)&p.z_, 4);
              file.read((char*)&p.r_, 1);
              file.read((char*)&p.g_, 1);
              file.read((char*)&p.b_, 1);
              file.read((char*)&p.a_, 1);
              file.read((char*)&p.reserved_0_, 4);
              file.read((char*)&p.reserved_1_, 4);
              file.read((char*)&p.reserved_2_, 4);
              file.read((char*)&p.num_features_, 4);

              for (uint32_t j = 0; j < p.num_features_; ++j) {
                aux_feature f;
                file.read((char*)&f.camera_id_, 4);
                file.read((char*)&f.using_count_, 4);
                file.read((char*)&f.img_x_, 4);
                file.read((char*)&f.img_y_, 4);
                file.read((char*)&f.error_x_, 4);
                file.read((char*)&f.error_y_, 4);
                file.read((char*)&f.reserved_0_, 4);
                file.read((char*)&f.reserved_1_, 4);
                p.features_.push_back(f);
              }

              points_.push_back(p);
            }

        }
        
    };




    class aux_view_seg : public aux_serializable { // 1 per camera
    public:
        aux_view_seg()
        : aux_serializable() {};
        ~aux_view_seg() {};
        
        uint32_t segment_id_;
        uint32_t camera_id_;

        aux_vec position_;
        uint32_t reserved_0_;

        aux_quat orientation_;
        
        float focal_length_;
        float distortion_;

        uint32_t reserved_1_;
        uint32_t reserved_2_;

        uint32_t reserved_3_;
        uint32_t reserved_4_;

        uint32_t reserved_5_;
        uint32_t reserved_6_;

        uint32_t image_width_;
        uint32_t image_height_;

        uint32_t tex_atlas_id_;
        uint32_t reserved_7_;

        uint32_t reserved_8_;
        uint32_t reserved_9_;

        aux_string camera_name_;
        aux_string image_file_;
        
        
    protected:
        friend class aux_stream;
        const size_t size() const {
            size_t seg_size = 24*sizeof(uint32_t);
            seg_size += 8 + camera_name_.length_ + (32 - ((8 + camera_name_.length_) % 32));
            seg_size += 8 + image_file_.length_ + (32 - ((8 + image_file_.length_) % 32));
            return seg_size;
        };
        void signature(char* signature) {
            signature[0] = 'A';
            signature[1] = 'U';
            signature[2] = 'X';
            signature[3] = 'X';
            signature[4] = 'V';
            signature[5] = 'I';
            signature[6] = 'E';
            signature[7] = 'W';
        }
        void serialize(std::fstream& file) {
            if (!file.is_open()) {
                throw std::runtime_error(
                    "PROV: aux_stream::Unable to serialize");
            }
            file.write((char*)&segment_id_, 4);
            file.write((char*)&camera_id_, 4);
            file.write((char*)&position_.x_, 4);
            file.write((char*)&position_.y_, 4);
            file.write((char*)&position_.z_, 4);
            file.write((char*)&reserved_0_, 4);
            file.write((char*)&orientation_.x_, 4);
            file.write((char*)&orientation_.y_, 4);
            file.write((char*)&orientation_.z_, 4);
            file.write((char*)&orientation_.w_, 4);
            file.write((char*)&focal_length_, 4);
            file.write((char*)&distortion_, 4);
            file.write((char*)&reserved_1_, 4);
            file.write((char*)&reserved_2_, 4);
            file.write((char*)&reserved_3_, 4);
            file.write((char*)&reserved_4_, 4);
            file.write((char*)&reserved_5_, 4);
            file.write((char*)&reserved_6_, 4);
            file.write((char*)&image_width_, 4);
            file.write((char*)&image_height_, 4);
            file.write((char*)&tex_atlas_id_, 4);
            file.write((char*)&reserved_7_, 4);
            file.write((char*)&reserved_8_, 4);
            file.write((char*)&reserved_9_, 4);

            serialize_string(file, camera_name_);
            serialize_string(file, image_file_);

        }
        void deserialize(std::fstream& file) {
            if (!file.is_open()) {
                throw std::runtime_error(
                    "PROV: aux_stream::Unable to deserialize");
            }

            file.read((char*)&segment_id_, 4);
            file.read((char*)&camera_id_, 4);
            file.read((char*)&position_.x_, 4);
            file.read((char*)&position_.y_, 4);
            file.read((char*)&position_.z_, 4);
            file.read((char*)&reserved_0_, 4);
            file.read((char*)&orientation_.x_, 4);
            file.read((char*)&orientation_.y_, 4);
            file.read((char*)&orientation_.z_, 4);
            file.read((char*)&orientation_.w_, 4);
            file.read((char*)&focal_length_, 4);
            file.read((char*)&distortion_, 4);
            file.read((char*)&reserved_1_, 4);
            file.read((char*)&reserved_2_, 4);
            file.read((char*)&reserved_3_, 4);
            file.read((char*)&reserved_4_, 4);
            file.read((char*)&reserved_5_, 4);
            file.read((char*)&reserved_6_, 4);
            file.read((char*)&image_width_, 4);
            file.read((char*)&image_height_, 4);
            file.read((char*)&tex_atlas_id_, 4);
            file.read((char*)&reserved_7_, 4);
            file.read((char*)&reserved_8_, 4);
            file.read((char*)&reserved_9_, 4);

            deserialize_string(file, camera_name_);
            deserialize_string(file, image_file_);

        }

    };


    
    void open_stream(const std::string& aux_filename,
                    const aux_stream_type type);
    void close_stream(const bool remove_file);    
 
    void write(aux_serializable& serializable);


private:
    aux_stream_type type_;    
    std::string filename_;
    std::fstream file_;
    uint32_t num_segments_;
    

};

    


} } // namespace lamure


#endif // PROV_AUX_STREAM__

