// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <scm/core/math.h>

#include <lamure/types.h>
//#include <lamure/mesh/cgal_mesh.h>
#include <lamure/vt/pre/AtlasFile.h>
#include <lamure/mesh/triangle.h>

#include <iostream>
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

bool check_obj(const std::string& obj_filename, bool& has_normals)
{
    std::ifstream obj_file(obj_filename.c_str());
    if(!obj_file.is_open())
    {
        std::cout << "could not open .obj file: " << obj_filename << std::endl;
        return false;
    }

    bool has_coords = false;

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

    if(!has_coords)
    {
        std::cout << "obj file does not contain texture coordinates: " << std::endl;
        return false;
    }

    /*std::string mtl_filename = obj_filename.substr(0, obj_filename.size()-4)+".mtl";
    std::ifstream mtl_file(mtl_filename.c_str());
    if (!mtl_file.is_open()) {
      std::cout << "could not open .mtl file: " << mtl_filename << std::endl;
      return false;
    }

    mtl_file.close();*/

    return true;
}

void load_obj(const std::string& obj_filename, std::string& mtl_filename, std::vector<lamure::mesh::triangle_t>& triangles, std::vector<std::string>& material_per_triangle, bool has_normals)
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

            if(l == EOF)
                break;
            if(strcmp(line_header, "usemtl") == 0)
            {
                char name[128];
                fscanf(file, "%s\n", name);
                current_material = std::string(name);
                current_material.erase(std::remove(current_material.begin(), current_material.end(), '\n'), current_material.end());
                current_material.erase(std::remove(current_material.begin(), current_material.end(), '\r'), current_material.end());
                std::cout << "obj switch material: " << current_material << std::endl;
            }
            if(strcmp(line_header, "mtllib") == 0)
            {
                char name[128];
                fscanf(file, "%s\n", name);

                // size_t found_slash;
                // found_slash = obj_filename.find_last_of("/\\");

                mtl_filename = /*obj_filename.substr(0, found_slash) + "/" +*/ std::string(name);

                if(mtl_filename.find("./") != std::string::npos)
                {
                    size_t start_pos = mtl_filename.find("./");
                    mtl_filename.replace(start_pos, std::string("./").length(), "");
                }

                mtl_filename.erase(std::remove(mtl_filename.begin(), mtl_filename.end(), '\n'), mtl_filename.end());
                mtl_filename.erase(std::remove(mtl_filename.begin(), mtl_filename.end(), '\r'), mtl_filename.end());
                std::cout << "mtl file: " << mtl_filename << std::endl;
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
                if(has_normals)
                {
                    sscanf(line_str.c_str(), "%d/%d/%d %d/%d/%d %d/%d/%d", &index[0], &coord[0], &normal[0], &index[1], &coord[1], &normal[1], &index[2], &coord[2], &normal[2]);

                    vindices.insert(vindices.end(), {index[0], index[1], index[2]});
                    tindices.insert(tindices.end(), {coord[0], coord[1], coord[2]});
                    nindices.insert(nindices.end(), {normal[0], normal[1], normal[2]});

                    materials.push_back(current_material);
                }
                else
                {
                    sscanf(line_str.c_str(), "%d/%d %d/%d %d/%d", &index[0], &coord[0], &index[1], &coord[1], &index[2], &coord[2]);

                    vindices.insert(vindices.end(), {index[0], index[1], index[2]});
                    tindices.insert(tindices.end(), {coord[0], coord[1], coord[2]});

                    materials.push_back(current_material);
                }
            }
        }

        fclose(file);

        std::cout << "obj positions: " << vindices.size() << std::endl;
        std::cout << "obj normals: " << nindices.size() << std::endl;
        std::cout << "obj coords: " << tindices.size() << std::endl;

        triangles.resize(vindices.size() / 3);
        material_per_triangle.resize(vindices.size() / 3);

        for(uint32_t i = 0; i < vindices.size() / 3; i++)
        {
            lamure::mesh::triangle_t tri;
            for(uint32_t j = 0; j < 3; ++j)
            {
                scm::math::vec3f position(v[3 * (vindices[3 * i + j] - 1)], v[3 * (vindices[3 * i + j] - 1) + 1], v[3 * (vindices[3 * i + j] - 1) + 2]);

                scm::math::vec3f normal(0.f, 1.f, 0.f);
                if(has_normals)
                {
                    normal = scm::math::vec3f(n[3 * (nindices[3 * i + j] - 1)], n[3 * (nindices[3 * i + j] - 1) + 1], n[3 * (nindices[3 * i + j] - 1) + 2]);
                }

                scm::math::vec2f coord(t[2 * (tindices[3 * i + j] - 1)], t[2 * (tindices[3 * i + j] - 1) + 1]);

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
            material_per_triangle[i] = materials[i];
        }
    }
    else
    {
        std::cout << "failed to open obj file: " << obj_filename << std::endl;
        exit(1);
    }

    materials.clear();
    vindices.clear();
    tindices.clear();
    nindices.clear();
}

