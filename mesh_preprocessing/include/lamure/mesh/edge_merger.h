
#ifndef LAMURE_EDGE_MERGER_H_
#define LAMURE_EDGE_MERGER_H_



#include <lamure/mesh/polyhedron.h>


// utility for merging border edges of a non-continuous mesh
// in order of angle between adjacent edges using a priority queue
//
// - in order to be merged, edges must belong to 2 adjacent faces
// - therefore merges of edges that share a face are not permitted
//

struct edge_merger
{

private:

	//checks if 2 face handles within a polyhedron are neighbours
	static bool faces_are_neighbours(lamure::mesh::Polyhedron& P, lamure::mesh::Facet_handle f1, lamure::mesh::Facet_handle f2){

	  typedef typename lamure::mesh::Polyhedron::Halfedge_around_facet_const_circulator HFCC;

	  int f1_id = f1->face_id;
	  bool are_neighbours = false;

	  HFCC hc = f2->facet_begin();
	  HFCC hc_end = hc;

	  //circulate around face 2 to check for shared edges with face 1
	  do {

	    if (!hc->opposite()->is_border()){
	      if (hc->opposite()->facet()->face_id == f1_id){
	        are_neighbours = true;
	        break;
	      }
	    }
	  } while( ++hc != hc_end);

	  return are_neighbours;
	}

	static lamure::mesh::Vec3 normalise(lamure::mesh::Vec3 v) {return v / std::sqrt(v.squared_length());}

	static bool point_equal_to_vec3(lamure::mesh::Point lhs, scm::math::vec3f rhs){
	  return (lhs.x() == rhs.x
	      && lhs.y() == rhs.y
	      && lhs.z() == rhs.z);
	}

public:

