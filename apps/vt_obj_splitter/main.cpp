// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <scm/core/math.h>

#include <lamure/types.h>
#include <lamure/mesh/triangle.h>
#include <lamure/mesh/old/kdtree.h>

#include <iostream>
#include <string>
#include <cstring>
#include <map>
#include <fstream>
#include <memory>

struct tile
{
    uint32_t x_;
    uint32_t y_;
    uint32_t width_;
    uint32_t height_;
    uint32_t rotated_;
};

static char* get_cmd_option(char** begin, char** end, const std::string& option)
{
    char** it = std::find(begin, end, option);
    if(it != end && ++it != end)
    {
        return *it;
    }
    return 0;
}

static bool cmd_option_exists(char** begin, char** end, const std::string& option) { return std::find(begin, end, option) != end; }

bool check_obj(const std::string& obj_filename, bool& has_coords, bool& has_normals)
{
    std::ifstream obj_file(obj_filename.c_str());
    if(!obj_file.is_open())
    {
        std::cout << "could not open .obj file: " << obj_filename << std::endl;
        return false;
    }


    std::string line;
    while(std::getline(obj_file, line))
    {
        if(line.length() >= 2)
        {
            if(line.substr(0, 2) == "vt")
            {
                has_coords = true;
            }
            if(line.substr(0, 2) == "vn")
            {
                has_normals = true;
            }
        }
    }

    obj_file.close();

    std::cout << "obj file does not contain texture coordinates: " << std::endl;
/*
    if(!has_coords)
    {

        return false;
    }
*/
    return true;
}