void load_mtl(const std::string& mtl_filename, std::map<std::string, std::string>& material_map)
{
    // parse .mtl file
    std::cout << "loading .mtl file ..." << std::endl;
    std::ifstream mtl_file(mtl_filename.c_str());
    if(!mtl_file.is_open())
    {
        std::cout << "could not open .mtl file" << std::endl;
        exit(-1);
    }

    std::string current_material = "";

    std::string line;
    while(std::getline(mtl_file, line))
    {
        if(line.length() >= 2)
        {
            if(line[0] == '#')
            {
                continue;
            }
            if(line.find("newmtl") != std::string::npos)
            {
                current_material = line;

                size_t start_pos = current_material.find("newmtl");
                current_material.replace(start_pos, std::string("newmtl").length(), "");

                current_material.erase(std::remove(current_material.begin(), current_material.end(), ' '), current_material.end());
                current_material.erase(std::remove(current_material.begin(), current_material.end(), '\n'), current_material.end());
                current_material.erase(std::remove(current_material.begin(), current_material.end(), '\r'), current_material.end());
                material_map[current_material] = "";
            }
            else if(line.find("map_Kd") != std::string::npos)
            {
                std::string current_texture = line;

                size_t start_pos = current_texture.find("map_Kd");
                current_texture.replace(start_pos, std::string("map_Kd").length(), "");

                current_texture.erase(std::remove(current_texture.begin(), current_texture.end(), ' '), current_texture.end());
                current_texture.erase(std::remove(current_texture.begin(), current_texture.end(), '\n'), current_texture.end());
                current_texture.erase(std::remove(current_texture.begin(), current_texture.end(), '\r'), current_texture.end());
                std::cout << current_material << " -> " << current_texture << std::endl;
                material_map[current_material] = current_texture;
            }
        }
    }

    mtl_file.close();
}

template <class InIt, class OutIt>
void split(InIt begin, InIt end, OutIt splits, char delim)
{
    InIt current = begin;
    while(begin != end)
    {
        if(*begin == delim)
        {
            *splits++ = std::string(current, begin);
            current = ++begin;
        }
        else
            ++begin;
    }
    *splits++ = std::string(current, begin);
}

scm::math::vec2ui load_log(const std::string& log_filename, std::map<std::string, tile>& tile_map)
{
    std::ifstream log_file(log_filename.c_str());

    if(!log_file.is_open())
    {
        std::cout << "could not open .log file" << std::endl;
        exit(-1);
    }

    uint32_t atlas_width = 0;
    uint32_t atlas_height = 0;

    std::string line = "";
    uint32_t line_no = 0;
    while(std::getline(log_file, line))
    {
        if(line.length() >= 2)
        {
            if(line[0] == '#')
            {
                continue;
            }
            std::vector<std::string> values;
            split(line.begin(), line.end(), std::back_inserter(values), ',');
            if(line_no > 0)
            {
                tile t{atoi(values[1].c_str()), atoi(values[2].c_str()), atoi(values[3].c_str()), atoi(values[4].c_str()), atoi(values[5].c_str())};
                tile_map[values[values.size() - 1]] = t;
                std::cout << values[values.size() - 1] << " tile: " << t.x_ << " " << t.y_ << " " << t.width_ << " " << t.height_ << " " << t.rotated_ << std::endl;
            }
            else
            {
                atlas_width = atoi(values[0].c_str());
                atlas_height = atoi(values[1].c_str());
            }
        }
        ++line_no;
    }

    log_file.close();

    return scm::math::vec2ui(atlas_width, atlas_height);
}

