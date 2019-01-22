#include "CGAL_typedefs.h"

#include<CGAL/Polyhedron_incremental_builder_3.h>

template<class HDS>
class polyhedron_builder : public CGAL::Modifier_base<HDS> {

public:
  std::vector<double> &vertices;
  std::vector<int>    &tris;
  std::vector<double> &tcoords;
  std::vector<int>    &tindices;

  bool CHECK_VERTICES;

  polyhedron_builder( std::vector<double> &_vertices,
                      std::vector<int> &_tris,
                      std::vector<double> &_tcoords,
                      std::vector<int> &_tindices,
                      bool _CHECK_VERTICES) 
                      : vertices(_vertices), tris(_tris), tcoords(_tcoords), tindices(_tindices), CHECK_VERTICES(_CHECK_VERTICES)  {}


  //Returns false for error
  void operator()( HDS& hds) {


    // //if ids of vertex positions and texture coordinates do not match, then they are matched manually
    // //this only selects the first texture coordinate associated with a vertex position - doesn't support multiple tex coords that share a vertex position 
    // bool vertices_match = true;
    // if (CHECK_VERTICES)
    // {
    //   //check if vertex indices and tindices are the same
    //   if ( (vertices.size()/3) != (tcoords.size() / 2)) {std::cerr << "Input Texture coords not matched with input (different quantities)\n";
    //       vertices_match = false;}
    //   else {
    //     for (uint32_t i = 0; i < tindices.size(); ++i)
    //     {
    //       if (tindices[i] != tris[i]){
    //         std::cerr << "Input Texture coords not matched with input (different indices)\n";
    //         vertices_match = false;
    //         break;
    //       }
    //     }
    //   }
    // }
    // if (!vertices_match)
    // {

    //   //if no incoming coords, fil with 0s
    //   if (tcoords.size()==0)
    //   {
    //     for (uint32_t i = 0; i < ((vertices.size()/3)*2); ++i)
    //     {
    //       tcoords.push_back(0);
    //     }
    //   }
    //   else {

    //     //match vertex indexes to texture indexes to create new texture array
    //     std::vector<double> matched_tex_coords;

    //     //for each vertex entry
    //     for (uint32_t i = 0; i < (vertices.size() / 3); ++i)
    //     {
    //       //find first use of vertex in tris array
    //       for (uint32_t t = 0; t < tris.size(); ++t)
    //       {
    //         if (tris[t] == (int)i)
    //         {
    //           int target_face_vertex = t;
    //           int matching_tex_coord_idx = tindices[target_face_vertex];
    //           matched_tex_coords.push_back(tcoords[matching_tex_coord_idx*2]);
    //           matched_tex_coords.push_back(tcoords[(matching_tex_coord_idx*2) + 1]);
    //           break;
    //         }
    //       }
    //     }

    //     tcoords = matched_tex_coords;

    //     //check each vertex has coords
    //     if ((vertices.size()/3) == (tcoords.size() / 2)){
    //       std::cout << "matched texture coords with vertices\n";
    //     }
    //     else {
    //       std::cout << "failed to matched texture coords with vertices (" << (vertices.size()/3) << " vertices, " << (tcoords.size() / 2) << " coords)\n";
    //     }
    //   }
    // }

    // create a cgal incremental builder
    CGAL::Polyhedron_incremental_builder_3<HDS> B( hds, true);
    B.begin_surface( vertices.size()/3, tris.size()/3 );
   
    // add the polyhedron vertices
    for( int i=0; i<(int)vertices.size() / 3; ++i ){



      Vertex_handle vh = B.add_vertex( XtndPoint<Kernel>( vertices[(i*3)], 
                           vertices[(i*3)+1], 
                           vertices[(i*3)+2],
                            tcoords[i*2],
                            tcoords[(i*2)+1]));
      vh->id = i;

    }

    // add the polyhedron triangles
    for( int i=0; i<(int)(tris.size()); i+=3 ){ 

      Facet_handle fh = B.begin_facet();
      fh->id() = i / 3;

      //add tex coords
      TexCoord tc1 ( tcoords[ tindices[i+0]*2 ] , tcoords[ (tindices[i+0]*2)+1 ] );
      TexCoord tc2 ( tcoords[ tindices[i+1]*2 ] , tcoords[ (tindices[i+1]*2)+1 ] );
      TexCoord tc3 ( tcoords[ tindices[i+2]*2 ] , tcoords[ (tindices[i+2]*2)+1 ] );
      fh->add_tex_coords(tc1,tc2,tc3);

      B.add_vertex_to_facet( tris[i+0] );
      B.add_vertex_to_facet( tris[i+1] );
      B.add_vertex_to_facet( tris[i+2] );
      B.end_facet();
    }
   
    // finish up the surface
    B.end_surface();

  }

};