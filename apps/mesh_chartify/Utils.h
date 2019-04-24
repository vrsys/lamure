#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <limits>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "CGAL_typedefs.h"

#include <lamure/mesh/triangle.h>

#ifndef UTILSH
#define UTILSH

struct BoundingBoxLimits
{
  scm::math::vec3f min;
  scm::math::vec3f max;
};



struct Utils
{
	static char* getCmdOption(char ** begin, char ** end, const std::string & option) {
	  char ** itr = std::find(begin, end, option);
	  if (itr != end && ++itr != end) {
	      return *itr;
	  }
	  return 0;
	}

	static bool cmdOptionExists(char** begin, char** end, const std::string& option) {
	  return std::find(begin, end, option) != end;
	}

	//parses a face string like "f  2//1  8//1  4//1 " into 3 given arrays
	static void parse_face_string (std::string face_string, int (&index)[3], int (&coord)[3], int (&normal)[3]){

	  //split by space into faces
	  std::vector<std::string> faces;
	  boost::algorithm::trim(face_string);
	  boost::algorithm::split(faces, face_string, boost::algorithm::is_any_of(" "), boost::algorithm::token_compress_on);

	  for (int i = 0; i < 3; ++i)
	  {
	    //split by / for indices
	    std::vector<std::string> inds;
	    boost::algorithm::split(inds, faces[i], [](char c){return c == '/';}, boost::algorithm::token_compress_off);

	    for (int j = 0; j < (int)inds.size(); ++j)
	    {
	      int idx = 0;
	      //parse value from string
	      if (inds[j] != ""){
	        idx = stoi(inds[j]);
	      }
	      if (j == 0){index[i] = idx;}
	      else if (j == 1){coord[i] = idx;}
	      else if (j == 2){normal[i] = idx;}
	      
	    }
	  }
	}

	static Vector normalise(Vector v) {return v / std::sqrt(v.squared_length());}

	static Point midpoint(Point p1, Point p2){
		return Point(
			((p1.x() + p2.x()) / 2.0),
			((p1.y() + p2.y()) / 2.0),
			((p1.z() + p2.z()) / 2.0)
			);
	}

	// load obj function from vt_obj_loader/Utils.h
	static BoundingBoxLimits load_obj(const std::string& filename, 
	              std::vector<double> &v, 
	              std::vector<int> &vindices, 
	              std::vector<double> &t,
	              std::vector<int> &tindices,
	              std::vector<std::string> &materials){

	  scm::math::vec3f min_pos(std::numeric_limits<double>::max(),std::numeric_limits<double>::max(),std::numeric_limits<double>::max());
	  scm::math::vec3f max_pos(std::numeric_limits<double>::lowest(),std::numeric_limits<double>::lowest(),std::numeric_limits<double>::lowest());

	  FILE *file = fopen(filename.c_str(), "r");

	  if (0 != file) {


        std::string current_material = "";

	    while (true) {
	      char line[128];
	      int32_t l = fscanf(file, "%s", line);

	      if (l == EOF) break;
	      if (strcmp(line, "usemtl") == 0) {
	        char name[128];
	        fscanf(file, "%s\n", name);
	        current_material = std::string(name);
	        current_material.erase(std::remove(current_material.begin(), current_material.end(), '\n'), current_material.end());
	        current_material.erase(std::remove(current_material.begin(), current_material.end(), '\r'), current_material.end());
	        boost::trim(current_material);
	        std::cout << "obj switch material: " << current_material << std::endl;
	      }
	      else if (strcmp(line, "v") == 0) {
	        double vx, vy, vz;
	        fscanf(file, "%lf %lf %lf\n", &vx, &vy, &vz);

	        	        //scaling for zebra
	        // vx *= 0.01;
	        // vy *= 0.01;
	        // vz *= 0.01;


	        v.insert(v.end(), {vx, vy, vz});


	        //compare to find bounding box limits
	        if (vx > max_pos.x){max_pos.x = vx;}
	        else if (vx < min_pos.x){min_pos.x = vx;}
	        if (vy > max_pos.y){max_pos.y = vy;}
	        else if (vy < min_pos.y){min_pos.y = vy;}
	        if (vz > max_pos.z){max_pos.z = vz;}
	        else if (vz < min_pos.z){min_pos.z = vz;}


	      } 
	      // else if (strcmp(line, "vn") == 0) {
	      //   float nx, ny, nz;
	      //   fscanf(file, "%f %f %f\n", &nx, &ny, &nz);
	      //   n.insert(n.end(), {nx,ny, nz});
	      // } 
	      else if (strcmp(line, "vt") == 0) {
	        double tx, ty;
	        fscanf(file, "%lf %lf\n", &tx, &ty);
	        t.insert(t.end(), {tx, ty});
	      } 
	      else if (strcmp(line, "f") == 0) {
	        fgets(line, 128, file);
	        std::string face_string = line; 
	        int index[3];
	        int coord[3];
	        int normal[3];

	        parse_face_string(face_string, index, coord, normal);

	        //here all indices are decremented by 1 to fit 0 indexing schemes
	        vindices.insert(vindices.end(), {index[0]-1, index[1]-1, index[2]-1});
	        tindices.insert(tindices.end(), {coord[0]-1, coord[1]-1, coord[2]-1});
	        // nindices.insert(nindices.end(), {normal[0]-1, normal[1]-1, normal[2]-1});

	        materials.push_back(current_material);
	      }
	    }

	    fclose(file);

	    std::cout << "positions: " << v.size()/3 << std::endl;
	    // std::cout << "normals: " << n.size()/3 << std::endl;
	    std::cout << "coords: " << t.size()/2 << std::endl;
	    std::cout << "faces: " << vindices.size()/3 << std::endl;

	  }

	  BoundingBoxLimits bbox;
	  bbox.min = min_pos;
	  bbox.max = max_pos;

	  return bbox;

	}

