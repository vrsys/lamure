// ADAPTED FROM THE FOLLOWING:
// Copyright (c) 1997  ETH Zurich (Switzerland).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
// You can redistribute it and/or modify it under the terms of the GNU
// General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL$
// $Id$
// 
//
// Author(s)     : Lutz Kettner  <kettner@mpi-sb.mpg.de>

#include <CGAL/IO/File_writer_wavefront.h>
#include <CGAL/IO/generic_print_polyhedron.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/basic.h>
#include <CGAL/Inverse_index.h>

#include <iostream>



// class CGAL_EXPORT File_writer_wavefront_xtnd {
//     std::ostream*  m_out;
//     std::size_t    m_facets;
// public:
//     std::ostream& out() const { return *m_out; }
//     void write_header( std::ostream& out,
//                        std::size_t vertices,
//                        std::size_t halfedges,
//                        std::size_t facets);
//     void write_footer() const {
//         out() << "\n# End of Wavefront obj format #" << std::endl;
//     }
//     void write_vertex( const double& x, const double& y, const double& z) {
//         out() << "v " << x << ' ' << y << ' ' << z << '\n';
//     }
//     void write_facet_header() {
//         out() << "\n# " << m_facets << " facets\n";
//         out() << "# ------------------------------------------\n\n";
//     }
//     void write_facet_begin( std::size_t)            { out() << "f "; }
//     void write_facet_vertex_index( std::size_t idx) { out() << ' ' << idx+1; }
//     void write_facet_end()                          { out() << '\n'; }
// };

class File_writer_wavefront_xtnd : public CGAL::File_writer_wavefront {
public:
    void write_tex_coord( const double& u, const double& v) {
        out() << "vt " << u << ' ' << v << '\n';
    }
    void write_tex_coord_header(std::size_t tex_coords) {
        out() << "\n# " <<  tex_coords << " texture coordinates\n";
        out() << "# ------------------------------------------\n\n";
    }

    void write_facet_vertex_index( std::size_t idx, int tex_idx) {
    	out() << ' ' << idx+1 << "/" << tex_idx+1; 
    }

};

//u = ((1.0/chart.size()) * chart_id)

struct OBJ_printer
{
	template <class Polyhedron>
	static void print_polyhedron_wavefront_with_tex( std::ostream& out, const Polyhedron& P, std::map<uint32_t, int32_t> &chart_id_map) {
	    File_writer_wavefront_xtnd  writer;
	    generic_print_polyhedron( out, P, writer, chart_id_map);
	}

	template <class Polyhedron, class Writer>
	// template <class Polyhedron>
	static void
	generic_print_polyhedron( std::ostream&     out, 
	                          const Polyhedron& P,
	                          Writer&           writer,
	                          std::map<uint32_t, int32_t> &chart_id_map) {


	    // writes P to `out' in the format provided by `writer'.
	    typedef typename Polyhedron::Vertex_const_iterator                  VCI;
	    typedef typename Polyhedron::Facet_const_iterator                   FCI;
	    typedef typename Polyhedron::Halfedge_around_facet_const_circulator HFCC;
	    // Print header.
	    writer.write_header( out,
	                         P.size_of_vertices(),
	                         P.size_of_halfedges(),
	                         P.size_of_facets());
	    for( VCI vi = P.vertices_begin(); vi != P.vertices_end(); ++vi) {
	        writer.write_vertex( ::CGAL::to_double( vi->point().x()),
	                             ::CGAL::to_double( vi->point().y()),
	                             ::CGAL::to_double( vi->point().z()));
	    }

	    //tex coords

	    writer.write_tex_coord_header(P.size_of_facets());
	    //for( VCI vi = P.vertices_begin(); vi != P.vertices_end(); ++vi) {
	    for (uint32_t face = 0; face < P.size_of_facets(); ++face) {
	        double u = ((1.0/(double)P.size_of_facets()) * face);

	        writer.write_tex_coord(u,u);
	    }
	    typedef CGAL::Inverse_index< VCI> Index;
	    Index index( P.vertices_begin(), P.vertices_end());
	    writer.write_facet_header();

	    //faces
	    for( FCI fi = P.facets_begin(); fi != P.facets_end(); ++fi) {
	        HFCC hc = fi->facet_begin();
	        HFCC hc_end = hc;
	        std::size_t n = circulator_size( hc);
	        CGAL_assertion( idx+n >= 3);
	        writer.write_facet_begin( n);

	        int id = fi->id();
	        int chart_id = chart_id_map[id];
	        do {
	            writer.write_facet_vertex_index( index[ VCI(hc->vertex())], chart_id);
	           
	            ++hc;
	        } while( hc != hc_end);
	        writer.write_facet_end();
	    }
	    writer.write_footer();
	}

};

