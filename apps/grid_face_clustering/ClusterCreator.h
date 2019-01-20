

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



    //calculate areas of each face
    std::map<face_descriptor,double> fareas;
    std::map<face_descriptor,Vector> fnormals;
    // calculate_normals_and_areas(P,fnormals,fareas);
    std::cout << "Calculating face areas...\n";
    for(face_descriptor fd: faces(P)){
      fareas[fd] = CGAL::Polygon_mesh_processing::face_area  (fd,P);
    }
    std::cout << "Calculating face normals...\n";
    CGAL::Polygon_mesh_processing::compute_face_normals(P,boost::make_assoc_property_map(fnormals));

    //get boost face iterator
    face_iterator fb_boost, fe_boost;
    boost::tie(fb_boost, fe_boost) = faces(P);

    //build existing charts from chart id map
    std::cout << "Creating charts from grid clusters...\n";
    std::vector<Chart> charts;
    // std::set<uint32_t> chart_ids_created;
    std::map<uint32_t, uint32_t> chart_ids_created; //key::chart id, value::location in chart array
    std::map<uint32_t, uint32_t>::iterator it_c;

    //for each face
    for ( Facet_iterator fb = P.facets_begin(); fb != P.facets_end(); ++fb){



      uint32_t given_chart_id = chart_id_map[fb->id()];

    //when adding a face, check that it shares at least one boundary with a face in the same chart - otherwise give it a new id
      Halfedge_facet_circulator fc = fb->facet_begin();
      bool found_chart_nbr = false;
      int32_t nbrs[3] = {-1};
      double edgelengths[3] = {0.0};
      uint32_t nbr_count = 0;
      do {
        if (!fc->is_border() && !(fc->opposite()->is_border()) )//guard against no neighbour at this edge
        {
          //get chart id of neighbour, compare with the chart id of this face
          uint32_t nbr_id = fc->opposite()->facet()->id();
          uint32_t nbr_chart_id = chart_id_map[nbr_id];
          if (nbr_chart_id == given_chart_id)
          {
            found_chart_nbr = true;
            break;
          }
          //Save neighbour id and edge length
          nbrs[nbr_count] = nbr_id;
          edgelengths[nbr_count] = Chart::edge_length(fc);
        }
        nbr_count++;
      } while ( ++fc != fb->facet_begin());


      //create chart 
      Chart new_chart(charts.size(),*fb, fnormals[*fb_boost], fareas[*fb_boost]);

      if (!found_chart_nbr) // determine which neighbour group it should belong to
      {

        //find longest edge
        double longest = 0.0;
        uint32_t longest_id = 0;
        for (int i = 0; i < 3; ++i)
        {
          if((edgelengths[i] > longest) && nbrs[i] > 0){longest_id = i;}
        }
        given_chart_id = chart_id_map[nbrs[longest_id]];

        //save for future reference when checking other faces
        chart_id_map[fb->id()] = given_chart_id;

 
        // given_chart_id = num_initial_charts++;
      }

      // if chart already exists for that id,  merge this chart into existing (discard merged in chart)
      it_c = chart_ids_created.find(given_chart_id);
      if (it_c != chart_ids_created.end())
      {
        //array location of existing chart
        uint32_t loc = it_c->second;
        charts[loc].merge_with(new_chart, JoinOperation::cost_of_join(charts[loc], new_chart, cluster_settings));
      }
        //if chart doesnt exist for that chart id, save created chart to chart list
      else {
        //add location to map
        chart_ids_created[given_chart_id] = charts.size();
        charts.push_back(new_chart);
      }

      fb_boost++;
    }

    std::cout << "Created " << charts.size() << " charts from grid clusters" << std::endl;

    populate_chart_LUT(charts, chart_id_map);

    // check that existing chart number is not already lower than threshold
    if (charts.size() <= chart_threshold)
    {
      std::cout << "Input to chart clusterer already had number of charts below chart threshold" << std::endl;
      return charts.size();
    }

    //recalculate perimeters of charts to ensure they are correct
    for (auto& chart : charts) {chart.recalculate_perimeter_from_scratch();} 

    


    //debug -check chart ids
    // for (auto& chart : charts) {std::cout << "Chart id: " << chart.id << std::endl;} 

    std::list<JoinOperation> joins;
    create_join_list_from_chart_vector(charts, joins, cluster_settings, chart_id_map);

    cluster_faces(charts, joins,cost_threshold,chart_threshold,cluster_settings);





    return populate_chart_LUT(charts, chart_id_map);
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

            // added_joins++;
        }
      }

      // std::cout << "Added " << added_joins << " joins\n";

    //add this chart to set of processed charts, so that it is not considered for new joins
      processed_charts.insert(chart.id);
    }

    std::cout << "Created " << joins.size() << " joins\n";

  }

  //takes a list of joins and charts, and executes joins until target number of charts/cost threshold is reached
  static void 
  cluster_faces(std::vector<Chart> &charts, 
                std::list<JoinOperation> &joins,
                const double cost_threshold, 
                const uint32_t chart_threshold,
                CLUSTER_SETTINGS &cluster_settings
                ){


    std::stringstream report;
    report << "--------------------\nReport:\n----------------------\n";

    std::list<JoinOperation>::iterator it;

    //for reporting and calculating when to stop merging
    const uint32_t initial_charts = charts.size();
    const uint32_t desired_merges = initial_charts - chart_threshold;
    uint32_t chart_merges = 0;
    // join charts until target is reached
    int prev_cost_percent = -1;
    int prev_charts_percent = -1;
    int overall_percent = -1;

    joins.sort(JoinOperation::sort_joins);
    const double lowest_cost = joins.front().cost;


    //execute lowest join cost and update affected joins.  re-sort.
    std::cout << "Processing join queue...\n";
    while (joins.front().cost < cost_threshold  
          &&  !joins.empty()
          &&  (charts.size() - chart_merges) > chart_threshold){

      //reporting-------------
      int percent = (int)(((joins.front().cost - lowest_cost) / (cost_threshold - lowest_cost)) * 100);
      if (percent != prev_cost_percent && percent > overall_percent) {
        prev_cost_percent = percent;
        overall_percent = percent;
        std::cout << percent << " percent complete\n";
      } 
      percent = (int)(((float)chart_merges / (float)desired_merges) * 100);
      if (percent != prev_charts_percent && percent > overall_percent) {
        prev_charts_percent = percent;
        overall_percent = percent;
        std::cout << percent << " percent complete\n";
      }

      //implement the join with lowest cost
      JoinOperation join_todo = joins.front();
      joins.pop_front();

      // std::cout << "join cost : " << join_todo.cost << std::endl; 

      //merge faces from chart2 into chart 1
      // std::cout << "merging charts " << join_todo.chart1_id << " and " << join_todo.chart2_id << std::endl;
      charts[join_todo.chart1_id].merge_with(charts[join_todo.chart2_id], join_todo.cost);


      //DEactivate chart 2
      if (charts[join_todo.chart2_id].active == false)
      {
        report << "chart " << join_todo.chart2_id << " was already inactive at merge " << chart_merges << std::endl; // should not happen
        continue;
      }
      charts[join_todo.chart2_id].active = false;
      
      int current_item = 0;
      std::list<int> to_erase;
      std::vector<JoinOperation> to_replace;

      //update itremaining joins that include either of the merged charts
      for (it = joins.begin(); it != joins.end(); ++it)
      {
        //if join is affected, update references and cost
        if (it->chart1_id == join_todo.chart1_id 
           || it->chart1_id == join_todo.chart2_id 
           || it->chart2_id == join_todo.chart1_id 
           || it->chart2_id == join_todo.chart2_id )
        {

          //eliminate references to joined chart 2 (it is no longer active)
          // by pointing them to chart 1
          if (it->chart1_id == join_todo.chart2_id){
            it->chart1_id = join_todo.chart1_id;
          }
          if (it->chart2_id == join_todo.chart2_id){
            it->chart2_id = join_todo.chart1_id; 
          }

          //search for duplicates
          if ((it->chart1_id == join_todo.chart1_id && it->chart2_id == join_todo.chart2_id) 
            || (it->chart2_id == join_todo.chart1_id && it->chart1_id == join_todo.chart2_id) ){
            report << "duplicate found : c1 = " << it->chart1_id << ", c2 = " << it->chart2_id << std::endl; 

            to_erase.push_back(current_item);
          }
          //check for joins within a chart
          else if (it->chart1_id == it->chart2_id)
          {
            report << "Join found within a chart: " << it->chart1_id << std::endl;
            to_erase.push_back(current_item);
            
          }
          else {
            //update cost with new cost
            it->cost = JoinOperation::cost_of_join(charts[it->chart1_id], charts[it->chart2_id], cluster_settings);

            //save this join to be deleted and replaced in correct position after deleting duplicates
            to_replace.push_back(*it);
            to_erase.push_back(current_item);
          }
        }
        current_item++;
      }

      //adjust ID to be deleted to account for previously deleted items
      to_erase.sort();
      int num_erased = 0;
      for (auto id : to_erase) {
        std::list<JoinOperation>::iterator it2 = joins.begin();
        std::advance(it2, id - num_erased);
        joins.erase(it2);
        num_erased++;
      }

      // replace joins that were filtered out to be sorted
      if (to_replace.size() > 0)
      {
        std::sort(to_replace.begin(), to_replace.end(), JoinOperation::sort_joins);
        std::list<JoinOperation>::iterator it2;
        uint32_t insert_item = 0;
        for (it2 = joins.begin(); it2 != joins.end(); ++it2){
          //insert items while join list item has bigger cost than element to be inserted
          while (it2->cost > to_replace[insert_item].cost
                && insert_item < to_replace.size()){
            joins.insert(it2, to_replace[insert_item]);
            insert_item++;
          }
          //if all items are in place, we are done
          if (insert_item >= to_replace.size())
          {
            break;
          }
        }
        //add any remaining items
        for (uint32_t i = insert_item; i < to_replace.size(); i++){
          joins.push_back(to_replace[i]);
        }
      }

  #if 0
      //CHECK that each join would give a chart with at least 3 neighbours
      //TODO also need to check boundary edges
      to_erase.clear();
      std::vector<std::vector<uint32_t> > neighbour_count (charts.size(), std::vector<uint32_t>(0));
      std::list<JoinOperation>::iterator it2;
      for (it2 = joins.begin(); it2 != joins.end(); ++it2){
        //for chart 1 , add entry in vector for that chart containing id of chart 2
        // and vice versa
        neighbour_count[it2->chart1_id].push_back(it2->chart2_id);
        neighbour_count[it2->chart2_id].push_back(it2->chart1_id);
      }

      uint32_t join_id = 0;
      for (it2 = joins.begin(); it2 != joins.end(); ++it2){
        // combined neighbour count of joins' 2 charts should be at least 5
        // they will both contain each other (accounting for 2 neighbours) and require 3 more

        //merge the vectors for each chart in the join and count unique neighbours
        std::vector<uint32_t> combined_nbrs (neighbour_count[it2->chart1_id]);
        combined_nbrs.insert(combined_nbrs.end(), neighbour_count[it2->chart2_id].begin(), neighbour_count[it2->chart2_id].end());

        //find unique
        std::sort(combined_nbrs.begin(), combined_nbrs.end());
        uint32_t unique = 1;
        for (uint32_t i = 1; i < combined_nbrs.size(); i++){
          if (combined_nbrs[i] != combined_nbrs [i-1])
          {
            unique++;
          }
        }
        if (unique < 5)
        {
          to_erase.push_back(join_id);
        }
        join_id++;
      }
      //erase joins that would result in less than 3 corners
      to_erase.sort();
      num_erased = 0;
      for (auto id : to_erase) {
        std::list<JoinOperation>::iterator it2 = joins.begin();
        std::advance(it2, id - num_erased);
        joins.erase(it2);
        num_erased++;
      }
  #endif

      chart_merges++;

      
    }

    std::cout << "--------------------\nCharts:\n----------------------\n";

    uint32_t total_faces = 0;
    uint32_t total_active_charts = 0;
    for (uint32_t i = 0; i < charts.size(); ++i)
    {
      if (charts[i].active)
      {
        uint32_t num_faces = charts[i].facets.size();
        total_faces += num_faces;
        total_active_charts++;
      }
    }
    std::cout << "Total number of faces in charts = " << total_faces << std::endl;
    std::cout << "Initial charts = " << charts.size() << std::endl;
    std::cout << "Total number merges = " << chart_merges << std::endl;
    std::cout << "Total active charts = " << total_active_charts << std::endl;


    // std::cout << report.str();
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

    cluster_faces(charts, joins, cost_threshold, chart_threshold, cluster_settings);

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

