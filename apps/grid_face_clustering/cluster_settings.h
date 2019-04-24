struct CLUSTER_SETTINGS
{
  double e_fit_cf;
  double e_ori_cf;
  double e_shape_cf;
  double chart_split_threshold;

  bool write_charts_as_textures = false;

  CLUSTER_SETTINGS(double ef, double eo, double es, double cst){
    e_fit_cf = ef;
    e_ori_cf = eo;
    e_shape_cf = es;
    chart_split_threshold = cst;

  }
};