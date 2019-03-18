#include <stack>

typedef scm::math::vec<int, 3> vec3i;

struct Cluster
{
	std::vector<uint32_t> triangles;
	std::vector<double> areas;
	std::vector<Vector> normals;
	std::vector<Point> centroids;
	int depth;
	Point min;
	Point max;

	Cluster(){
		depth = 0;
		min = Point(std::numeric_limits<double>::max(),std::numeric_limits<double>::max(),std::numeric_limits<double>::max());
		max = Point(std::numeric_limits<double>::lowest(),std::numeric_limits<double>::lowest(),std::numeric_limits<double>::lowest());

	}

	Cluster (uint32_t tri, double area, Vector normal, Point centroid){
		// triangles.push_back(tri);
		// areas.push_back(area);
		// normals.push_back(normal);
		// centroids.push_back(centroids);

		min = Point(std::numeric_limits<double>::max(),std::numeric_limits<double>::max(),std::numeric_limits<double>::max());
		max = Point(std::numeric_limits<double>::lowest(),std::numeric_limits<double>::lowest(),std::numeric_limits<double>::lowest());
		
		add_face(tri, area, normal, centroid);

		depth = 0;
	}

	void add_face(uint32_t tri, double area, Vector normal, Point centroid){
		triangles.push_back(tri);
		areas.push_back(area);
		normals.push_back(normal);
		centroids.push_back(centroid);
		update_bounds();
	}

// private:
	void update_bounds(){

		auto& centroid = centroids[centroids.size()-1];

		min = Point( std::min(min.x(), centroid.x()), std::min(min.y(), centroid.y()), std::min(min.z(), centroid.z()) );
		max = Point( std::max(max.x(), centroid.x()), std::max(max.y(), centroid.y()), std::max(max.z(), centroid.z()) );

	}
};

struct GridClusterCreator
{

  	//creates clusters using a grid
  	//params:
  	// - empty chart map
  	// - bounding box of model
  	// - number of cells to split into along longest axis
  	// - cluster settings from input args
  	//
  	// applies post processing steps to give different chart numbers to non-continous chart regions
  	// and to split charts by normal deviation within them
	static uint32_t create_grid_clusters(Polyhedron &P,
									 std::map<uint32_t, uint32_t> &chart_id_map,
									 BoundingBoxLimits limits,
									 int CELL_RES,
									 CLUSTER_SETTINGS cluster_settings) {


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

		std::vector<Point> centroids;
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
			centroids.push_back(centroid_p);
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

		std::cout << "Charts created by counter : " << chart_counter << std::endl;

		//for all triangles, swap location id for chart id
		for ( Facet_iterator fb = P.facets_begin(); fb != P.facets_end(); ++fb){
			int old_chart_id = chart_id_map[fb->id()];
			int new_chart_id = charts_created[old_chart_id];
			chart_id_map[fb->id()] = new_chart_id;
		}

		uint32_t charts_added = 0;
		// uint32_t charts_added = correct_split_charts(P,chart_id_map, charts_created.size());
		// std::cout << "correct_split_charts method added " << charts_added << " Charts" << std::endl;

		uint32_t total_grid_charts = charts_created.size() + charts_added;
		std::cout << "Created " <<   total_grid_charts << " charts in total \n"; 

		uint32_t total_charts = split_charts(P, chart_id_map, centroids, cluster_settings);
		std::cout << "Chart splitting added " << total_charts - total_grid_charts << "Charts\n";
		return total_charts;

		// return total_grid_charts;
		// return charts_created.size();



	}