void load_obj(const std::string& obj_filename, std::vector<indexed_triangle_t>& triangles, bool has_coords, bool has_normals)
{
    triangles.clear();

    std::vector<double> v;
    std::vector<uint32_t> vindices;
    std::vector<double> n;
    std::vector<uint32_t> nindices;
    std::vector<double> t;
    std::vector<uint32_t> tindices;

    std::vector<std::string> materials;

    uint32_t num_tris = 0;

    FILE* file = fopen(obj_filename.c_str(), "r");

    if(0 != file)
    {
        std::string current_material = "";

        while(true)
        {
            char line_header[128];
            int32_t l = fscanf(file, "%s", line_header);

            if(l == EOF) {
                break;
            }
            
            if(strcmp(line_header, "v") == 0)
            {
                double vx, vy, vz;
                fscanf(file, "%lf %lf %lf\n", &vx, &vy, &vz);
                v.insert(v.end(), {vx, vy, vz});
            }
            else if(strcmp(line_header, "vn") == 0)
            {
                double nx, ny, nz;
                fscanf(file, "%lf %lf %lf\n", &nx, &ny, &nz);
                n.insert(n.end(), {nx, ny, nz});
            }
            else if(strcmp(line_header, "vt") == 0)
            {
                double tx, ty;
                fscanf(file, "%lf %lf\n", &tx, &ty);
                while (tx < -0.0001) { tx += 1.0; }
                while (ty < -0.0001) { ty += 1.0; }
                while (tx > 1.0001) { tx -= 1.0; }
                while (ty > 1.0001) { ty -= 1.0; }
                
                t.insert(t.end(), {tx, ty});

            }
            else if(strcmp(line_header, "f") == 0)
            {
                char line[240];
                fgets(line, 240, file);

                std::string line_str(line);

                line_str.erase(std::remove(line_str.begin(), line_str.end(), '\n'), line_str.end());
                line_str.erase(std::remove(line_str.begin(), line_str.end(), '\r'), line_str.end());
                if(line_str.find("/") == std::string::npos)
                {
                    continue;
                }

                std::string vertex1, vertex2, vertex3;
                uint32_t index[3];
                uint32_t coord[3];
                uint32_t normal[3];
                if(has_coords && has_normals)
                {
                    sscanf(line_str.c_str(), "%d/%d/%d %d/%d/%d %d/%d/%d", &index[0], &coord[0], &normal[0], &index[1], &coord[1], &normal[1], &index[2], &coord[2], &normal[2]);

                    vindices.insert(vindices.end(), {index[0], index[1], index[2]});
                    tindices.insert(tindices.end(), {coord[0], coord[1], coord[2]});
                    nindices.insert(nindices.end(), {normal[0], normal[1], normal[2]});
                }
                else if(has_coords)
                {
                    sscanf(line_str.c_str(), "%d/%d %d/%d %d/%d", &index[0], &coord[0], &index[1], &coord[1], &index[2], &coord[2]);

                    vindices.insert(vindices.end(), {index[0], index[1], index[2]});
                    tindices.insert(tindices.end(), {coord[0], coord[1], coord[2]});
                }
                else if(has_normals)
                {
                    sscanf(line_str.c_str(), "%d//%d %d//%d %d//%d", &index[0], &normal[0], &index[1], &normal[1], &index[2], &normal[2]);

                    vindices.insert(vindices.end(), {index[0], index[1], index[2]});
                    nindices.insert(nindices.end(), {normal[0], normal[1], normal[2]});
                }
                else
                {
                    sscanf(line_str.c_str(), "%d %d %d", &index[0], &index[1], &index[2]);

                    vindices.insert(vindices.end(), {index[0], index[1], index[2]});
                }
            }
        }

        fclose(file);

        std::cout << "obj positions: " << vindices.size() << std::endl;
        std::cout << "obj normals: " << nindices.size() << std::endl;
        std::cout << "obj coords: " << tindices.size() << std::endl;

        triangles.resize(vindices.size() / 3);

        uint32_t num_tris = 0;
        for(uint32_t i = 0; i < vindices.size() / 3; i++)
        {
            indexed_triangle_t tri;
            tri.tri_id_ = num_tris++;
            tri.tex_idx_ = 0;

            for(uint32_t j = 0; j < 3; ++j)
            {
                scm::math::vec3f position(v[3 * (vindices[3 * i + j] - 1)], v[3 * (vindices[3 * i + j] - 1) + 1], v[3 * (vindices[3 * i + j] - 1) + 2]);

                scm::math::vec3f normal(0.f, 1.f, 0.f);
                if(has_normals)
                {
                    normal = scm::math::vec3f(n[3 * (nindices[3 * i + j] - 1)], n[3 * (nindices[3 * i + j] - 1) + 1], n[3 * (nindices[3 * i + j] - 1) + 2]);
                }

                scm::math::vec2f coord{0.0f, 0.0f};
                if(has_coords) {
                    coord = scm::math::vec2f(t[2 * (tindices[3 * i + j] - 1)], t[2 * (tindices[3 * i + j] - 1) + 1]);
                }
                switch(j)
                {
                case 0:
                    tri.v0_.pos_ = position;
                    tri.v0_.nml_ = normal;
                    tri.v0_.tex_ = coord;
                    break;

                case 1:
                    tri.v1_.pos_ = position;
                    tri.v1_.nml_ = normal;
                    tri.v1_.tex_ = coord;
                    break;

                case 2:
                    tri.v2_.pos_ = position;
                    tri.v2_.nml_ = normal;
                    tri.v2_.tex_ = coord;
                    break;

                default:
                    break;
                }
            }
            triangles[i] = tri;
        }
    }
    else
    {
        std::cout << "failed to open obj file: " << obj_filename << std::endl;
        exit(1);
    }

    vindices.clear();
    tindices.clear();
    nindices.clear();
}

