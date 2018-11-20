#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

#include <CGAL/Simple_cartesian.h>
#include <CGAL/Polyhedron_3.h>


#include <CGAL/IO/print_wavefront.h>

//for obj export
#include <CGAL/IO/print_wavefront.h>

// Simplification function
#include <CGAL/Surface_mesh_simplification/edge_collapse.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/property_map.h>

// Stop-condition policy
#include <CGAL/Surface_mesh_simplification/Policies/Edge_collapse/Count_stop_predicate.h>
#include <CGAL/Surface_mesh_simplification/Policies/Edge_collapse/Edge_length_cost.h>
#include <CGAL/Surface_mesh_simplification/Policies/Edge_collapse/Midpoint_placement.h>
#include <CGAL/Surface_mesh_simplification/Policies/Edge_collapse/Constrained_placement.h>

#include<CGAL/Polyhedron_incremental_builder_3.h>


#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/trim.hpp>


#include <scm/core.h>
#include <scm/core/math.h>

#include "Utils.h"
#include "OBJ_printer.h"

#include <lamure/mesh/bvh.h>
#include <cstring>


typedef CGAL::Simple_cartesian<double> Kernel;
typedef CGAL::Polyhedron_3<Kernel> Polyhedron;
typedef Polyhedron::HalfedgeDS HalfedgeDS;
typedef Kernel::Point_3 Point;



namespace SMS = CGAL::Surface_mesh_simplification ;

