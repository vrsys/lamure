
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
		// correct_split_charts(P,chart_id_map, charts_created.size());

		std::cout << "Created " <<  charts_created.size() << " charts \n"; 

		return charts_created.size();

	}

	static void correct_split_charts(Polyhedron &P,
									 std::map<uint32_t, uint32_t> &chart_id_map,
									 uint32_t num_charts_created){

		//TODO check continuity of mesh groups - split charts can occur

		//start from one face, grow cluster by adding its neighbours to set
		//add their neighbours to the set
		//ideally all faces will be included in final set

		uint32_t faces_processed = 0;//debug only
		uint32_t additional_charts = 0;

		for (uint32_t chart_id = 0; chart_id < num_charts_created; ++chart_id)
		{
			//build group of faces in chart from chart id map
			std::set<uint32_t> faces;
			for ( Facet_iterator fb = P.facets_begin(); fb != P.facets_end(); ++fb){
				if (chart_id_map[fb->id()] == chart_id)
				{
					faces.insert(fb->id());
				}
			}
			faces_processed += faces.size();//debug only

			// std::stack<uint32_t> face_stack;

			//TODO complete
			

		}


		if (P.size_of_facets() != faces_processed)//debug only
		{
			std::cout << "WARNING: [correct_split_charts] processed " << faces_processed << " faces, should have processed " << P.size_of_facets() << std::endl;
		}
	}

	//convert 3D cell reference to ID number
	static int location_to_id(vec3i& loc, vec3i& cell_count){
		return loc.x + (loc.y * cell_count.x) + (loc.z * (cell_count.x * cell_count.y));
	}

};