	//seeks chart areas that are not connected, and assigns new ids to unconnected areas
	//returns number of charts added
	static uint32_t correct_split_charts(Polyhedron &P,
									 std::map<uint32_t, uint32_t> &chart_id_map,
									 uint32_t num_charts_created){

		//start from one face, grow cluster by adding its neighbours to set
		//add their neighbours to the set
		//ideally all faces will be included in final set

		std::cout << "Correcting split charts...\n";
		std::cout << "Starting with " << num_charts_created << " charts\n";

		uint32_t faces_processed = 0;//debug only
		uint32_t additional_charts_overall = 0;
		//for each chart id
		for (uint32_t chart_id = 0; chart_id < num_charts_created; ++chart_id)
		{

			std::cout << "Chart: " << chart_id << std::endl;

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

			int32_t additional_charts_for_this_chart = -1;

			//pick first face in chart faces
			//get neighbours 
			//if nbr face is in chart_faces, but not in processed faces, add to faces to process stack

			//if stack is empty, and not all chart faces have been processed, then create repeat process from a different starting point

			while (processed_faces.size() < chart_faces.size()){

				additional_charts_for_this_chart++;

				//get starting face
				//must not be in processed faces, but must be in faces
				for(uint32_t f : chart_faces){
					if (processed_faces.find(f) == processed_faces.end())//if not processed faces in set
					{
						//add this face as starting face
						Facet_iterator f_it = P.facets_begin();
						std::advance(f_it,f);

						//debug - check iteration of polyhedron faces
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

				//loop to find all connected faces of starting face
				while (!faces_to_process.empty()){

					//get and remove top node, make sure it is not already processed
					Facet_handle top_node = faces_to_process.top();
					faces_to_process.pop();
					uint32_t top_node_id = top_node->id();

					// std::cout << "Processing face: " << top_node_id << std::endl;

					if (processed_faces.find(top_node_id) != processed_faces.end()){
						// std::cout << "top node already processed\n";
						continue;
					}



					//add new chart id if necessary for this face
					//if this is found on the first pass, no action needed
		      		//otherwise new id needs to be calculated
		      		if (additional_charts_for_this_chart > 0)
		      		{
		      			uint32_t new_id = num_charts_created + additional_charts_overall + additional_charts_for_this_chart;
		      			chart_id_map[top_node_id] = new_id;
		      		}

					//record that this face has been processed
					processed_faces.insert(top_node_id);

					//get neighbours of top node
					Halfedge_facet_circulator he = top_node->facet_begin();
				    do {

				    	//check edge is not a border
				    	if ( !(he->is_border()) && !(he->opposite()->is_border()) ){

						    Facet_handle nbr_facet = he->opposite()->facet(); 
						    uint32_t nbr_facet_id = nbr_facet->id();

						    // std::cout << " - found neighbour " << nbr_facet_id << std::endl;

						    //if this neighbour appears in chart faces...
						    if (chart_faces.find(nbr_facet_id) != chart_faces.end())
						    {
						    	//and if its not in processed faces...
						    	if (processed_faces.find(nbr_facet_id) == processed_faces.end())
						    	{
						    		//then add to stack to process later
						    		faces_to_process.push(nbr_facet);

						    	}
						    }
				    	}


				    } while ( ++he != top_node->facet_begin());


				} // end faces_to_process while


			}//end chart while
			
			additional_charts_overall += additional_charts_for_this_chart;

		}//end for chart

		if (P.size_of_facets() != faces_processed)//debug only
		{
			std::cout << "WARNING: [correct_split_charts] processed " << faces_processed << " faces, should have processed " << P.size_of_facets() << std::endl;
		}
		else {
			std::cout << "[correct_split_charts] processed all facets in Polyhedron\n";
		}

		return additional_charts_overall;
	}

	//convert 3D cell reference to ID number
	static int location_to_id(vec3i& loc, vec3i& cell_count){
		return loc.x + (loc.y * cell_count.x) + (loc.z * (cell_count.x * cell_count.y));
	}


	//splits charts recursively by an error metric
	//return total number of charts
	//
	//aims to avoid non-planar initial chartsthat may result from grid-initialisation
	static uint32_t split_charts(Polyhedron &P,
									 std::map<uint32_t, uint32_t> &chart_id_map,
									 std::vector<Point> centroids,
									 CLUSTER_SETTINGS cluster_settings){

		std::cout << "Started Chart splitting...\n";


		//calculate areas and normals of each face
	    std::map<face_descriptor,double> fareas;
	    std::map<face_descriptor,Vector> fnormals;
	    for(face_descriptor fd: faces(P)){
	      fareas[fd] = CGAL::Polygon_mesh_processing::face_area  (fd,P);
	    }
	    CGAL::Polygon_mesh_processing::compute_face_normals(P,boost::make_assoc_property_map(fnormals));
	    //create boost face iterator
	    face_iterator fb_boost, fe_boost;
	    boost::tie(fb_boost, fe_boost) = faces(P);

		std::map<uint32_t, Cluster> chart_clusters;
		std::map<uint32_t, Cluster>::iterator cluster_it;
		std::stack<uint32_t> clusters_to_process;
		uint32_t chart_count = 0;

		//sort charts into clusters
		for(uint32_t i = 0; i < P.size_of_facets(); i++){

			uint32_t tri_id = i;
			uint32_t chart_id = chart_id_map[i];

			chart_count = std::max(chart_id, chart_count);
			
			//if cluster exists add to it
			cluster_it = chart_clusters.find(chart_id);
			if (cluster_it != chart_clusters.end())
			{
				Cluster &c = cluster_it->second;
				c.add_face(tri_id, fareas[*fb_boost], fnormals[*fb_boost], centroids[i]);
			}
			else { //if not, create it
				Cluster new_cluster (tri_id, fareas[*fb_boost], fnormals[*fb_boost],centroids[i]);
				chart_clusters.insert(std::pair<uint32_t,Cluster>(chart_id, new_cluster));

				clusters_to_process.push(chart_id);
			}

			fb_boost++;
		}

		std::cout << "Chart splitting: added " << clusters_to_process.size() << " to stack for processing\n";

		//process the stack until all nodes processed
		while (!clusters_to_process.empty()){

			const int MAX_DEPTH = 5;

			// pop cluster id from top
			uint32_t cluster_id = clusters_to_process.top();
			clusters_to_process.pop();

			Cluster cluster = chart_clusters[cluster_id];

			if (cluster.triangles.size() == 0)//debug only
			{
				std::cout << "cluster " << cluster_id << "has no triangles\n";
				continue;
			}

			if (cluster.depth >= MAX_DEPTH)//don't split if depth is already deep enough
			{
				continue;
			}

			Vector avg_normal(0.0,0.0,0.0);
			double total_area = 0.0;

			//calculated weighted average normal
			for (uint32_t i = 0; i < cluster.triangles.size(); ++i)
			{
				total_area += cluster.areas[i];
				avg_normal += (cluster.areas[i] * cluster.normals[i]); 
			}
			// avg_normal /= total_area;
			avg_normal = Utils::normalise(avg_normal);

			if (total_area == 0.0)//debug only
			{
				std::cout << "cluster " << cluster_id << "has no area\n";
				continue;
			}

			//calculate weighted variance from average normal
			double variance = 0.0;
			for (uint32_t i = 0; i < cluster.triangles.size(); ++i){

				//calc angle diff from avg_normal
				double dot = avg_normal * cluster.normals[i];
				dot = std::max(-1.0, std::min(1.0,dot));
				double angle_diff = acos(dot);
				angle_diff = std::abs(angle_diff) * cluster.areas[i];
				variance += angle_diff;
			} 
			variance /= cluster.triangles.size();
			variance /= total_area;

			//decide whether to split this cluster further:
			// std::cout << "chart " << cluster_id << ", variance = " << variance << std::endl;

			// const double VAR_THRES = 0.001;
			if (variance > cluster_settings.chart_split_threshold)
			{
				//create new nodes by dividing the node into 8

				const Point midpoint = Utils::midpoint(cluster.min, cluster.max);
				Cluster new_clusters[8];

				//distribute triangles between 8 new clusters
				for (uint32_t i = 0; i < cluster.triangles.size(); ++i)
				{
					const Point& centroid = cluster.centroids[i];

					//determine 3D index
					int x = 0, y = 0, z = 0;
					if (centroid.x() > midpoint.x()) {x = 1;}
					if (centroid.y() > midpoint.y()) {y = 1;}
					if (centroid.z() > midpoint.z()) {z = 1;}
					vec3i loc = vec3i(x,y,z);
					vec3i cell_count = vec3i(2,2,2);
					const int cell_id =  location_to_id(loc, cell_count);

					//add face to cluster with that index
					new_clusters[cell_id].add_face(
						cluster.triangles[i],
						cluster.areas[i],
						cluster.normals[i],
						cluster.centroids[i]);

					//update chart id map here - create new id for each new cluster
					chart_id_map[ cluster.triangles[i] ] = chart_count + cell_id;

				}

				//add new clusters to stack, with appropriate ids
				for (int i = 0; i < 8; ++i)
				{

					//check that they have faces
					if (new_clusters[i].triangles.size() > 0)
					{
						new_clusters[i].depth = cluster.depth + 1;
						chart_clusters.insert(std::pair<uint32_t,Cluster>(chart_count + i, new_clusters[i]));
						clusters_to_process.push(chart_count + i);

						// std::cout << "Adding cluster " << chart_count+i << " with depth " << new_clusters[i].depth << std::endl;
					}
				}

				//update total number of charts
				chart_count += 8;
			
			}//end if add to stack
		
		}//end while


		//update chart id map 
		std::map<uint32_t, uint32_t> tidy_id_map; //key: old chart id, value: new chart id

		//go through chart id map and count active charts, put into tidy id map
		chart_count = 0;
		for (auto& face : chart_id_map){
			uint32_t chart_id = face.second;
			//if not in tidy map, add and assign new id
			if (tidy_id_map.find(chart_id) == tidy_id_map.end())
			{
				tidy_id_map[chart_id] = chart_count++;
			}
		}
		//go through again to update chart ids
		for (auto& face : chart_id_map){
			uint32_t old_chart_id = face.second;
			uint32_t new_chart_id = tidy_id_map[old_chart_id];
			face.second = new_chart_id;
		}

		std::cout << "Finished original chart splitting: " << chart_count << " charts" << std::endl;


		//return total num of charts 
		return chart_count;

	}


};