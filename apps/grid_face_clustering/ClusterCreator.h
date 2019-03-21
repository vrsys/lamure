

//class for running clustering algorithm on Charts
struct ClusterCreator
{

  //takes a chart id map that was created by grid/octree clusters, and creates a list of Chart objects
  static uint32_t 
  create_chart_clusters_from_grid_clusters(Polyhedron &P,
                const double cost_threshold , 
                const uint32_t chart_threshold, 
                CLUSTER_SETTINGS cluster_settings,
                std::map<uint32_t, uint32_t> &chart_id_map,
                uint32_t num_initial_charts){

    std::vector<Chart> charts;

    uint32_t num_charts = initialise_charts_from_grid_clusters(P, chart_id_map, charts, cluster_settings, chart_threshold);


    // check that existing chart number is not already lower than threshold
    if (num_charts <= chart_threshold)
    {
      std::cout << "Input to chart clusterer already had number of charts below chart threshold" << std::endl;
      return num_charts;
    }

    //recalculate perimeters of charts to ensure they are correct
    for (auto& chart : charts) {
      chart.recalculate_perimeter_from_scratch();
      chart.create_neighbour_set(chart_id_map);
    } 

    //create join list
    std::list<JoinOperation> joins;
    create_join_list_from_chart_vector(charts, joins, cluster_settings, chart_id_map);

    //do the clustering!
    // cluster_faces(charts, joins,cost_threshold,chart_threshold,cluster_settings,chart_id_map);

    uint32_t active_charts =  populate_chart_LUT(charts, chart_id_map);

    //checking - how many charts have border edges?
    // uint32_t charts_with_borders = 0;
    // for(auto& chart : charts){if (chart.active && chart.has_border_edge) charts_with_borders++;}
    // std::cout << charts_with_borders << " of " << active_charts << " charts have borders\n";

    return active_charts;
  }

  //given a chart_id_map, fill a list of chart objects which describe initial clustering state
  static uint32_t
  initialise_charts_from_grid_clusters(Polyhedron &P, 
                                       std::map<uint32_t, uint32_t> &chart_id_map,
                                       std::vector<Chart> &charts,
                                       CLUSTER_SETTINGS cluster_settings,
                                       const uint32_t chart_threshold ){

        //calculate areas of each face
    std::map<face_descriptor,double> fareas;
    std::map<face_descriptor,Vector> fnormals;
    // calculate_normals_and_areas(P,fnormals,fareas);
    std::cout << "Calculating face areas...\n";
    for(face_descriptor fd: faces(P)){
      fareas[fd] = CGAL::Polygon_mesh_processing::face_area  (fd,P);
    }

    //debugging
    std::cout << "Checking areas...\n";
    for (auto const& fd : fareas){
      if (fd.second == 0 || std::isnan(fd.second) )
      {
        std::cout << "Face " << fd.first->id() << " has area " << fd.second << std::endl;
      }
    }

    std::cout << "Calculating face normals...\n";
    CGAL::Polygon_mesh_processing::compute_face_normals(P,boost::make_assoc_property_map(fnormals));

    std::cout << "Creating charts from grid clusters...\n";

    //get boost face iterator
    face_iterator fb_boost, fe_boost;
    boost::tie(fb_boost, fe_boost) = faces(P);

    //to create chart vector
    //create vector of vectors, each chart has a vector of face ids
    // then creation of each chart can be parallelised once vactors are created
    // add new chart method where chart is built from a list of faces rather than incrementally

    //inverse index (K = chart, V = face ids)
    std::map<uint32_t, std::vector<uint32_t> > faces_per_chart;
    for (auto face_entry : chart_id_map)
    {
      //face entry: Key = face id, Value = chart id
      faces_per_chart[face_entry.second].push_back(face_entry.first);
    }
    //convert to vector of vectors
    std::vector<std::vector<uint32_t> > faces_per_chart_vector;
    for (auto& face_list_entry : faces_per_chart)
    {
      std::sort(face_list_entry.second.begin(), face_list_entry.second.end());
      faces_per_chart_vector.push_back(face_list_entry.second);
    }
    std::cout << "Found " << faces_per_chart_vector.size() << " charts in chart_id_map\n";

    if (faces_per_chart_vector.size() <= chart_threshold)
    {
      std::cout << "Chart threshold already reached\n";
      return faces_per_chart_vector.size();
    }

    std::cout << "Compiling in to chart objects...\n";

    // resize vector before paralellisation
    charts.resize(faces_per_chart_vector.size());

    //for each chart
    #pragma omp parallel for 
    for (uint32_t i = 0; i < faces_per_chart_vector.size(); ++i)
    {
      Chart chart_local;

      //go through list of faces, build charts
      std::vector<uint32_t> face_list = faces_per_chart_vector[i];

      //get begin iterators
      uint32_t current_position = 0;
      Facet_iterator fi = P.facets_begin();
      face_iterator fb_boost, fe_boost;
      boost::tie(fb_boost, fe_boost) = faces(P);

      for (uint32_t f = 0; f < face_list.size(); ++f)
      {
        uint32_t face_id = face_list[f];
        uint32_t to_advance = face_id - current_position;

        //advance iterator by required number of steps
        std::advance(fi, to_advance);
        std::advance(fb_boost, to_advance);

        //create chart from this face, and merge if not the first
        if (f == 0)
        {
          chart_local = Chart(i,*fi, fnormals[*fb_boost], fareas[*fb_boost]);
        }
        else {
          Chart new_chart(i,*fi, fnormals[*fb_boost], fareas[*fb_boost]);
          chart_local.quick_merge_with(new_chart);
        }

        current_position = face_id;
      }//for each face

      //to calculate perimeter and avg normal properly
      chart_local.update_after_quick_merge();

      // charts.push_back(chart_local);
      charts[i] = chart_local;

    }//for each chart

    //sort the charts in order of id
    // std::sort(charts.begin(), charts.end(), Chart::sort_by_id);


    std::cout << "Created " << charts.size() << " charts from grid clusters" << std::endl;

    return charts.size();

  }

