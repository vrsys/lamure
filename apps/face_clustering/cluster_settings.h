struct CLUSTER_SETTINGS
{
  double e_fit_cf;
  double e_ori_cf;
  double e_shape_cf;

  CLUSTER_SETTINGS(double ef, double eo, double es){
    e_fit_cf = ef;
    e_ori_cf = eo;
    e_shape_cf = es;
  }
};