//load an .obj file and return all vertices, normals and coords interleaved
void load_obj_into_triangles(const std::string& _file, std::vector<lamure::mesh::triangle_t>& triangles) {

    triangles.clear();

    std::vector<float> v;
    std::vector<uint32_t> vindices;
    std::vector<float> n;
    std::vector<uint32_t> nindices;
    std::vector<float> t;
    std::vector<uint32_t> tindices;

    // uint32_t num_tris = 0;

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

// implementatio of a Polyhedron_incremental_builder_3 to create the  polyhedron
//http://jamesgregson.blogspot.com/2012/05/example-code-for-building.html
template<class HDS>
class polyhedron_builder : public CGAL::Modifier_base<HDS> {

public:
  // std::vector<lamure::mesh::triangle_t> &left_tris_;


  std::vector<double> &vertices;
  std::vector<int>    &tris;



  polyhedron_builder( std::vector<double> &_vertices,
                      std::vector<int> &_tris ) 
                      : vertices(_vertices), tris(_tris)
                       {}

  // polyhedron_builder(
  //   std::vector<lamure::mesh::triangle_t>& left_child_tris) 
  //   : left_tris_(left_child_tris) {}


  // void operator()(HDS& hds) {
 
  //   // create a cgal incremental builder
  //   CGAL::Polyhedron_incremental_builder_3<HDS> B(hds, true);
  //   uint32_t num_tris = left_tris_.size();
  //   B.begin_surface(3*num_tris, num_tris);
    
  //   //add the polyhedron vertices
  //   // uint32_t vertex_id = 0;

  //   for (uint32_t i = 0; i < left_tris_.size(); ++i) {

  //     const lamure::mesh::triangle_t& tri = left_tris_[i];

  //     // B.add_vertex(XtndPoint<Kernel>(tri.v0_.pos_.x, tri.v0_.pos_.y, tri.v0_.pos_.z));
  //     // B.add_vertex(XtndPoint<Kernel>(tri.v1_.pos_.x, tri.v1_.pos_.y, tri.v1_.pos_.z));
  //     // B.add_vertex(XtndPoint<Kernel>(tri.v2_.pos_.x, tri.v2_.pos_.y, tri.v2_.pos_.z));

  //     B.add_vertex(Point(tri.v0_.pos_.x, tri.v0_.pos_.y, tri.v0_.pos_.z));
  //     B.add_vertex(Point(tri.v1_.pos_.x, tri.v1_.pos_.y, tri.v1_.pos_.z));
  //     B.add_vertex(Point(tri.v2_.pos_.x, tri.v2_.pos_.y, tri.v2_.pos_.z));

  //     // B.begin_facet();
  //     // B.add_vertex_to_facet(vertex_id);
  //     // B.add_vertex_to_facet(vertex_id+1);
  //     // B.add_vertex_to_facet(vertex_id+2);
  //     // B.end_facet();

  //     // vertex_id += 3;
  //   }

  //   for (uint32_t i = 0; i < left_tris_.size()*3; i+=3) {

  //     B.begin_facet();
  //     B.add_vertex_to_facet(i);
  //     B.add_vertex_to_facet(i+1);
  //     B.add_vertex_to_facet(i+2);
  //     B.end_facet();

  //     // vertex_id += 3;
  //   }

  //   B.end_surface();
  // }

  void operator()( HDS& hds) {

  typedef CGAL::Point_3<Kernel> Point;


  // create a cgal incremental builder
  CGAL::Polyhedron_incremental_builder_3<HDS> B( hds, true);
  B.begin_surface( vertices.size()/3, tris.size()/3 );
 
  // add the polyhedron vertices
  for( int i=0; i<(int)vertices.size() / 3; ++i ){

    B.add_vertex( Point( vertices[(i*3)], 
                                     vertices[(i*3)+1], 
                                     vertices[(i*3)+2]));
  }
 
  // add the polyhedron triangles
  for( int i=0; i<(int)(tris.size()); i+=3 ){


    B.begin_facet();
    B.add_vertex_to_facet( tris[i+0] );
    B.add_vertex_to_facet( tris[i+1] );
    B.add_vertex_to_facet( tris[i+2] );
    B.end_facet();
  }
 
  B.end_surface();
  }
};


int main( int argc, char** argv ) 
{


  std::string obj_filename = "../data/bunny.obj";
  
  bool terminate = false;
  
  if (Utils::cmdOptionExists(argv, argv+argc, "-f")) {
    obj_filename = Utils::getCmdOption(argv, argv+argc, "-f");
  }
  else {
    terminate = true;
  }

  std::string out_filename = "data/simplified_mesh_test.obj";
  if (Utils::cmdOptionExists(argv, argv+argc, "-o")) {
    out_filename = std::string(Utils::getCmdOption(argv, argv + argc, "-o"));
  }
  
  if (terminate) {
    std::cout << "Usage: " << argv[0] << "<flags>\n" <<
      "INFO: " << argv[0] << "\n" <<
      "\t-f: select .obj file\n" << 
      "\n";
    return 0;
  }

  //load the obj as triangles
  std::vector<lamure::mesh::triangle_t> triangles;
  load_obj_into_triangles(obj_filename, triangles);

  std::cout << "obj loaded as triangles" << std::endl;
  std::cout << "found " << triangles.size() << " triangles\n"; 

  //load OBJ into arrays
  std::vector<double> vertices;
  std::vector<int> tris;
  std::vector<double> t_coords;
  std::vector<int> tindices;
  Utils::load_obj( obj_filename, vertices, tris, t_coords, tindices);

  std::cout << "obj loaded as vectors" << std::endl;
  std::cout << "found " << tris.size() / 3 << " triangles\n"; 



  //build a polyhedron
  Polyhedron polyMesh;
  // polyhedron_builder<HalfedgeDS> builder(triangles);
  polyhedron_builder<HalfedgeDS> builder( vertices, tris );
  polyMesh.delegate(builder);

  if (polyMesh.is_valid(false) && CGAL::is_triangle_mesh(polyMesh)){
    std::cout << "triangle mesh valid" << std::endl;
  }
  std::cout << "triangle mesh has " << polyMesh.size_of_facets() << " faces " << std::endl;

  std::cout << "Starting simplification" << std::endl;

  SMS::Count_stop_predicate<Polyhedron> stop(4000);

  SMS::edge_collapse
            (polyMesh
            ,stop
             ,CGAL::parameters::vertex_index_map(get(CGAL::vertex_external_index,polyMesh)) 
                               .halfedge_index_map  (get(CGAL::halfedge_external_index  ,polyMesh))
                               .get_cost (SMS::Edge_length_cost <Polyhedron>())
                               .get_placement(SMS::Midpoint_placement<Polyhedron>())
            );

  std::cout << "\nFinished...\n" << polyMesh.size_of_facets() << " faces " << std::endl;

    //write to file
  std::ofstream ofs(out_filename);
  // OBJ_printer::print_polyhedron_wavefront_with_tex(ofs, polyMesh);
  CGAL::print_polyhedron_wavefront(ofs, polyMesh);
  ofs.close();
  std::cout << "poly mesh was written to " << out_filename << std::endl;





  return EXIT_SUCCESS ;
}