int32_t main(int argc, char* argv[])
{
    if(argc == 1 || !cmd_option_exists(argv, argv + argc, "-obj") || !cmd_option_exists(argv, argv + argc, "-atlas"))
    {
        std::cout << "Usage: " << argv[0] << " -obj -log -atlas" << std::endl
                  << "INFO: " << argv[0] << "\n"
                  << "-obj <required>: input .obj file (must have .mtl file in same directory)" << std::endl
                  << "-atlas <required>: input .atlas file" << std::endl
                  << "-log <optional>: input .log file (to accomodate for collection of images in one atlas)" << std::endl;
        return 0;
    }

    std::string obj_filename = get_cmd_option(argv, argv + argc, "-obj");
    bool has_normals = false;
    if(!check_obj(obj_filename, has_normals))
    {
        exit(-1);
    }
    std::cout << "obj ok" << std::endl;

    std::cout << "loading obj..." << std::endl;
    std::vector<lamure::mesh::triangle_t> triangles;
    std::vector<std::string> material_per_triangle;
    std::string mtl_filename;
    load_obj(obj_filename, mtl_filename, triangles, material_per_triangle, has_normals);
    std::cout << triangles.size() << " triangles loaded" << std::endl;

    std::cout << "loading mtl... " << mtl_filename << std::endl;
    // std::string mtl_filename = obj_filename.substr(0, obj_filename.size()-4)+".mtl";
    std::map<std::string, std::string> material_map;
    load_mtl(mtl_filename, material_map);
    std::cout << material_map.size() << " materials loaded" << std::endl;

    if(cmd_option_exists(argv, argv + argc, "-log"))
    {
        std::cout << "loading log..." << std::endl;
        std::string log_filename = get_cmd_option(argv, argv + argc, "-log");

        std::map<std::string, tile> tile_map;
        scm::math::vec2ui atlas = load_log(log_filename, tile_map);
        std::cout << tile_map.size() << " tiles loaded" << std::endl;

        // std::cout << "atlas width: " << atlas_width << std::endl;
        // std::cout << "atlas height: " << atlas_height << std::endl;

        std::cout << "scaling texture coordinates..." << std::endl;

        for(uint32_t i = 0; i < triangles.size(); ++i)
        {
            const std::string& mat = material_per_triangle[i];

            tile tile{0, 0, 0, 0};
            if(material_map.find(mat) != material_map.end())
            {
                const std::string& texture = material_map[mat];
                if(tile_map.find(texture) != tile_map.end())
                {
                    tile = tile_map[texture];
                    // std::cout << mat << " tile: " << tile.x_ << " " << tile.y_ << " " << tile.width_ << " " << tile.height_ << " " << tile.rotated_ << std::endl;
                }
                else
                {
                    std::cout << "tile was not found" << std::endl;
                    exit(1);
                }
            }
            else
            {
                std::cout << "material was not found" << std::endl;
                exit(1);
            }

            /*if (tile.rotated_) {
              std::cout << "rotated tiles are not implemented" << std::endl;
              exit(1);
            }
            else {*/
            uint32_t image_dim = std::max(atlas.x, atlas.y);

            lamure::mesh::triangle_t& tri = triangles[i];

            double scaled_u, scaled_v;

            scaled_u = ((double)(tile.x_) + (tri.v0_.tex_.x) * (double)tile.width_) / (double)atlas.x;
            scaled_v = ((double)(tile.y_) + (tri.v0_.tex_.y) * (double)tile.height_) / (double)atlas.y;
            tri.v0_.tex_.x = std::min(0.999999999, std::max(0.0000001, scaled_u));
            tri.v0_.tex_.y = std::min(0.999999999, std::max(0.0000001, scaled_v));

            scaled_u = ((double)(tile.x_) + (tri.v1_.tex_.x) * (double)tile.width_) / (double)atlas.x;
            scaled_v = ((double)(tile.y_) + (tri.v1_.tex_.y) * (double)tile.height_) / (double)atlas.y;
            tri.v1_.tex_.x = std::min(0.999999999, std::max(0.0000001, scaled_u));
            tri.v1_.tex_.y = std::min(0.999999999, std::max(0.0000001, scaled_v));

            scaled_u = ((double)(tile.x_) + (tri.v2_.tex_.x) * (double)tile.width_) / (double)atlas.x;
            scaled_v = ((double)(tile.y_) + (tri.v2_.tex_.y) * (double)tile.height_) / (double)atlas.y;
            tri.v2_.tex_.x = std::min(0.999999999, std::max(0.0000001, scaled_u));
            tri.v2_.tex_.y = std::min(0.999999999, std::max(0.0000001, scaled_v));

            /*}*/
        }
    }

    std::cout << "scaling texture coordinates..." << std::endl;

    std::string atlas_filename = get_cmd_option(argv, argv + argc, "-atlas");
    std::shared_ptr<vt::pre::AtlasFile> atlas_file;
    try
    {
        atlas_file = std::make_shared<vt::pre::AtlasFile>(atlas_filename.c_str());
    }
    catch(std::runtime_error& error)
    {
        std::cout << "could not open atlas file" << std::endl;
        exit(-1);
    }

    uint64_t image_width = atlas_file->getImageWidth();
    uint64_t image_height = atlas_file->getImageHeight();

    // tile's width and height without padding
    uint64_t tile_inner_width = atlas_file->getInnerTileWidth();
    uint64_t tile_inner_height = atlas_file->getInnerTileHeight();

    // Quadtree depth counter, ranges from 0 to depth-1
    uint64_t depth = atlas_file->getDepth();

    atlas_file.reset();

    double factor_u = (double)image_width / (tile_inner_width * std::pow(2, depth - 1));
    double factor_v = (double)image_height / (tile_inner_height * std::pow(2, depth - 1));

    double factor_max = std::max(factor_u, factor_v);

    std::cout << "img width: " << image_width << ", factor_u: " << factor_u << std::endl;
    std::cout << "img height: " << image_height << ", factor_v: " << factor_v << std::endl;

#if 1
    for(auto& tri : triangles)
    {
        tri.v0_.tex_.x = (tri.v0_.tex_.x) * factor_u;
        tri.v0_.tex_.y = 1.0 - ((1.0 - tri.v0_.tex_.y) * factor_v);
        tri.v1_.tex_.x = (tri.v1_.tex_.x) * factor_u;
        tri.v1_.tex_.y = 1.0 - ((1.0 - tri.v1_.tex_.y) * factor_v);
        tri.v2_.tex_.x = (tri.v2_.tex_.x) * factor_u;
        tri.v2_.tex_.y = 1.0 - ((1.0 - tri.v2_.tex_.y) * factor_v);
    }
#else
    for(auto& tri : triangles)
    {
        tri.v0_.tex_.x = (tri.v0_.tex_.x) * factor_u;
        tri.v0_.tex_.y = (tri.v0_.tex_.y) * factor_v;
        tri.v1_.tex_.x = (tri.v1_.tex_.x) * factor_u;
        tri.v1_.tex_.y = (tri.v1_.tex_.y) * factor_v;
        tri.v2_.tex_.x = (tri.v2_.tex_.x) * factor_u;
        tri.v2_.tex_.y = (tri.v2_.tex_.y) * factor_v;
    }
#endif

    std::cout << "writing obj..." << std::endl;

    std::string obj_out_filename = obj_filename.substr(0, obj_filename.size() - 4) + "_vt.obj";
    std::ofstream obj_out_file(obj_out_filename.c_str());

    std::stringstream obj_out_str;
    obj_out_str << std::fixed << std::setprecision(10);
    obj_out_str << "mtllib atlas.mtl" << std::endl;

    // write v
    for(const auto& tri : triangles)
    {
        obj_out_str << "v " << tri.v0_.pos_.x << " " << tri.v0_.pos_.y << " " << tri.v0_.pos_.z << std::endl;
        obj_out_str << "v " << tri.v1_.pos_.x << " " << tri.v1_.pos_.y << " " << tri.v1_.pos_.z << std::endl;
        obj_out_str << "v " << tri.v2_.pos_.x << " " << tri.v2_.pos_.y << " " << tri.v2_.pos_.z << std::endl;
    }

#if 1
    if (has_normals) {
        //write vn
        for (const auto& tri : triangles) {
          obj_out_str << "vn " << tri.v0_.nml_.x << " " << tri.v0_.nml_.y << " " << tri.v0_.nml_.z << std::endl;
          obj_out_str << "vn " << tri.v1_.nml_.x << " " << tri.v1_.nml_.y << " " << tri.v1_.nml_.z << std::endl;
          obj_out_str << "vn " << tri.v2_.nml_.x << " " << tri.v2_.nml_.y << " " << tri.v2_.nml_.z << std::endl;
        }
    }
#endif

    // write vt
    for(const auto& tri : triangles)
    {
        obj_out_str << "vt " << tri.v0_.tex_.x << " " << tri.v0_.tex_.y << std::endl;
        obj_out_str << "vt " << tri.v1_.tex_.x << " " << tri.v1_.tex_.y << std::endl;
        obj_out_str << "vt " << tri.v2_.tex_.x << " " << tri.v2_.tex_.y << std::endl;
    }

    obj_out_str << "g vt_mesh" << std::endl;
    obj_out_str << "usemtl atlas" << std::endl;

    // write f
    for(uint32_t i = 0; i < triangles.size(); ++i)
    {
        if (has_normals) {
          obj_out_str << "f " << i * 3 + 1 << "/" << i * 3 + 1 << "/" << i * 3 + 1;
          obj_out_str << " " << i * 3 + 2 << "/" << i * 3 + 2 << "/" << i * 3 + 2;
          obj_out_str << " " << i * 3 + 3 << "/" << i * 3 + 3 << "/" << i * 3 + 3 << std::endl;
        }
        else {
          obj_out_str << "f " << i * 3 + 1 << "/" << i * 3 + 1;
          obj_out_str << " " << i * 3 + 2 << "/" << i * 3 + 2;
          obj_out_str << " " << i * 3 + 3 << "/" << i * 3 + 3 << std::endl;
        }
    }

    obj_out_file << obj_out_str.str();

    obj_out_file.close();
    std::cout << "obj written to " << obj_out_filename << std::endl;

#if 1

#else
    if(!has_normals)
    {
        std::cout << "computing normals ..." << std::endl;

        lamure::mesh::Polyhedron polyhedron;
        lamure::mesh::polyhedron_builder<lamure::mesh::HalfedgeDS> builder(triangles);
        polyhedron.delegate(builder);

        triangles.clear();

        if(polyhedron.is_valid(false) && CGAL::is_triangle_mesh(polyhedron))
        {
        }
        else
        {
            std::cout << "Cgal says triangle mesh invalid" << std::endl;
            exit(-1);
        }

        // TODO: compute normals

        // convert back to triangle soup
        uint32_t num_vertices_simplified = 0;
        for(lamure::mesh::Polyhedron::Facet_iterator f = polyhedron.facets_begin(); f != polyhedron.facets_end(); ++f)
        {
            lamure::mesh::Polyhedron::Halfedge_around_facet_circulator c = f->facet_begin();

            lamure::mesh::triangle_t tri;

            for(int i = 0; i < 3; ++i, ++c)
            {
                switch(i)
                {
                case 0:
                    tri.v0_.pos_ = scm::math::vec3f(c->vertex()->point()[0], c->vertex()->point()[1], c->vertex()->point()[2]);
                    tri.v0_.tex_ = scm::math::vec2f(c->vertex()->point().get_u(), c->vertex()->point().get_v());
                    tri.v0_.nml_ = scm::math::vec3f(0.f, 1.f, 0.f);
                    break;

                case 1:
                    tri.v1_.pos_ = scm::math::vec3f(c->vertex()->point()[0], c->vertex()->point()[1], c->vertex()->point()[2]);
                    tri.v1_.tex_ = scm::math::vec2f(c->vertex()->point().get_u(), c->vertex()->point().get_v());
                    tri.v1_.nml_ = scm::math::vec3f(0.f, 1.f, 0.f);
                    break;

                case 2:
                    tri.v2_.pos_ = scm::math::vec3f(c->vertex()->point()[0], c->vertex()->point()[1], c->vertex()->point()[2]);
                    tri.v2_.tex_ = scm::math::vec2f(c->vertex()->point().get_u(), c->vertex()->point().get_v());
                    tri.v2_.nml_ = scm::math::vec3f(0.f, 1.f, 0.f);
                    break;

                default:
                    break;
                }
            }

            triangles.push_back(tri);
        }

        std::cout << "normals computed." << std::endl;
    }
#endif

    return 0;
}