	static void merge_similar_border_edges(lamure::mesh::Polyhedron& P,
	                                    std::vector<lamure::mesh::Triangle_Chartid>& tri_list,
	                                    int target_merges
	                                    ){

	  struct Edge_merge_candidate
	  {
	    lamure::mesh::Edge_handle edge1;
	    lamure::mesh::Edge_handle edge2;
	    double cost;

	    int face1_id;
	    int face2_id;

	    static bool sort_edge_candidates (Edge_merge_candidate e1, Edge_merge_candidate e2) {
	      return (e1.cost < e2.cost);
	    }
	  };

	  //build list of possible edge merges
	  std::list<Edge_merge_candidate> merge_cs;

	  std::cout << "Searching " << P.size_of_halfedges() << " halfedges for merge candidates\n";

	  //check every edge in mesh, determine if it is a border edge
	  for ( lamure::mesh::Polyhedron::Halfedge_iterator  ei = P.halfedges_begin(); ei != P.halfedges_end(); ++ei) {

	    if(ei->is_border()){
	      Edge_merge_candidate mc;
	      mc.edge1 = ei;

	      //find next border edge, if there is only one candidate from next vertex
	      bool found = false;
	      lamure::mesh::Edge_handle edge2_candidate;

	      lamure::mesh::Polyhedron::Halfedge_around_vertex_circulator he,end;
	      he = end = ei->vertex()->vertex_begin();
	      do {

	        //if this halfedge is a border and not the starting edge
	        if(he->is_border()  &&  (he->edge_id != ei->edge_id)){

	          //if this is not the first border edge found...
	          if (found)
	          {
	            //stop searching
	            found = false;
	            break;
	          }
	          else { //if its the first, save it
	            edge2_candidate = he;
	            found = true;
	          }
	        }

	        //if opposite halfedge edge is border and not the starting edge
	        lamure::mesh::Edge_handle heopp = he->opposite();
	        if(heopp->is_border()  &&  (heopp->edge_id != ei->edge_id)){

	          //if this is not the first border edge found...
	          if (found)
	          {
	            //stop searching
	            found = false;
	            break;
	          }
	          else { //if its the first, save it
	            edge2_candidate = heopp;
	            found = true;
	          }
	        }
	      } while (++he != end);

	      //check if continuing edge was found only once, save to object if so
	      if (found){
	        mc.edge2 = edge2_candidate;
	      }
	      else //jump to next edge without adding to edge merge list
	      {
	        // std::cout << "No continuing edge or too many continuing edges found\n";
	        continue;
	      }

	      //calculate cost (angle difference)
	      //TODO - use area of subtended triangle as error measure?
	      lamure::mesh::Vec3 v1 = normalise(mc.edge1->vertex()->point() - mc.edge1->prev()->vertex()->point());
	      lamure::mesh::Vec3 v2 = normalise(mc.edge2->vertex()->point() - mc.edge2->prev()->vertex()->point());
	      double dot = v1 * v2;
	      dot = std::max(-1.0, std::min(1.0,dot));
	      mc.cost = acos(dot);

	      // std::cout << "[" << mc.edge1->edge_id << ", " << mc.edge2->edge_id << "]" << " cost = " << mc.cost << std::endl;

	      //determine faces that would be merged
	      //opposites used because border edges do not have references to faces (by definition)
	      mc.face1_id = mc.edge1->opposite()->facet()->face_id;
	      mc.face2_id = mc.edge2->opposite()->facet()->face_id;

	      if (mc.face1_id == mc.face2_id)
	      {
	        // std::cout << "edges of the same triangle found, not adding to merge list\n";
	      }
	      else if (!faces_are_neighbours(P, mc.edge1->opposite()->facet(), mc.edge2->opposite()->facet())){
	        // std::cout << "Faces of edges are not adjacent and cannot be easily merged. Not adding to merge list\n";
	      }
	      else {
	        merge_cs.push_back(mc);
	      }

	    }
	  }

	  double edge_face_ratio = (double)(merge_cs.size())/P.size_of_facets();

	  std::cout << "Found " << merge_cs.size() << " border edge merge candidates in mesh with " << P.size_of_facets() << " faces\n";
	  // std::cout << "Ratio = " << edge_face_ratio << std::endl;


	  //sort list into a queue
	  // std::cout << "sorting\n";
	  merge_cs.sort(Edge_merge_candidate::sort_edge_candidates);

	  int merges_executed = 0;

	  //execute merges, lowest cost first
	  while (merges_executed < target_merges
	        && !merge_cs.empty()){

	    //get first merge from queue
	    auto& merge = merge_cs.front();
	    merge_cs.pop_front();

	    // std::cout << "Executing merge " << merges_executed << " [" << merge.edge1->edge_id << ", " << merge.edge2->edge_id << "]" << "\n";

	    //get triangles to merge
	    std::map<int, lamure::mesh::vertex> new_tri_vertices;
	    auto& tri1 = tri_list[merge.face1_id];
	    auto& tri2 = tri_list[merge.face2_id];
	    lamure::mesh::Point doomed_vertex = merge.edge1->vertex()->point();

	    //add vertices from tri 1, if they are not doomed
	    for (int i = 0; i < 3; ++i)
	    {

	      //if not doomed point add to vector
	      if ( !point_equal_to_vec3(doomed_vertex, tri1.getVertex(i).pos_) )
	      {
	        new_tri_vertices[i] = tri1.getVertex(i);
	      }
	      if (new_tri_vertices.size() >= 2) {
	        // std::cout << " found 2 edges from tri1 \n";
	        break;
	      } // max size of 2 at this point
	    }
	    //add remaining vertex from tri2
	    for (int i = 0; i < 3; ++i)
	    {

	      //not doomed vertex
	      if (!point_equal_to_vec3(doomed_vertex, tri2.getVertex(i).pos_) )
	      {
	        //check it's not already in vertex list
	        bool found = false;
	        for (auto& v : new_tri_vertices)
	        {
	          if (v.second.pos_ == tri2.getVertex(i).pos_){
	            found = true;
	            break;
	          }
	        }
	        if (!found) // add if not already in list
	        {
	          //find position thats not already used
	          for (int j = 0; j < 3; ++j)
	          {
	            if (new_tri_vertices.find(j) == new_tri_vertices.end())
	            {
	              new_tri_vertices[j] = tri2.getVertex(i);
	              break;
	            }
	          }
	        }
	      }
	      if (new_tri_vertices.size() >= 3) {break;} // max size of 3 at this point
	    }


	    // add vertices to triangle 
	    for (int i = 0; i < 3; ++i)
	    {
	      tri1.setVertex(i, new_tri_vertices[i]);
	    }
	    //adjust old triangle to set as unused
	    tri2.chart_id = -1;

	    //update remaining merge candidates
	    for (auto mc = merge_cs.begin(); mc != merge_cs.end(); mc++){

	      //point references to removed face to remaining face
	      if(mc->face1_id == merge.face2_id ){
	        mc->face1_id = merge.face1_id;
	      }
	      if(mc->face2_id == merge.face2_id){
	        mc->face2_id = merge.face1_id;
	      }
	      //error checking
	      if (mc->face2_id == mc->face1_id)
	      {
	        // std::cout << "Error: 2 faces for merge have the same id\n";
	        merge_cs.erase(mc);
	      }
	    }

	    merges_executed++;
	  }//end while loop

	  std::cout << "Executed " << merges_executed << " merges\n";
	  
	  //remove unwanted triangles
	  int triangles_removed = 0;
	  for (auto it = tri_list.begin(); it != tri_list.end();){
	    if (it->chart_id == -1)
	    {
	      it = tri_list.erase(it);
	      triangles_removed++;
	    }
	    else {
	      ++it;
	    }
	  }
	  std::cout << "Removed " << triangles_removed << " triangles\n";


	}



};

#endif