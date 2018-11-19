#include <iostream>
#include <algorithm>
#include <memory>

#include <lamure/mesh/bvh.h>
#include <cstring>


char* get_cmd_option(char** begin, char** end, const std::string& option) {
  char** it = std::find(begin, end, option);
  if (it != end && ++it != end) {
    return *it;
  }
  return 0;
}

bool cmd_option_exists(char** begin, char** end, const std::string& option) {
  return std::find(begin, end, option) != end;
}


//load an .obj file and return all vertices, normals and coords interleaved
void load_obj(const std::string& _file, std::vector<lamure::mesh::triangle_t>& triangles) {

    triangles.clear();

    std::vector<float> v;
    std::vector<uint32_t> vindices;
    std::vector<float> n;
    std::vector<uint32_t> nindices;
    std::vector<float> t;
    std::vector<uint32_t> tindices;

    uint32_t num_tris = 0;

    FILE *file = fopen(_file.c_str(), "r");

    if (0 != file) {

        while (true) {
            char line[128];
            int32_t l = fscanf(file, "%s", line);

            if (l == EOF) break;
            if (strcmp(line, "v") == 0) {
                float vx, vy, vz;
                fscanf(file, "%f %f %f\n", &vx, &vy, &vz);
                v.insert(v.end(), {vx, vy, vz});
            } else if (strcmp(line, "vn") == 0) {
                float nx, ny, nz;
                fscanf(file, "%f %f %f\n", &nx, &ny, &nz);
                n.insert(n.end(), {nx, ny, nz});
            } else if (strcmp(line, "vt") == 0) {
                float tx, ty;
                fscanf(file, "%f %f\n", &tx, &ty);
                t.insert(t.end(), {tx, ty});
            } else if (strcmp(line, "f") == 0) {
                std::string vertex1, vertex2, vertex3;
                uint32_t index[3];
                uint32_t coord[3];
                uint32_t normal[3];
                fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n",
                       &index[0], &coord[0], &normal[0],
                       &index[1], &coord[1], &normal[1],
                       &index[2], &coord[2], &normal[2]);

                vindices.insert(vindices.end(), {index[0], index[1], index[2]});
                tindices.insert(tindices.end(), {coord[0], coord[1], coord[2]});
                nindices.insert(nindices.end(), {normal[0], normal[1], normal[2]});
            }
        }

        fclose(file);

        std::cout << "positions: " << vindices.size() << std::endl;
        std::cout << "normals: " << nindices.size() << std::endl;
        std::cout << "coords: " << tindices.size() << std::endl;

        triangles.resize(nindices.size()/3);

        for (uint32_t i = 0; i < nindices.size()/3; i++) {
          lamure::mesh::triangle_t tri;
          for (uint32_t j = 0; j < 3; ++j) {
            
            scm::math::vec3f position(
                    v[3 * (vindices[3*i+j] - 1)], v[3 * (vindices[3*i+j] - 1) + 1], v[3 * (vindices[3*i+j] - 1) + 2]);

            scm::math::vec3f normal(
                    n[3 * (nindices[3*i+j] - 1)], n[3 * (nindices[3*i+j] - 1) + 1], n[3 * (nindices[3*i+j] - 1) + 2]);

            scm::math::vec2f coord(
                    t[2 * (tindices[3*i+j] - 1)], t[2 * (tindices[3*i+j] - 1) + 1]);

            
            switch (j) {
              case 0:
              tri.v0_.pos_ =  position;
              tri.v0_.nml_ = normal;
              tri.v0_.tex_ = coord;
              break;

              case 1:
              tri.v1_.pos_ =  position;
              tri.v1_.nml_ = normal;
              tri.v1_.tex_ = coord;
              break;

              case 2:
              tri.v2_.pos_ =  position;
              tri.v2_.nml_ = normal;
              tri.v2_.tex_ = coord;
              break;

              default:
              break;
            }
          }
          triangles[i] = tri;
        }

    } else {
        std::cout << "failed to open file: " << _file << std::endl;
        exit(1);
    }

}


int32_t main(int argc, char* argv[]) {

  std::string obj_filename = "../data/bunny.obj";
  
  bool terminate = false;
  
  if (cmd_option_exists(argv, argv+argc, "-f")) {
    obj_filename = get_cmd_option(argv, argv+argc, "-f");
  }
  else {
    terminate = true;
  }
  
  if (terminate) {
    std::cout << "Usage: " << argv[0] << "<flags>\n" <<
      "INFO: " << argv[0] << "\n" <<
      "\t-f: select .obj file\n" << 
      "\n";
    return 0;
  }

  std::vector<lamure::mesh::triangle_t> triangles;

  //load the obj as triangles
  load_obj(obj_filename, triangles);

  std::cout << "obj loaded" << std::endl;
  std::cout << "creating LOD hierarchy..." << std::endl;

  auto bvh = std::make_shared<lamure::mesh::bvh>(triangles, 1000);

  std::string bvh_filename = obj_filename.substr(0, obj_filename.size()-4)+".bvh";
  bvh->write_bvh_file(bvh_filename);
  std::cout << "Bvh file written to " << bvh_filename << std::endl;

  std::string lod_filename = obj_filename.substr(0, obj_filename.size()-4)+".lod";
  bvh->write_lod_file(lod_filename);
  std::cout << "Lod file written to " << lod_filename << std::endl;

  bvh.reset();

  return 0;
   
}