	static bool load_mtl(const std::string& mtl_filename, std::map<std::string, std::pair<std::string, int> >& material_map) {

	    //parse .mtl file
	    std::cout << "loading .mtl file ..." << std::endl;
	    std::ifstream mtl_file(mtl_filename.c_str());
	    if (!mtl_file.is_open()) {
	      std::cout << "could not open .mtl file" << std::endl;
	      return false;
	    }

	    std::string current_material = "";
	    int material_index = 0;

	    std::string line;
	    while (std::getline(mtl_file, line)) {
	      boost::trim(line);
	      if(line.length() >= 2) {
	        if (line[0] == '#') {
	          continue;
	        }
	        if (line.substr(0, 6) == "newmtl") {
	          current_material = line.substr(7);
	          boost::trim(current_material);
	          current_material.erase(std::remove(current_material.begin(), current_material.end(), '\n'), current_material.end());
			  current_material.erase(std::remove(current_material.begin(), current_material.end(), '\r'), current_material.end());
	          std::cout << "found: " << current_material << std::endl;
	          material_map[current_material] = std::make_pair("",-1);
	        }
	        else if (line.substr(0, 6) == "map_Kd") {

	          std::string current_texture = line.substr(7);
	          current_texture.erase(std::remove(current_texture.begin(), current_texture.end(), '\n'), current_texture.end());
	          current_texture.erase(std::remove(current_texture.begin(), current_texture.end(), '\r'), current_texture.end());
	          boost::trim(current_texture);
	          
	          std::cout << current_material << " -> " << current_texture << ", " << material_index << std::endl;
	          material_map[current_material] =  std::make_pair(current_texture, material_index);
	          ++material_index;
	        }
	      }
	    
	    }

	    mtl_file.close();

	    return true;

	}

	//writes the texture id of each face of a polyhedron into a text file
	static void write_tex_id_file(Polyhedron &P, std::string tex_file_name) {
		//write chart file
	    std::ofstream ocfs( tex_file_name );
		for( Facet_iterator fi = P.facets_begin(); fi != P.facets_end(); ++fi) {    
	        ocfs << fi->tex_id << " ";
		}
		ocfs.close();
		std::cout << "Texture id per face file written to:  " << tex_file_name << std::endl;
	}


   //load an .obj file and return all vertices, normals and coords interleaved
    static void load_obj(const std::string& _file, std::vector<lamure::mesh::triangle_t>& triangles, std::vector<std::string>& materials) {

	    triangles.clear();

	    std::vector<float> v;
	    std::vector<uint32_t> vindices;
	    std::vector<float> n;
	    std::vector<uint32_t> nindices;
	    std::vector<float> t;
	    std::vector<uint32_t> tindices;

	    uint32_t num_tris = 0;

	    FILE *file = fopen(_file.c_str(), "r");

	    std::string current_material = "";

	    if (0 != file) {

	        while (true) {
	            char line[128];
	            int32_t l = fscanf(file, "%s", line);


		        if (l == EOF) break;
		        if (strcmp(line, "usemtl") == 0) {
		          char name[128];
		          fscanf(file, "%s\n", name);
		          current_material = std::string(name);
		          current_material.erase(std::remove(current_material.begin(), current_material.end(), '\n'), current_material.end());
		          current_material.erase(std::remove(current_material.begin(), current_material.end(), '\r'), current_material.end());
		          boost::trim(current_material);
		          std::cout << "obj switch material: " << current_material << std::endl;
		        }
		        else if (strcmp(line, "v") == 0) {
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

	                materials.push_back(current_material);
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

	    } else {
	        std::cout << "failed to open file: " << _file << std::endl;
	        exit(1);
    	}

	}


	//reads dimensions of png from header 
	static scm::math::vec2i get_png_dimensions(std::string filepath) { 
	    std::ifstream in(filepath);
	    uint32_t width, height;

	    in.seekg(16);
	    in.read((char *)&width, 4);
	    in.read((char *)&height, 4);

	    width = be32toh(width);
	    height = be32toh(height);

	    return scm::math::vec2i(width, height);
	}

};

#endif