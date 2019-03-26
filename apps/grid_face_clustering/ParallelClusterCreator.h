#include <memory>

#include <omp.h>

//class for running clustering algorithm on Charts
struct ParallelClusterCreator
{

  static uint32_t create_charts(std::map<uint32_t, uint32_t> &chart_id_map, 
                                Polyhedron &P,
                                const double cost_threshold, 
                                const uint32_t chart_threshold,
                                CLUSTER_SETTINGS cluster_settings){


    std::vector<Chart> charts;

    if (chart_id_map.size() == 0)
    {
      std::cout << "Creating charts from individual faces\n";

      //create charts
      create_initial_charts(charts, P);

      //populate lookup table to allow quicker determination of whether faces are neighbours when making joins
      populate_chart_LUT(charts, chart_id_map);

    }
    else {

      std::cout << "Creating charts from grid based initial splitting\n";
      uint32_t num_charts = ClusterCreator::initialise_charts_from_grid_clusters(P, chart_id_map, charts, cluster_settings, chart_threshold);

      // populate_chart_LUT(charts, chart_id_map);

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


      // return chart_id_map.size();
    }


    //create join bank vector and queue
    std::vector< std::shared_ptr<JoinOperation> > join_queue;
    create_joins_from_chart_vector(charts, join_queue, cluster_settings, chart_id_map);


    //do the clustering
    cluster_faces(charts,join_queue, cost_threshold, chart_threshold,cluster_settings, chart_id_map);


    return populate_chart_LUT(charts, chart_id_map);;
  }

  //builds a chart list where each face of a polyhedron is 1 chart 
  static void 
  create_initial_charts(std::vector<Chart> &charts, 
                        Polyhedron &P){

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
    std::cout << "Creating initial charts...";
    for ( Facet_iterator fb = P.facets_begin(); fb != P.facets_end(); ++fb){

      //init chart instance for face
      Chart c(charts.size(),std::make_shared<Facet>(*fb), fnormals[*fb_boost], fareas[*fb_boost]);
      charts.push_back(c);

      fb_boost++;
    }

    std::cout << "..." << charts.size() << " charts.\n";

  }

  static void
  create_joins_from_chart_vector(std::vector<Chart> &charts, 
                                     // std::vector<std::shared_ptr<JoinOperation> > &joins,
                                     std::vector< std::shared_ptr<JoinOperation> > &join_queue,
                                     CLUSTER_SETTINGS cluster_settings,
                                     std::map<uint32_t, uint32_t> &chart_id_map){

    std::cout << "Creating joins from chart list...";

    std::set<uint32_t> processed_charts;
    std::set<uint32_t> chart_neighbours;

    //for each chart
    for (auto& chart : charts)
    {
      chart_neighbours.clear();



    // for each face in chart, find neighbours, add to chart_neighbours set
      for (auto& face : chart.facets)
      {
        //for each edge
        Halfedge_facet_circulator fc = face->facet_begin();
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
        } while ( ++fc != face->facet_begin());
      }



      //create joins...
      //if neighbours have not already been processed, create join between this and neighbour
      for (auto& nbr_chart_id : chart_neighbours)
      {
        //make sure it hasnt been processed already
        if (processed_charts.find(nbr_chart_id) == processed_charts.end())
        {
            // chart ids should be equal to their index in the vector at this point
            JoinOperation join (chart.id, nbr_chart_id ,JoinOperation::cost_of_join(charts[chart.id],charts[nbr_chart_id], cluster_settings));
            join_queue.push_back( std::make_shared<JoinOperation>(join));
        }
      }

    //add this chart to set of processed charts, so that it is not considered for new joins
      processed_charts.insert(chart.id);

    } // end for each chart

