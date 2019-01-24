#include <stack>

typedef scm::math::vec<int, 3> vec3i;


struct GridClusterCreator
{

	static uint32_t create_grid_clusters(Polyhedron &P,
									 std::map<uint32_t, uint32_t> &chart_id_map,
									 BoundingBoxLimits limits,
									 int CELL_RES) {


		std::cout <<"Creating initial clusters with grid...\n";

		std::cout << "Received mesh with " << P.size_of_facets() << " faces\n";
		std::cout << "Limits: max = " << limits.max << ", min = " << limits.min << std::endl;

		//set cell size by dividing longest axis into given number of cells
		scm::math::vec3f range = limits.max - limits.min;
		double longest = range.x;
		if (range.y >= range.x && range.y >= range.z){longest = range.y;}
		else if (range.z >= range.y && range.z >= range.x){longest = range.z;}
		const double CELL_SIZE = (longest / CELL_RES) * 1.01;

		//store cell count in each dimension, for computing ids later
		vec3i cell_count (ceil(range.x / CELL_SIZE), ceil(range.y / CELL_SIZE), ceil(range.z / CELL_SIZE));

		std::cout << "Cell size = " << CELL_SIZE << std::endl;
		std::cout << "Volume size = " << range << std::endl;
		std::cout << "Cell count: " << cell_count << "\n----------" << std::endl;

		//map to count charts created and give sensible ids at the end of the process
		std::map<uint32_t, uint32_t> charts_created; //key = location id, value = chart id
		std::map<uint32_t, uint32_t>::iterator it;
		int chart_counter = 0;

		//for each face in the mesh, give it an id depending on grid position
		for ( Facet_iterator fb = P.facets_begin(); fb != P.facets_end(); ++fb)
		{
			//get centroid of face
			std::vector<Point> points;
			Halfedge_facet_circulator fc = fb->facet_begin();
	        do {
	        	points.push_back(fc->vertex()->point());
	        } while ( ++fc != fb->facet_begin());

			// scm::math::vec3f centroid_t = triangles[i].get_centroid();
			Point centroid_p = CGAL::centroid(points[0],points[1],points[2]); 
			scm::math::vec3f centroid_t (centroid_p.x(), centroid_p.y(), centroid_p.z());

			centroid_t -= limits.min; //move to object coords
			centroid_t /= CELL_SIZE; //scale to cell size

			vec3i location ((int)(centroid_t.x), (int)(centroid_t.y), (int)(centroid_t.z)); 

			//determine and store chart id by location
			int chart_id = location_to_id(location, cell_count);
			chart_id_map[fb->id()] = chart_id;

			// // std::cout << "location = " << location << std::endl;
			// // std::cout << "ID: " << id << std::endl;

			//check if this is a new chart (not found in list) - give it a count id if so
			it = charts_created.find(chart_id);
			if(it == charts_created.end())
			{
			   charts_created[chart_id] = chart_counter++;
			}
		}

		//for all triangles, swap location id for chart id
		for ( Facet_iterator fb = P.facets_begin(); fb != P.facets_end(); ++fb){
			int old_chart_id = chart_id_map[fb->id()];
			int new_chart_id = charts_created[old_chart_id];
			chart_id_map[fb->id()] = new_chart_id;
		}

// TODO complete
		uint32_t charts_added = correct_split_charts(P,chart_id_map, charts_created.size());
		std::cout << "correct_split_charts method added " << charts_added << " Charts" << std::endl;

		std::cout << "Created " <<  charts_created.size() + charts_added << " charts in total \n"; 

		return charts_created.size() + charts_added;

	}

