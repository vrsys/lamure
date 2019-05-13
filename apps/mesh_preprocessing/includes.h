#ifndef LAMURE_MP_INCLUDES_H
#define LAMURE_MP_INCLUDES_H

/***
 * All app includes should be contained in this file (to reduce the clutter)
 *
 ***/

/*** Define macros early to rule out unnecessary includes! ***/

#define VCG_PARSER
// #define ADHOC_PARSER

#define MEASURE_EXECUTION_TIME

/***
 * Turn this flag on to enable saving app state before each stage.
 * Load it any time for examination like this:
 *
 * SparseOctree octree;
 * ifstream ifstream_tree(_input_path);
 * text_iarchive ia_tree(ifstream_tree);
 * ia_tree >> octree;
 *
 */
#define FLUSH_APP_STATE

// STL
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <chrono>
#include <limits>
#include <float.h>
#include <memory>
#include <iostream>
#include <vector>
#include <string>
#include <stack>

// OMP
#include <omp.h>

// CGAL
#include <CGAL/Polygon_mesh_processing/measure.h>
#include <CGAL/Polygon_mesh_processing/compute_normal.h>

// BOOST
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#ifdef FLUSH_APP_STATE
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/collections_load_imp.hpp>
#include <boost/serialization/collections_save_imp.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <experimental/filesystem>
#endif

// GL
#include <GL/glew.h>
#include <GL/freeglut.h>

// LAMURE
#include <lamure/mesh/tools.h>
#include <lamure/mesh/bvh.h>
#include <lamure/mesh/triangle.h>
#include <lamure/mesh/lodepng.h>

#ifdef VCG_PARSER
#include <vcg/complex/complex.h>
#include <vcg/complex/algorithms/update/texture.h>
#include <vcg/complex/algorithms/update/topology.h>
#include <vcg/complex/algorithms/update/bounding.h>
#include <vcg/complex/algorithms/update/flag.h>
#include <vcg/complex/algorithms/clean.h>
#include <vcg/space/intersection/triangle_triangle3.h>
#include <vcg/math/histogram.h>
#include <vcg/simplex/face/pos.h>
#include <vcg/complex/algorithms/inertia.h>
#include <vcg/space/index/grid_static_ptr.h>
#include <wrap/ply/plylib.h>
#include <wrap/io_trimesh/import_obj.h>
#include <wrap/io_trimesh/import.h>
#include <wrap/io_trimesh/export.h>
#endif

// TODO: none of these should remain here, none of these should be order-dependent

#include <lamure/mesh/old/cluster_settings.h>
#include <lamure/mesh/old/polyhedron_builder.h>
#include <lamure/mesh/old/SymMat.h>
#include <lamure/mesh/old/ErrorQuadric.h>
#include <lamure/mesh/old/Chart.h>
#include <lamure/mesh/old/JoinOperation.h>
#include <lamure/mesh/old/ClusterCreator.h>
#include <lamure/mesh/old/GridClusterCreator.h>
#include <lamure/mesh/old/ParallelClusterCreator.h>

#include <lamure/mesh/old/texture.h>
#include <lamure/mesh/old/frame_buffer.h>

#include <lamure/mesh/old/chart_packing.h>

#endif // LAMURE_MP_INCLUDES_H
