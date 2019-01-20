#include "Utils.h"

struct JoinOperation {

  uint32_t chart1_id;
  uint32_t chart2_id;
  double cost;

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


    double error = 0;

    Vector fit_plane_normal;

    double e_fit =       cluster_settings.e_fit_cf *   Chart::get_fit_error(c1,c2, fit_plane_normal);

    fit_plane_normal = Utils::normalise(fit_plane_normal);

    double e_direction = cluster_settings.e_ori_cf *   Chart::get_direction_error(c1,c2,fit_plane_normal); 
    double e_shape =     cluster_settings.e_shape_cf * Chart::get_compactness_of_merged_charts(c1,c2);

    error = e_fit + e_direction + e_shape;

    // std::cout << "Error [" << c1.id << ", " << c2.id << "]: " << error << ", e_fit: " << e_fit << ", e_ori: " << e_direction << ", e_shape: " << e_shape << std::endl;

    return error;
  }

  static bool sort_joins (JoinOperation j1, JoinOperation j2) {
    return (j1.cost < j2.cost);
  }

};
