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

  std::vector<int> &face_textures;

  polyhedron_builder( std::vector<double> &_vertices,
                      std::vector<int> &_tris,
                      std::vector<double> &_tcoords,
                      std::vector<int> &_tindices,
                      bool _CHECK_VERTICES,
                      std::vector<int> &_face_textures) 
                      : vertices(_vertices), 
                        tris(_tris), 
                        tcoords(_tcoords), 
                        tindices(_tindices), 
                        CHECK_VERTICES(_CHECK_VERTICES), 
                        face_textures(_face_textures)  {}


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


    uint32_t degenerate_faces = 0;

    //if there is different coordinates for every vertex, then we probably need to create an indexed vertex list
    if (tris.size()*3 == vertices.size())
    {
      std::cout << "Creating indexed vertex list from loaded obj\n";

      //create indexed vertex list
      std::vector<XtndPoint<Kernel> > vertices_indexed;
      std::vector<uint32_t> tris_indexed;
      create_indexed_triangle_list(tris_indexed, vertices_indexed);

      //add vertices of surface
      std::cout << "Polyhedron builder: adding vertices\n";
      for (uint32_t i = 0; i < vertices_indexed.size(); ++i) {
        Vertex_handle vh = B.add_vertex(vertices_indexed[i]);
        vh->id = i;
      }

      //create faces using vertex index references
      std::cout << "Polyhedron builder: adding faces\n";
      uint32_t face_count = 0;
      for (uint32_t i = 0; i < tris_indexed.size(); i+=3) {

        //check for degenerate faces:
        bool degenerate = is_face_degenerate(
                    Point( vertices_indexed[tris_indexed[i]] ),
                    Point( vertices_indexed[tris_indexed[i+1]] ),
                    Point( vertices_indexed[tris_indexed[i+2]] )
                    );

        if (!degenerate){

          Facet_handle fh = B.begin_facet();
          fh->id() = face_count++;

                    //add tex coords 
          TexCoord tc1 ( vertices_indexed[tris_indexed[i]].get_u() , vertices_indexed[tris_indexed[i]].get_v() );
          TexCoord tc2 ( vertices_indexed[tris_indexed[i+1]].get_u() , vertices_indexed[tris_indexed[i+1]].get_v() );
          TexCoord tc3 ( vertices_indexed[tris_indexed[i+2]].get_u() , vertices_indexed[tris_indexed[i+2]].get_v() );
          fh->add_tex_coords(tc1,tc2,tc3);

          //Add texture id for face
          if (face_textures.size() > 0)
          {
            fh->tex_id = face_textures[i/3];
          }

          B.add_vertex_to_facet(tris_indexed[i]);
          B.add_vertex_to_facet(tris_indexed[i+1]);
          B.add_vertex_to_facet(tris_indexed[i+2]);
          B.end_facet();
        }
        else {
          degenerate_faces++;
        }



      }
    }
    else {


      std::cout << "Loaded obj already indexed - creating polyhedron directly\n";

      std::cout << "Polyhedron builder: adding vertices\n";
      for( int i=0; i<(int)vertices.size() / 3; ++i ){

        Vertex_handle vh = B.add_vertex( XtndPoint<Kernel>( 
                             vertices[(i*3)], 
                             vertices[(i*3)+1], 
                             vertices[(i*3)+2],
                              tcoords[i*2],
                              tcoords[(i*2)+1]));
        vh->id = i;

      }

      // add the polyhedron triangles
      std::cout << "Polyhedron builder: adding faces\n";
      uint32_t face_count = 0;
      for( int i=0; i<(int)(tris.size()); i+=3 ){ 


        //check for degenerate faces:
        bool degenerate = is_face_degenerate(
                    Point( vertices[(tris[i+0]) * 3], vertices[((tris[i+0]) * 3) + 1], vertices[((tris[i+0]) * 3) + 2] ),
                    Point( vertices[(tris[i+1]) * 3], vertices[((tris[i+1]) * 3) + 1], vertices[((tris[i+1]) * 3) + 2] ),
                    Point( vertices[(tris[i+2]) * 3], vertices[((tris[i+2]) * 3) + 1], vertices[((tris[i+2]) * 3) + 2] )
                    );

        if(!degenerate){

          Facet_handle fh = B.begin_facet();
          fh->id() = face_count++;

          //add tex coords 
          TexCoord tc1 ( tcoords[ tindices[i+0]*2 ] , tcoords[ (tindices[i+0]*2)+1 ] );
          TexCoord tc2 ( tcoords[ tindices[i+1]*2 ] , tcoords[ (tindices[i+1]*2)+1 ] );
          TexCoord tc3 ( tcoords[ tindices[i+2]*2 ] , tcoords[ (tindices[i+2]*2)+1 ] );
          fh->add_tex_coords(tc1,tc2,tc3);

          //Add texture id for face
          if (face_textures.size() > 0)
          {
            fh->tex_id = face_textures[i/3];
          }

          B.add_vertex_to_facet( tris[i+0] );
          B.add_vertex_to_facet( tris[i+1] );
          B.add_vertex_to_facet( tris[i+2] );
          B.end_facet();
        }
        else {
          degenerate_faces++;
        }

      }
    }


   

   
    // finish up the surface
    std::cout << "Polyhedron builder: ending surface\n";
    std::cout << "Discarded " << degenerate_faces << " degenerate faces\n";
    std::cout << "---------------------------------------------\n";
    B.end_surface();

  }

  bool is_face_degenerate(Point v1, Point v2, Point v3){
    if (v1 == v2 || v1 == v3 || v2 == v3)
    {
      return true;
    }
    return false;
  }


    //creates indexed triangles list (indexes and vertices) from triangle list
  void create_indexed_triangle_list(std::vector<uint32_t>& tris_indexed,
                                    std::vector<XtndPoint<Kernel> >& vertices_indexed) {


    //for each vertex in original tris list
    for (uint32_t v = 0; v < tris.size() ; v++)
    {
        //get original vertex position and tex coord position in vertices array
        int tri_v = tris[v];
        int tex_c = tindices[v];

        //get Point value
        const XtndPoint<Kernel> tri_pnt = XtndPoint<Kernel>(
                vertices[(tri_v*3)+0], 
                vertices[(tri_v*3)+1], 
                vertices[(tri_v*3)+2],
                tcoords[(tex_c*2)+0],
                tcoords[(tex_c*2)+1]
                );

        //compare point with all previous vertices
        //go backwards, neighbours are more likely to be added at the end of the list
        uint32_t vertex_id = vertices_indexed.size();
        for (int32_t p = (vertices_indexed.size() - 1); p >= 0; --p)
        {
          //if a match is found, record index 
          if (XtndPoint<Kernel>::float_safe_equals(tri_pnt, vertices_indexed[p]))
          {
            vertex_id = p;
            break;
          }
        }
        //if no match then add to vertices list
        if (vertex_id == vertices_indexed.size()){
          vertices_indexed.push_back(tri_pnt);
        }
        //store index
        tris_indexed.push_back(vertex_id);
    }

  }

};