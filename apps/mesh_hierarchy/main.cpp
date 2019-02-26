#include <iostream>
#include <algorithm>
#include <memory>

#include <chrono>
#include <lamure/mesh/bvh.h>
#include <lamure/mesh/triangle_chartid.h>
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
void load_obj(const std::string& _file, std::vector<lamure::mesh::Triangle_Chartid>& triangles) {

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
          lamure::mesh::Triangle_Chartid tri;
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

//returns number of charts
//assumes charts are numbered sequentially
int load_chart_file(std::string chart_file, std::vector<int>& chart_id_per_triangle) {

  int num_charts = 0;

  std::ifstream file(chart_file);

  std::string line;
  while (std::getline(file, line)) {
    std::stringstream ss(line);
    std::string chart_id_str;

    while (std::getline(ss, chart_id_str, ' ')) {
      
      int chart_id = atoi(chart_id_str.c_str());
      num_charts = std::max(num_charts, chart_id+1);
      chart_id_per_triangle.push_back(chart_id);

      // std::cout << chart_id << std::endl;
    }
  }

  file.close();
  return num_charts;
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

  std::string chart_file = obj_filename.substr(0,obj_filename.length()-3).append(".chart");
  if (cmd_option_exists(argv, argv+argc, "-cf")) {
    chart_file = get_cmd_option(argv, argv+argc, "-cf");
  }
  
  uint32_t triangle_budget = 1000;
  if (cmd_option_exists(argv, argv+argc, "-t")) {
    triangle_budget = atoi(get_cmd_option(argv, argv+argc, "-t"));
  }
  
  if (terminate) {
    std::cout << "Usage: " << argv[0] << "<flags>\n" <<
      "INFO: " << argv[0] << "\n" <<
      "\t-f: select .obj file\n" << 
      "\t-cf: select .chart file (default: same name as obj file)\n" << 
      "\t-t: set triangle budget (=1000)\n" << 
      "\n";
    return 0;
  }


  // std::vector<lamure::mesh::triangle_t> triangles;
  std::vector<lamure::mesh::Triangle_Chartid> triangles;

  //load the obj as triangles
  load_obj(obj_filename, triangles);
  const uint32_t num_faces = triangles.size();

  std::cout << "obj loaded" << std::endl;

  //checking UV propogation
  //   for (auto& tri : triangles)
  // {
  //   std::cout << tri.to_string() << std::endl;
  // }
  // return 0;

  // load chart ids for triangles
  std::vector<int> chart_id_per_triangle;
  int num_charts = load_chart_file(chart_file, chart_id_per_triangle);
  std::cout << "Loaded chart ids for " << chart_id_per_triangle.size() << " triangles, (" << num_charts << " charts) from file: " << chart_file << std::endl;
  //check compatibility of chart and obj file
  if (chart_id_per_triangle.size() < triangles.size())
  {
    std::cout << "Error: charts were not found for every triangle\n";
    return 0; 

    //to debug loading problem
    // std::cout << "adding arbitrary extra charts\n";
    // while (chart_id_per_triangle.size() < triangles.size()){
    //   chart_id_per_triangle.push_back(0);
    // }
  }
  //assign chart ids to triangle vector
  for (uint32_t i = 0; i < triangles.size(); ++i)
  {
    triangles[i].chart_id = chart_id_per_triangle[i];
  }


  auto start_time = std::chrono::system_clock::now();

  std::cout << "creating LOD hierarchy..." << std::endl;

  auto bvh = std::make_shared<lamure::mesh::bvh>(triangles, triangle_budget);

  std::string bvh_filename = obj_filename.substr(0, obj_filename.size()-4)+".bvh";
  bvh->write_bvh_file(bvh_filename);
  std::cout << "Bvh file written to " << bvh_filename << std::endl;

  std::string lod_filename = obj_filename.substr(0, obj_filename.size()-4)+".lod";
  bvh->write_lod_file(lod_filename);
  std::cout << "Lod file written to " << lod_filename << std::endl;
  
  std::string lod_chart_filename = obj_filename.substr(0, obj_filename.size()-4)+".lodchart";
  bvh->write_chart_lod_file(lod_chart_filename);
  std::cout << "Lod chart file written to " << lod_chart_filename << std::endl;

  bvh.reset();


  //Logging
  auto time = std::chrono::system_clock::now();
  std::chrono::duration<double> diff = time - start_time;
  std::time_t now_c = std::chrono::system_clock::to_time_t(time);
  std::string log_path = "../../data/logs/bvh_creation_log.txt";
  std::ofstream ofs;
  ofs.open (log_path, std::ofstream::out | std::ofstream::app);
  ofs << "\n-------------------------------------\n";
  ofs << "Executed at " << std::put_time(std::localtime(&now_c), "%F %T") << std::endl;
  ofs << "Ran for " << (int)diff.count() / 60 << " m "<< (int)diff.count() % 60 << " s" << (int)(diff.count() * 1000) % 1000 << " ms " << std::endl;
  ofs << "Input file: " << obj_filename << "\nOutput bvh file: " << bvh_filename << std::endl;
  ofs << "Input file faces: " << num_faces << std::endl;
  ofs << "Triangle budget: " << triangle_budget << std::endl;
  ofs.close();
  std::cout << "Log written to " << log_path << std::endl;

  return 0;
   
}

