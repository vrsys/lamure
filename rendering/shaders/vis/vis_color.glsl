
vec3 quick_interp(vec3 color1, vec3 color2, float value) {
  return color1 + (color2 - color1) * clamp(value, 0, 1);
}


vec3 get_color() {

  vec4 normal = inv_mv_matrix * vec4(in_normal,0.0f);
  float prov_value = 0.0;
  vec3 color = vec3(0.0);

      if (show_normals) {
        vec4 vis_normal = normal;
        if( vis_normal.z < 0 ) {
          vis_normal = vis_normal * -1;
        }
        color = vec3(vis_normal.xyz * 0.5 + 0.5);
      }
      else if (channel == 0) {
        color = vec3(in_r, in_g, in_b);
      }
      else {
        if (channel == 1) {
          prov_value = prov1;
        }
        else if (channel == 2) {
          prov_value = prov2;
        }
        else if (channel == 3) {
          prov_value = prov3;
        }
        else if (channel == 4) {
          prov_value = prov4;
        }
        else if (channel == 5) {
          prov_value = prov5;
        }
        else if (channel == 6) {
          prov_value = prov6;
        }
        if (heatmap) {
          float value = (prov_value - heatmap_min) / (heatmap_max - heatmap_min);
          color = quick_interp(heatmap_min_color, heatmap_max_color, value);
        }
        else {
          color = vec3(prov_value, prov_value, prov_value);
        }
      }

      if (show_accuracy) {
        color = color + vec3(accuracy, 0.0, 0.0);
      }

  return color;

}