  static void
  create_join_list_from_chart_vector(std::vector<Chart> &charts, 
                                     std::list<JoinOperation> &joins,
                                     CLUSTER_SETTINGS cluster_settings,
                                     std::map<uint32_t, uint32_t> &chart_id_map){

    std::cout << "Creating joins from chart list...\n";

    std::set<uint32_t> processed_charts;
    std::set<uint32_t> chart_neighbours;

    //for each chart
    for (auto& chart : charts)
    {
      chart_neighbours.clear();

      // uint32_t this_chart_id = chart.id()
    // for each face in chart, find neighbours, add to set
      for (auto& face : chart.facets)
      {
        //for each edge
        Halfedge_facet_circulator fc = face.facet_begin();
        do {
          if (!fc->is_border() && !(fc->opposite()->is_border()) )//guard against no neighbour at this edge
          {
            //get chart id of neighbour, add to set if it is not this chart
            uint32_t nbr_face_id = fc->opposite()->facet()->id();
            uint32_t nbr_chart_id = chart_id_map[nbr_face_id];

            if (nbr_chart_id != chart.id){
              chart_neighbours.insert(nbr_chart_id);
            }
          }
        } while ( ++fc != face.facet_begin());
      }

      // std::cout << "found " << chart_neighbours.size() << " unique neighbours for chart " << chart.id << std::endl;
      // int added_joins = 0;

      //create joins...
      //if neighbouts have not already been processed, create join between this and neighbour
      for (auto& nbr_chart_id : chart_neighbours)
      {
        //make sure it hasnt been processed already
        if (processed_charts.find(nbr_chart_id) == processed_charts.end())
        {
            // chart ids should be equal to their index in the vector at this point
            JoinOperation join (chart.id, nbr_chart_id ,JoinOperation::cost_of_join(charts[chart.id],charts[nbr_chart_id], cluster_settings));
            joins.push_back(join);

            //TODO don't add duplicate joins - where charts share many edges

            // added_joins++;
        }
      }

      // std::cout << "Added " << added_joins << " joins\n";

    //add this chart to set of processed charts, so that it is not considered for new joins
      processed_charts.insert(chart.id);
    }

    std::cout << "Created " << joins.size() << " joins\n";

  }
  
  //takes a polymesh and creates a list of Chart objects, one for each face
  //and a list of joins between all charts
  static uint32_t 
  create_chart_clusters_from_faces (Polyhedron &P, 
                const double cost_threshold , 
                const uint32_t chart_threshold, 
                CLUSTER_SETTINGS cluster_settings,
                std::map<uint32_t, uint32_t> &chart_id_map){

    std::stringstream report;
    report << "--------------------\nReport:\n----------------------\n";

    //calculate areas of each face
    std::cout << "Calculating face areas...\n";
    std::map<face_descriptor,double> fareas;
    for(face_descriptor fd: faces(P)){
      fareas[fd] = CGAL::Polygon_mesh_processing::face_area  (fd,P);
    }
    //calculate normals of each faces
    std::cout << "Calculating face normals...\n";
    std::map<face_descriptor,Vector> fnormals;
    CGAL::Polygon_mesh_processing::compute_face_normals(P,boost::make_assoc_property_map(fnormals));

    //get boost face iterator
    face_iterator fb_boost, fe_boost;
    boost::tie(fb_boost, fe_boost) = faces(P);

    //each face begins as its own chart
    std::cout << "Creating initial charts...\n";
    std::vector<Chart> charts;
    for ( Facet_iterator fb = P.facets_begin(); fb != P.facets_end(); ++fb){

      //init chart instance for face
      Chart c(charts.size(),*fb, fnormals[*fb_boost], fareas[*fb_boost]);
      charts.push_back(c);

      fb_boost++;
    }
    //create possible join list/queue. Each original edge in the mesh becomes a join (if not a boundary edge)
    std::cout << "Creating initial joins...\n";
    std::list<JoinOperation> joins;
    std::list<JoinOperation>::iterator it;

    int edgecount = 0;

    for( Edge_iterator eb = P.edges_begin(), ee = P.edges_end(); eb != ee; ++ eb){

      edgecount++;

      //only create join if halfedge is not a boundary edge
      if ( !(eb->is_border()) && !(eb->opposite()->is_border()) )
      {
            uint32_t face1 = eb->facet()->id();
            uint32_t face2 = eb->opposite()->facet()->id();
            JoinOperation join (face1,face2,JoinOperation::cost_of_join(charts[face1],charts[face2], cluster_settings));
            joins.push_back(join);
      }
    } 
    std::cout << joins.size() << " joins\n" << edgecount << " edges\n";

    // cluster_faces(charts, joins, cost_threshold, chart_threshold, cluster_settings,chart_id_map);

    return populate_chart_LUT(charts, chart_id_map);

  }

  static uint32_t populate_chart_LUT(std::vector<Chart> &charts, std::map<uint32_t, uint32_t> &chart_id_map){

    chart_id_map.clear();

    //populate LUT for face to chart mapping
    //count charts on the way to apply new chart ids
    uint32_t active_charts = 0;
    for (uint32_t id = 0; id < charts.size(); ++id) {
      auto& chart = charts[id];
      if (chart.active) {
        for (auto& f : chart.facets) {
          chart_id_map[f.id()] = active_charts;
        }
        active_charts++;
      }
    }

    return active_charts;
  }
};