int32_t main(int argc, char* argv[])
{
    if(argc == 1 || !cmd_option_exists(argv, argv + argc, "-obj") || !cmd_option_exists(argv, argv + argc, "-t"))
    {
        std::cout << "Usage: " << argv[0] << " -obj -log -atlas" << std::endl
                  << "INFO: " << argv[0] << "\n"
                  << "-obj <required>: input .obj file" << std::endl
                  << "-t <required>: input num tris per kdtree node" << std::endl;
        return 0;
    }

    std::string obj_filename = get_cmd_option(argv, argv + argc, "-obj");
    bool has_coords  = false;
    bool has_normals = false;
    if(!check_obj(obj_filename, has_coords, has_normals))
    {
        exit(-1);
    }
    std::cout << "obj ok" << std::endl;

    std::cout << "loading obj..." << std::endl;
    std::vector<indexed_triangle_t> triangles;
    

    load_obj(obj_filename, triangles, has_coords, has_normals);
    std::cout << triangles.size() << " triangles loaded" << std::endl;


    uint32_t tris_per_node = atoi(get_cmd_option(argv, argv + argc, "-t"));
    if (tris_per_node < 512) tris_per_node = 512;
    std::cout << tris_per_node << " triangles per node" << std::endl;

 

    auto kdtree = std::make_shared<kdtree_t>(triangles, tris_per_node);
    
    const auto& nodes = kdtree->get_nodes();
    const auto& indices = kdtree->get_indices(); 

    std::cout << "writing " << nodes.size() << " obj files..." << std::endl;

    for (uint32_t node_id = kdtree->get_first_node_id_of_depth(kdtree->get_depth()); node_id < nodes.size(); ++node_id) {
      const auto& node = nodes[node_id];

      std::cout << "writing " << node.end_-node.begin_ << " << triangles from node id " << node_id << std::endl; 

      int sub_id = (int)node_id - (int)kdtree->get_first_node_id_of_depth(kdtree->get_depth());

      std::string obj_out_filename = obj_filename.substr(0, obj_filename.size() - 4) + "_sub"+std::to_string(sub_id)+".obj";
      std::ofstream obj_out_file(obj_out_filename.c_str());


      std::stringstream obj_out_str;
      obj_out_str << std::fixed << std::setprecision(10);

      for (uint32_t tri_id = node.begin_; tri_id < node.end_; ++tri_id) {
        
        auto& tri = triangles[indices[tri_id]];

        // write v      
        obj_out_str << "v " << tri.v0_.pos_.x << " " << tri.v0_.pos_.y << " " << tri.v0_.pos_.z << std::endl;
        obj_out_str << "v " << tri.v1_.pos_.x << " " << tri.v1_.pos_.y << " " << tri.v1_.pos_.z << std::endl;
        obj_out_str << "v " << tri.v2_.pos_.x << " " << tri.v2_.pos_.y << " " << tri.v2_.pos_.z << std::endl;
        

        if (has_normals) {
            //write vn
            obj_out_str << "vn " << tri.v0_.nml_.x << " " << tri.v0_.nml_.y << " " << tri.v0_.nml_.z << std::endl;
            obj_out_str << "vn " << tri.v1_.nml_.x << " " << tri.v1_.nml_.y << " " << tri.v1_.nml_.z << std::endl;
            obj_out_str << "vn " << tri.v2_.nml_.x << " " << tri.v2_.nml_.y << " " << tri.v2_.nml_.z << std::endl;
        }

        if(has_coords) {
          // write vt
          obj_out_str << "vt " << tri.v0_.tex_.x << " " << tri.v0_.tex_.y << std::endl;
          obj_out_str << "vt " << tri.v1_.tex_.x << " " << tri.v1_.tex_.y << std::endl;
          obj_out_str << "vt " << tri.v2_.tex_.x << " " << tri.v2_.tex_.y << std::endl;
        }

        obj_out_str << "g sub_vt_mesh" << std::endl;


        uint32_t local_tri_id = tri_id-node.begin_;

        // write f
        if (has_normals && has_coords) {
          obj_out_str << "f " << local_tri_id * 3 + 1 << "/" << local_tri_id * 3 + 1 << "/" << local_tri_id * 3 + 1;
          obj_out_str << " " << local_tri_id * 3 + 2 << "/" << local_tri_id * 3 + 2 << "/" << local_tri_id * 3 + 2;
          obj_out_str << " " << local_tri_id * 3 + 3 << "/" << local_tri_id * 3 + 3 << "/" << local_tri_id * 3 + 3 << std::endl;
        }
        else if (has_normals) {
          obj_out_str << "f " << local_tri_id * 3 + 1 << "//" << local_tri_id * 3 + 1;
          obj_out_str << " " << local_tri_id * 3 + 2 << "//" << local_tri_id * 3 + 2;
          obj_out_str << " " << local_tri_id * 3 + 3 << "//" << local_tri_id * 3 + 3 << std::endl;
        }
        else if (has_coords) {
          obj_out_str << "f " << local_tri_id * 3 + 1 << "/" << local_tri_id * 3 + 1;
          obj_out_str << " " << local_tri_id * 3 + 2 << "/" << local_tri_id * 3 + 2;
          obj_out_str << " " << local_tri_id * 3 + 3 << "/" << local_tri_id * 3 + 3 << std::endl;
        }
        else {
          obj_out_str << "f " << local_tri_id * 3 + 1; 
          obj_out_str << " " << local_tri_id * 3 + 2;
          obj_out_str << " " << local_tri_id * 3 + 3 << std::endl;
        }

      }


      obj_out_file << obj_out_str.str();

      obj_out_file.close();
      std::cout << "obj " << node_id << " written to " << obj_out_filename << std::endl;

    }

    std::cout << "Done. Have a nice day." << std::endl;


    return 0;
}