	//seeks chart areas that are not connected, and assigns new ids to unconnected areas
	//returns number of charts added
	static uint32_t correct_split_charts(Polyhedron &P,
									 std::map<uint32_t, uint32_t> &chart_id_map,
									 uint32_t num_charts_created){

		//TODO check continuity of mesh groups - split charts can occur

		//start from one face, grow cluster by adding its neighbours to set
		//add their neighbours to the set
		//ideally all faces will be included in final set

		uint32_t faces_processed = 0;//debug only
		uint32_t additional_charts_overall = 0;
		//for each chart id
		for (uint32_t chart_id = 0; chart_id < num_charts_created; ++chart_id)
		{
			//build group of faces in chart from chart id map
			std::set<uint32_t> chart_faces;
			for ( Facet_iterator fb = P.facets_begin(); fb != P.facets_end(); ++fb){
				if (chart_id_map[fb->id()] == chart_id)
				{
					chart_faces.insert(fb->id());
				}
			}
			faces_processed += chart_faces.size();//debug only

			std::stack<Facet_handle> faces_to_process;
			std::set<uint32_t> processed_faces;

			uint32_t additional_charts_for_this_chart = 0;

			//TODO complete

			//pick first face in chart faces
			//get neighbours 
			//if nbr face is in chart_faces, but not in processed faces, add to faces to process stack

			//if stack is empty, and not all chart faces have been processed, then create repeat process from a different starting point

			while (processed_faces.size() < chart_faces.size()){

				//get starting face
				//must not be in processed faces, but be in faces
				for(uint32_t f : chart_faces){
					if (processed_faces.find(f) == processed_faces.end())//if not processed faces in set
					{
						//add this face as starting face
						Facet_iterator f_it = P.facets_begin();
						std::advance(f_it,f);

						//debug
						if (f_it->id() != (int)f) {
							std::cout << "[correct_split_charts] Error: face id is incorrect" << std::endl;
						}

						faces_to_process.push(f_it);
						break;
					}
				}

				//error checking only - stack should not be empty here
				if (faces_to_process.empty()){
					std::cout << "[correct_split_charts] Error: not all faces have been processed, but appropriate starting face was not found. (Chart " << chart_id << ")\n";
				}

				while (!faces_to_process.empty()){

					//get and remove top node
					Facet_handle top_node = faces_to_process.top();
					faces_to_process.pop();
					//recird that this face has been processed
					processed_faces.insert(top_node->id());

					//get neighbours of top node
					Halfedge_facet_circulator he = top_node->facet_begin();
				    do {
				      Facet_handle nbr_facet = he->opposite()->facet(); 
				      uint32_t nbr_facet_id = nbr_facet->id();

				      //if this neighbour appears in chart faces...
				      if (chart_faces.find(nbr_facet_id) != chart_faces.end())
				      {
				      	//and if its not in processed faces...
				      	if (processed_faces.find(nbr_facet_id) == processed_faces.end())
				      	{
				      		//then add to stack to process later
				      		faces_to_process.push(nbr_facet);

				      		//and adjust chart id accordingly
				      		//if this is found on the first pass, no action needed
				      		//otherwise new id needs to be calculated
				      		if (additional_charts_for_this_chart > 0)
				      		{
				      			uint32_t new_id = num_charts_created + additional_charts_overall + additional_charts_for_this_chart;
				      			chart_id_map[nbr_facet_id] = new_id;
				      		}
				      	}
				      }

				    } while ( ++he != top_node->facet_begin());


				} // end faces_to_process while


				additional_charts_for_this_chart++;

			}//end chart while
			
			additional_charts_overall += additional_charts_for_this_chart;

		}//end for chart

		if (P.size_of_facets() != faces_processed)//debug only
		{
			std::cout << "WARNING: [correct_split_charts] processed " << faces_processed << " faces, should have processed " << P.size_of_facets() << std::endl;
		}

		return additional_charts_overall;
	}

	//convert 3D cell reference to ID number
	static int location_to_id(vec3i& loc, vec3i& cell_count){
		return loc.x + (loc.y * cell_count.x) + (loc.z * (cell_count.x * cell_count.y));
	}

};