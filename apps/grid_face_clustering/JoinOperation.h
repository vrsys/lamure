#include "Utils.h"

struct JoinOperation {

  uint32_t chart1_id;
  uint32_t chart2_id;
  double cost;

  bool active = true;

  JoinOperation(uint32_t _c1, uint32_t _c2) : chart1_id(_c1), chart2_id(_c2){
    cost = 0;
  }
  JoinOperation(uint32_t _c1, uint32_t _c2, double _cost) : chart1_id(_c1), chart2_id(_c2), cost(_cost){
    if (_c1 == _c2)
    {
      std::cerr << "ERROR: Join added between the same chart! (chart " << _c1 << ")\n";
    }
  }

  //calculates cost of joining these 2 charts
  static double cost_of_join(Chart &c1, Chart &c2 ,CLUSTER_SETTINGS& cluster_settings){

    // std::cout << "-----------------\n";

    if(c1.area == 0)
      std::cout << "Chart has 0 area: " << c1.id << std::endl;
    else if(c2.area == 0)
      std::cout << "Chart has 0 area: " << c2.id << std::endl;

    double error = 0;

    Vector fit_plane_normal;

    double e_fit =       cluster_settings.e_fit_cf *   Chart::get_fit_error(c1,c2, fit_plane_normal);

    fit_plane_normal = Utils::normalise(fit_plane_normal);

    double e_direction = cluster_settings.e_ori_cf *   Chart::get_direction_error(c1,c2,fit_plane_normal); 
    double e_shape =     cluster_settings.e_shape_cf * Chart::get_compactness_of_merged_charts(c1,c2);

    error = e_fit + e_direction + e_shape;

    if (std::isnan(error))
    {
      std::cout << "Nan error [" << c1.id << ", " << c2.id << "]: " << error << ", e_fit: " << e_fit << ", e_ori: " << e_direction << ", e_shape: " << e_shape << std::endl;
    }

    return error;
  }

  static bool sort_joins (JoinOperation j1, JoinOperation j2) {
    return (j1.cost < j2.cost);
  }

  static bool sort_join_ptrs (JoinOperation* j1, JoinOperation* j2) {
    return (j1->cost < j2->cost);
  }

  int results_in_chart_with_neighbours(std::vector<Chart> &charts,
                                       std::map<uint32_t, uint32_t> &chart_id_map ){
    Chart &c1 = charts[this->chart1_id];
    Chart &c2 = charts[this->chart2_id];

    int combined_nbrs = c1.has_border_edge | c2.has_border_edge;

    //get number of neighbours per chart
    c1.create_neighbour_set(chart_id_map);
    c2.create_neighbour_set(chart_id_map);


    combined_nbrs += c1.neighbour_charts.size();
    combined_nbrs += c2.neighbour_charts.size();

    return combined_nbrs - 2;
  }

};