    std::cout << join_queue.size() << " joins\n";

  }

  //takes a list of joins and charts, and executes joins until target number of charts/cost threshold is reached
  static void 
  cluster_faces(std::vector<Chart> &charts, 
                // std::vector<std::shared_ptr<JoinOperation> > &joins,
                std::vector< std::shared_ptr<JoinOperation> >& join_queue,
                const double cost_threshold, 
                const uint32_t chart_threshold,
                CLUSTER_SETTINGS &cluster_settings,
                std::map<uint32_t, uint32_t> &chart_id_map
                ){

    if (join_queue.empty())
    {
      std::cout << "ERROR: join_queue is empty - no joins possible" << std::endl;
    }

    std::cout << "Clustering faces...." << std::endl;

    // std::stringstream report;
    // report << "--------------------\nReport:\n----------------------\n";

    std::vector< std::shared_ptr<JoinOperation> >::iterator it;

    //for reporting and calculating when to stop merging
    const uint32_t initial_charts = charts.size();
    const uint32_t desired_merges = initial_charts - chart_threshold;
    uint32_t chart_merges = 0;
    int prev_cost_percent = -1;
    int prev_charts_percent = -1;
    int overall_percent = -1;

    //key chart position (in chart vector) :: value - list of pointers to join operations that reference this chart
    // std::map<uint32_t, std::vector<std::shared_ptr<JoinOperation> > > chart_to_join_inverse_index;
    // populate_inverse_index(chart_to_join_inverse_index, charts, joins);

    // join_queue.sort(JoinOperation::sort_join_ptrs);
    std::sort(join_queue.begin(),join_queue.end(), JoinOperation::sort_join_ptrs);
    const double lowest_cost = join_queue.front()->cost;


    //execute lowest join cost and update affected joins.  re-sort.
    std::cout << "Processing join queue...\n";
    while (join_queue.front()->cost < cost_threshold  
          &&  !join_queue.empty()
          &&  (charts.size() - chart_merges) > chart_threshold){


      //reporting-------------
      int percent = (int)(((join_queue.front()->cost - lowest_cost) / (cost_threshold - lowest_cost)) * 100);
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


      JoinOperation join_todo = *(join_queue.front());
      join_queue.erase(join_queue.begin());

      //guard against inactive joins
      if (!join_todo.active)
      {
        continue;
      }
      //check amount of neighbours resulting chart would have. if too few, skip to next one
      if (join_todo.results_in_chart_with_neighbours(charts, chart_id_map) < 3)
      {
        continue;
      }


      //merge faces from chart2 into chart 1
      // std::cout << "merging charts " << join_todo.chart1_id << " and " << join_todo.chart2_id << std::endl;

      // charts[join_todo.get_chart1_id()].merge_with(charts[join_todo.get_chart2_id()], join_todo.cost);
      charts[join_todo.get_chart1_id()].merge_with(charts[join_todo.get_chart2_id()]);


      //DEactivate chart 2
      if (charts[join_todo.get_chart2_id()].active == false)
      {
        // report << "chart " << join_todo.chart2_id << " was already inactive at merge " << chart_merges << std::endl; // should not happen
        continue;
      }
      //DEactivate chart 2
      charts[join_todo.get_chart2_id()].active = false;


      //--------------------------------------------------------------
      //update remaining joins that include either of the merged charts
      //--------------------------------------------------------------



#if 0

      // use inverse index to retrieve the joins that need to be updated

      //merge affected join lists from 2 charts involved (from chart 2 to 1)
      std::vector<std::shared_ptr<JoinOperation>>& affected_joins = chart_to_join_inverse_index[join_todo.get_chart1_id()];
      affected_joins.insert( 
        affected_joins.end() , 
        chart_to_join_inverse_index[join_todo.get_chart2_id()].begin(),
        chart_to_join_inverse_index[join_todo.get_chart2_id()].end());

      // std::cout << "merged: " << affected_joins.size() << "\n";

      std::list<uint32_t> indices_to_remove;
      //for each affected join, update or add to list for removal
      for (uint32_t i = 0; i < affected_joins.size(); i++){

        // std::shared_ptr<JoinOperation> join_op = affected_joins[i];
        std::shared_ptr<JoinOperation> join_op ( affected_joins[i] );

        //replace expired chart and sorts chart ids
        join_op->replace_id_with(join_todo.get_chart2_id(), join_todo.get_chart1_id());

        //check if this join is within a chart now - add to removal list
        if (join_op->get_chart1_id() == join_op-> get_chart2_id())
        {
          indices_to_remove.push_back(i);
          join_op->active = false;
        }
      }


      // std::cout << "to remove: " << indices_to_remove.size() << "\n";

      //remove those not needed any more
      indices_to_remove.sort();
      int num_removed = 0;
      for (auto id : indices_to_remove) {
        std::vector< std::shared_ptr<JoinOperation> >::iterator it2 = affected_joins.begin();
        // adjust ID to be deleted to account for previously deleted items
        std::advance(it2, id - num_removed);
        affected_joins.erase(it2);
        num_removed++;
      }

      // std::cout << "after removing: " << affected_joins.size() << "\n";

      //remove duplicates in affected joins
      auto new_end_of_array = std::unique(affected_joins.begin(), affected_joins.end(), JoinOperation::compare);
      affected_joins.resize( std::distance(affected_joins.begin(),new_end_of_array) ); 

      // std::cout << "after removing duplicates: " << affected_joins.size() << "\n";

      //recalculate costs for what is left
      for (uint32_t i = 0; i < affected_joins.size(); i++){
        // std::cout << "join " << i << std::endl;

        std::shared_ptr<JoinOperation> join_op ( affected_joins[i] );


        // std::cout << "got join " << i << std::endl;
        join_op->cost = JoinOperation::cost_of_join(charts[join_op->get_chart1_id()], charts[join_op->get_chart2_id()], cluster_settings);

        // std::cout << "costed join " << i << std::endl;
      }

      // std::cout << "updated\n";


      //resort join queue
      std::sort(join_queue.begin(),join_queue.end(), JoinOperation::sort_join_ptrs);


      // std::cout << "sorted\n";

#else
              //old method of updating join list


      std::vector<int> to_erase_merged;
      std::vector<std::shared_ptr<JoinOperation> > to_recalculate_error_merged;

      #pragma omp declare reduction (merge : std::vector<int> : omp_out.insert(omp_out.end(), omp_in.begin(), omp_in.end()))
      #pragma omp declare reduction (merge : std::vector< std::shared_ptr<JoinOperation> > : omp_out.insert(omp_out.end(), omp_in.begin(), omp_in.end()))

      //find affected joins and add to list for erase/update
      #pragma omp parallel for reduction(merge: to_erase_merged,to_recalculate_error_merged)
      for(uint32_t j = 0; j < join_queue.size(); j++)
      {

        //if join is affected, update references and cost
        if (  join_queue[j]->get_chart1_id() == join_todo.get_chart1_id() 
           || join_queue[j]->get_chart1_id() == join_todo.get_chart2_id() 
           || join_queue[j]->get_chart2_id() == join_todo.get_chart1_id() 
           || join_queue[j]->get_chart2_id() == join_todo.get_chart2_id() )
        {

          //eliminate references to joined chart 2 (it is no longer active)
          // by pointing them to chart 1
          if ( join_queue[j]->get_chart1_id() == join_todo.get_chart2_id()){
            join_queue[j]->set_chart1_id( join_todo.get_chart1_id() );
          }
          if ( join_queue[j]->get_chart2_id() == join_todo.get_chart2_id()){
            join_queue[j]->set_chart2_id( join_todo.get_chart1_id() ); 
          }

          //save this join to be deleted (and replaced in queue if necessary)
          // to_erase[omp_get_thread_num()].push_back(j);
          to_erase_merged.push_back(j);


          //search for duplicates
          if ( join_queue[j]->get_chart1_id() == join_todo.get_chart1_id() 
            && join_queue[j]->get_chart2_id() == join_todo.get_chart2_id() ){
            // report << "duplicate found : c1 = " << it->chart1_id << ", c2 = " << it->chart2_id << std::endl; 

            //set inactive
            join_queue[j]->active = false;
          }
          //check for joins within a chart
          else if ( join_queue[j]->get_chart1_id() == join_queue[j]->get_chart2_id())
          {
            // report << "Join found within a chart: " << join_queue[j]->chart1_id << std::endl;

            //set inactive
            join_queue[j]->active = false;

            
          }
          else {

            //add (pointer of JO) to vector to be updated
            // to_recalculate_error[omp_get_thread_num()].push_back(join_queue[j]);
            to_recalculate_error_merged.push_back(join_queue[j]);
          }
        }
      }

      //merge parallel vectors

      // std::vector<int> to_erase_merged;
      // std::vector<std::shared_ptr<JoinOperation> > to_recalculate_error_merged;
      // for (uint32_t i = 0; i < to_erase.size(); ++i)
      // {
      //   std::copy(to_erase[i].begin(), to_erase[i].end(), std::back_inserter(to_erase_merged));
      // }
      // for (uint32_t i = 0; i < to_recalculate_error.size(); ++i)
      // {
      //   std::copy(to_recalculate_error[i].begin(), to_recalculate_error[i].end(), std::back_inserter(to_recalculate_error_merged));
      // }

      //erase all elements that need to be erased (either no longer needed or will be recalculated)
      std::sort(to_erase_merged.begin(), to_erase_merged.end());
      int num_erased = 0;
      for (auto id : to_erase_merged) {
        std::vector< std::shared_ptr<JoinOperation> >::iterator it2 = join_queue.begin();
        // adjust ID to be deleted to account for previously deleted items
        std::advance(it2, id - num_erased);
        join_queue.erase(it2);
        num_erased++;
      }

      //recalculate error for joins that need to be updated
      #pragma omp parallel for 
      for(uint32_t j = 0; j < to_recalculate_error_merged.size(); j++){
        std::shared_ptr<JoinOperation> join_ptr ( to_recalculate_error_merged[j] );
        join_ptr->cost = JoinOperation::cost_of_join(charts[join_ptr->get_chart1_id()], charts[join_ptr->get_chart2_id()], cluster_settings);
      }

      // replace joins that were filtered out to be sorted
      if (to_recalculate_error_merged.size() > 0)
      {
        std::sort(to_recalculate_error_merged.begin(), to_recalculate_error_merged.end(), JoinOperation::sort_join_ptrs);
        std::vector< std::shared_ptr<JoinOperation> >::iterator it2;
        uint32_t insert_item = 0;
        for (it2 = join_queue.begin(); it2 != join_queue.end(); ++it2){

          //insert items while join list item has bigger cost than element to be inserted
          while ( insert_item < to_recalculate_error_merged.size() && 
                  (*it2)->cost > to_recalculate_error_merged[insert_item]->cost){

            join_queue.insert(it2, to_recalculate_error_merged[insert_item]);
            insert_item++;
          }
          //if all items are in place, we are done
          if (insert_item >= to_recalculate_error_merged.size())
          {
            break;
          }
        }
        //add any remaining items to end of queue
        for (uint32_t i = insert_item; i < to_recalculate_error_merged.size(); i++){
          join_queue.push_back(to_recalculate_error_merged[i]);
        }
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
    if (!join_queue.empty()) {
      std::cout << "joins remaining: " << join_queue.size() << std::endl;
      std::cout << "Cost of cheapest un-executed join: " << join_queue.front()->cost << std::endl;
    }
    else {
      std::cout << "join list empty" << std::endl;
    }
    std::cout << "Total number of faces in charts = " << total_faces << std::endl;
    std::cout << "Initial charts = " << charts.size() << std::endl;
    std::cout << "Total number merges = " << chart_merges << std::endl;
    std::cout << "Total active charts = " << total_active_charts << std::endl;


    // std::cout << report.str();
  }




  
  // //takes a polymesh and creates a list of Chart objects, one for each face
  // //and a list of joins between all charts
  // static uint32_t 
  // create_chart_clusters_from_faces (Polyhedron &P, 
  //               const double cost_threshold , 
  //               const uint32_t chart_threshold, 
  //               CLUSTER_SETTINGS cluster_settings,
  //               std::map<uint32_t, uint32_t> &chart_id_map){

  //   std::stringstream report;
  //   report << "--------------------\nReport:\n----------------------\n";

  //   //calculate areas of each face
  //   std::cout << "Calculating face areas...\n";
  //   std::map<face_descriptor,double> fareas;
  //   for(face_descriptor fd: faces(P)){
  //     fareas[fd] = CGAL::Polygon_mesh_processing::face_area  (fd,P);
  //   }
  //   //calculate normals of each faces
  //   std::cout << "Calculating face normals...\n";
  //   std::map<face_descriptor,Vector> fnormals;
  //   CGAL::Polygon_mesh_processing::compute_face_normals(P,boost::make_assoc_property_map(fnormals));

  //   //get boost face iterator
  //   face_iterator fb_boost, fe_boost;
  //   boost::tie(fb_boost, fe_boost) = faces(P);

  //   //each face begins as its own chart
  //   std::cout << "Creating initial charts...\n";
  //   std::vector<Chart> charts;
  //   for ( Facet_iterator fb = P.facets_begin(); fb != P.facets_end(); ++fb){

  //     //init chart instance for face
  //     Chart c(charts.size(),*fb, fnormals[*fb_boost], fareas[*fb_boost]);
  //     charts.push_back(c);

  //     fb_boost++;
  //   }
  //   //create possible join list/queue. Each original edge in the mesh becomes a join (if not a boundary edge)
  //   std::cout << "Creating initial joins...\n";
  //   std::list<JoinOperation> joins;
  //   std::list<JoinOperation>::iterator it;

  //   int edgecount = 0;

  //   for( Edge_iterator eb = P.edges_begin(), ee = P.edges_end(); eb != ee; ++ eb){

  //     edgecount++;

  //     //only create join if halfedge is not a boundary edge
  //     if ( !(eb->is_border()) && !(eb->opposite()->is_border()) )
  //     {
  //           uint32_t face1 = eb->facet()->id();
  //           uint32_t face2 = eb->opposite()->facet()->id();
  //           JoinOperation join (face1,face2,JoinOperation::cost_of_join(charts[face1],charts[face2], cluster_settings));
  //           joins.push_back(join);
  //     }
  //   } 
  //   std::cout << joins.size() << " joins\n" << edgecount << " edges\n";

  //   cluster_faces(charts, joins, cost_threshold, chart_threshold, cluster_settings,chart_id_map);

  //   return populate_chart_LUT(charts, chart_id_map);

  // }

  //fill chart_id_map from chart vector
  static uint32_t populate_chart_LUT(std::vector<Chart> &charts, std::map<uint32_t, uint32_t> &chart_id_map){
    
    chart_id_map.clear();

    //populate LUT for face to chart mapping
    //count charts on the way to apply new chart ids
    uint32_t active_charts = 0;
    for (uint32_t id = 0; id < charts.size(); ++id) {
      auto& chart = charts[id];
      if (chart.active) {
        for (auto& f : chart.facets) {
          chart_id_map[f->id()] = active_charts;
        }
        active_charts++;
      }
    }

    return active_charts;
  }

  //fills inverse index linking each chart with joins that reference it
  static void populate_inverse_index( std::map<uint32_t, std::vector<std::shared_ptr<JoinOperation> > > &chart_to_join_inverse_index,
                                      std::vector<Chart> &charts,
                                      std::vector<std::shared_ptr<JoinOperation> > &joins){



    if (charts.size() == 0)
    {
      std::cout << "WARNING: no charts received in populate_inverse_index() \n";
      return; 
    }    
    if (joins.size() == 0)
    {
      std::cout << "WARNING: no joins received in populate_inverse_index() \n";
      return; 
    }

    if (chart_to_join_inverse_index.size() == 0)
    {
      std::cout << "building inverse index from scratch...";

      //initialise map?
      // for (int i = 0; i < charts.size())
    }

    //for each join, add a pointer to the list for each relevant chart
    for (uint32_t i = 0; i < joins.size(); i++){

      // chart_to_join_inverse_index[joins[i].get_chart1_id()].push_back( &(joins[i]) );
      // chart_to_join_inverse_index[joins[i].get_chart2_id()].push_back( &(joins[i]) );

      chart_to_join_inverse_index[joins[i]->get_chart1_id()].push_back( std::shared_ptr<JoinOperation>( joins[i] ) );
      chart_to_join_inverse_index[joins[i]->get_chart2_id()].push_back( std::shared_ptr<JoinOperation>( joins[i] ) );

    }

    std::cout << "Inverse index populated with " << chart_to_join_inverse_index.size() << " entries\n";

    //debug only - checking inverse index was created correctly
    // for(auto& entry : chart_to_join_inverse_index){
    //   if(entry.second.size() == 0)
    //     std::cout << "Chart with no joins: " << entry.first << std::endl;
    // }

  }

};

