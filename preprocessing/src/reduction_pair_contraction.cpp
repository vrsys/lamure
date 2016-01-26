// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/reduction_pair_contraction.h>
#include <lamure/pre/surfel.h>
#include <set>
#include <functional>
#include <queue>
#include <deque>
#include <map>
#include <cmath>
#include <unordered_map>

namespace lamure {

namespace pre {
struct quadric {
  quadric(vec4r const& hessian) 
   :values{hessian.x * hessian.x, hessian.y * hessian.x, hessian.z * hessian.x, hessian.w * hessian.x,
                                  hessian.y * hessian.y, hessian.z * hessian.y, hessian.w * hessian.y,
                                                         hessian.z * hessian.z, hessian.w * hessian.z,
                                                                                hessian.w * hessian.w}
  {}

  std::array<real, 10> values;
};

struct edge_t
{
  edge_t(surfel_id_t p1, surfel_id_t p2) 
   :a{p1 < p2 ? p1 : p2}
   ,b{p1 < p2 ? p2 : p1}
   {};

  surfel_id_t a;
  surfel_id_t b;
};

bool operator==(const edge_t& e1, const edge_t& e2) {
  if(e1.a == e2.a)   {
    return e1.b == e2.b;
  }
  return false;
}

bool operator<(const edge_t& e1, const edge_t& e2) {
  if(e1.a < e2.a) {
    return true;
  }
  else {
    if(e1.a == e2.a) {
      return e1.b < e2.b;
    }
    else return false;
  }
}

std::ostream& operator<<(std::ostream& os, const edge_t& e) {
  os << "<" << e.a << "--" << e.b << ">";
  return os;
}

struct contraction_op;

struct contraction {
  contraction(edge_t e, mat4r quad, real err, surfel surf)
   :edge{e}
   ,quadric{quad}
   ,error{err}
   ,new_surfel{surf}
  {}

  edge_t edge;
  mat4r quadric;
  real error;
  surfel new_surfel;

  contraction_op* cont_op;
};

bool operator<(const contraction& c1, const contraction& c2) {
  return c1.error < c2.error;
}

struct contraction_op {
  contraction_op(contraction* c)
   :cont{c}
  {};

  contraction* cont;
};

bool operator<(const contraction_op& c1, const contraction_op& c2) {
  // invalid op -> must return false to form strict-weak ordering
  // otherwise undefined behaviour for sort
  return c1.cont->error > c2.cont->error;
}

}
}

const size_t num_surfels_per_node = 3000;
const size_t num_nodes_per_level = 14348907;
namespace std {
template <> struct hash<lamure::pre::edge_t>
{
  size_t operator()(lamure::pre::edge_t const & e) const noexcept
  {
    uint16_t h1 = e.a.node_idx * num_surfels_per_node + e.a.surfel_idx;  
    uint16_t h2 = e.b.node_idx * num_surfels_per_node + e.b.surfel_idx;  
    size_t hash = h1;
    hash += h2 >> 16;
    return hash;
  }
};
template <> struct hash<lamure::surfel_id_t>
{
  size_t operator()(lamure::surfel_id_t const & s) const noexcept
  {
    return s.node_idx * num_surfels_per_node + s.surfel_idx;  
  }
};
}
namespace lamure {
namespace pre {

bool a = false;
size_t num_neighbours = 1;

real sum(const mat4r& quadric) {  
  real sum = 0;
  for(int i = 0; i < 16; ++i) {
    sum += quadric[i] * quadric[i];
  }
  return sum;
}

vec3b n_to_c(vec3f normal) {
  // normal = normal * 255;
  normal = (normal * 0.5f + 0.5f) * 255;
  return vec3b{uint8_t(normal.x), uint8_t(normal.y), uint8_t(normal.z)}; 
}
vec3b n_to_cb(vec3f normal) {
  normal.x = (normal.x < 0) ? 0 : 1;
  normal.y = (normal.y < 0) ? 0 : 1;
  normal.z = (normal.z < 0) ? 0 : 1;
  normal = normal * 255;
  // normal = (normal * 0.5f + 0.5f) * 255;
  return vec3b{uint8_t(normal.x), uint8_t(normal.y), uint8_t(normal.z)}; 
}

static vec3b heatmap(float norm_val) {
  vec3f color{0.0f};
  float third = 1 / 3.0f;
  if(norm_val <= third) {
    color.g = 1.0f;
    color.r =std::max(0.0f, (third - norm_val) * 3.0f);
  }
  else if(norm_val <= third * 2.0f) { 
    norm_val -= third;
    color.b =std::max(0.0f, norm_val * 3.0f);
    color.g =std::max(0.0f, (third - norm_val) * 3.0f);
  }
  else {
    norm_val -= 2.0f * third;
    color.r =std::max(0.0f, norm_val * 3.0f);
    color.b =std::max(0.0f, (third - norm_val) * 3.0f);
  }
  color *= 255;
  return vec3b{uint8_t(color.x), uint8_t(color.y), uint8_t(color.z)};
}

real qlength(const mat4r& quadric) {
  real a = sqrt(quadric[0]);
  vec4r n = vec4r{quadric[0], quadric[1], quadric[2], quadric[3]};
  n /= a;
  return scm::math::length_sqr(n);
}


surfel_mem_array reduction_pair_contraction::
create_lod(real& reduction_error,
          const std::vector<surfel_mem_array*>& input,
          const uint32_t surfels_per_node,
          const bvh& tree,
          const size_t start_node_id) const
{
  vec3f n = vec3f{0,0,1};
  vec3r p = vec3r{1,2,1};
  vec3r p2 = vec3r{9,5,1};
  vec3r p3 = vec3r{1,2,3};
  // column major
  mat4r q = surfel_quadric(n, p);
  std::cout << q << std::endl;
  std::cout << quadric_error(p2, q) << std::endl;
  std::cout << quadric_error(p3, q) << std::endl;
  // exit(0); 



  const uint32_t fan_factor = input.size();
  size_t num_surfels = 0;
  size_t min_num_surfels = input[0]->length(); 
  //compute max total number of surfels from all nodes
  for (size_t node_idx = 0; node_idx < fan_factor; ++node_idx) {
    num_surfels += input[node_idx]->length();

    if (input[node_idx]->length() < min_num_surfels) {
      min_num_surfels = input[node_idx]->length();
    }
  }

  std::map<surfel_id_t, mat4r> quadrics{};
  std::vector<std::vector<surfel>> node_surfels{input.size() + 1, std::vector<surfel>{}};
  std::set<edge_t> edges{};
  std::cout << "creating quadrics" << std::endl;
  // accumulate edges and point quadrics
  for (node_id_type node_idx = 0; node_idx < fan_factor; ++node_idx) {
    for (size_t surfel_idx = 0; surfel_idx < input[node_idx]->length(); ++surfel_idx) {
      
      surfel curr_surfel = input[node_idx]->read_surfel(surfel_idx);
      // curr_surfel.color() = n_to_c(curr_surfel.normal());
      // input[node_idx]->write_surfel(curr_surfel, surfel_idx);
      // save surfel
      node_surfels[node_idx].push_back(curr_surfel);
      surfel_id_t curr_id = surfel_id_t{node_idx, surfel_idx};

      assert(node_idx < num_nodes_per_level && surfel_idx < num_surfels_per_node);
      // get and store neighbours
      auto nearest_neighbours = get_local_nearest_neighbours(input, number_of_neighbours_, curr_id);
      std::vector<surfel_id_t> neighbour_ids{};
      for (const auto& pair : nearest_neighbours) {
        neighbour_ids.push_back(pair.first);
      }

      mat4r curr_quadric = mat4r::zero();
      for (auto const& neighbour : nearest_neighbours) {
        edge_t curr_edge = edge_t{curr_id, neighbour.first}; 
        assert(neighbour.first.node_idx < num_nodes_per_level && neighbour.first.surfel_idx <num_surfels_per_node);
        if (edges.find(curr_edge) == edges.end()) {
          edges.insert(curr_edge);
        }
        // accumulate quadric
        surfel neighbour_surfel = input[neighbour.first.node_idx]->read_surfel(neighbour.first.surfel_idx);
        curr_quadric += edge_quadric(curr_surfel.normal(), neighbour_surfel.normal(), curr_surfel.pos(), neighbour_surfel.pos());
      }
      // curr_quadric[3] /= real(nearest_neighbours.size());
      // curr_quadric[7] /= real(nearest_neighbours.size());
      // curr_quadric[11] /= real(nearest_neighbours.size());
      // curr_quadric[15] /= real(nearest_neighbours.size() * nearest_neighbours.size());
      // quadrics[curr_id] = curr_quadric;
      quadrics[curr_id] = surfel_quadric(curr_surfel.normal(), curr_surfel.pos());
      // std::cout << sum(curr_quadric) << std::endl;
    }
  }


  // allocate space for new surfels that will be created
  node_surfels.back() = std::vector<surfel>{num_surfels - surfels_per_node};

  real error_min = std::numeric_limits<real>::max();
  real error_max = 0;
  real d_max = 0;
  real d_min = 0;
  real length_min = 0;
  real length_max = 0;
  std::cout << "creating contractions" << std::endl;
  auto create_contraction = [&node_surfels, &quadrics, &error_max, &error_min, &d_max, &d_min, &length_max, &length_min](const edge_t& edge)->contraction {
    const surfel& surfel1 = node_surfels[edge.a.node_idx][edge.a.surfel_idx];
    const surfel& surfel2 = node_surfels[edge.b.node_idx][edge.b.surfel_idx];
    // new surfel is mean of both old surfels
    surfel new_surfel = surfel{(surfel1.pos() + surfel2.pos()) * 0.5f,
                          (surfel1.color() + surfel2.color()) * 0.5f,
                          (surfel1.radius() + surfel2.radius()) * 0.5f,
                          (normalize(surfel1.normal() + surfel2.normal()))
                          };
    mat4r q1 = quadrics.at(edge.a);
    mat4r q2 = quadrics.at(edge.b);
    if(dot(q1.column(0), q2.column(0)) < 0) {
      q1 = q1 * -1.0;
    }
    mat4r new_quadric = (q1 + q2);
    // vec3f n = vec3f{0,0,1};
    // vec3r p = vec3r{1,2,1};
    // mat4r q = surfel_quadric(n,p);
    // std::cout << "diff " << quadric_error(surfel1.pos(), q) << " " << quadric_error(surfel2.pos(), q) << std::endl;
    real error = quadric_error(new_surfel.pos(), new_quadric);
    real length = qlength(new_quadric);
    if (error > error_max) error_max = error;
    if (error < error_min) error_min = error;
    if (length > length_max) length_max = length;
    if (length < length_min) length_min = length;

    return contraction{edge, std::move(new_quadric), error, std::move(new_surfel)};
  };

  // store contractions and operations in queue
  std::unordered_map<surfel_id_t, std::map<surfel_id_t,std::shared_ptr<contraction>>> contractions{};
  std::vector<std::shared_ptr<contraction_op>> contraction_queue{};
  for (const auto& edge : edges) {
    if (contractions[edge.a].find(edge.b) != contractions[edge.a].end()) {
      continue;
    }
    // map contraction to both surfels
    contractions[edge.a][edge.b] = std::make_shared<contraction>(create_contraction(edge));
    contractions[edge.b][edge.a] = contractions.at(edge.a).at(edge.b);
    // check surfel id bounds
    assert(edge.a.node_idx < num_nodes_per_level && edge.a.surfel_idx <num_surfels_per_node);
    assert(edge.b.node_idx < num_nodes_per_level && edge.b.surfel_idx <num_surfels_per_node);
    // store contraction operation pointing to new contraction
    contraction_op op{contractions.at(edge.a).at(edge.b).get()};
    contraction_queue.push_back(std::make_shared<contraction_op>(op));  
    // let contraction point to related contraction_op
    contractions.at(edge.a).at(edge.b)->cont_op = contraction_queue.back().get();  
    //check pointer correctness
    assert(contraction_queue.back().get() == contraction_queue.back()->cont->cont_op);
    assert(contractions.at(edge.a).at(edge.b).get() == contractions.at(edge.a).at(edge.b).get()->cont_op->cont);
    assert(contractions.at(edge.a).at(edge.b).get() == contractions.at(edge.b).at(edge.a).get());
  }

  real pd_max = 0;
  real pd_min = 0;
  for(const auto& qpair : quadrics)
 {
    real d = qpair.second[15];
    if (d > pd_max) pd_max = d;
    if (d < pd_min) pd_min = d;
 }
  std::cout << "error min " << error_min << " max " << error_max << std::endl;
  std::cout << "contraction d min " << d_min << " max " << d_max << std::endl;
  std::cout << "surfel d min " << pd_min << " max " << pd_max << std::endl;
  std::cout << "length min " << length_min << " max " << length_max << std::endl;
  exit(0); 

  error_min = std::numeric_limits<real>::max();
  error_max = 0;
  for (node_id_type node_idx = 0; node_idx < fan_factor; ++node_idx) {
    for (size_t surfel_idx = 0; surfel_idx < input[node_idx]->length(); ++surfel_idx) {
      real error = 0;
      size_t i = 0;
      for(const auto& pair : contractions.at(surfel_id_t{node_idx, surfel_idx})) {
        error += pair.second->error;
        ++i;
      
      }
      error /= real(i);
      if (error > error_max) error_max = error;
      if (error < error_min) error_min = error;
    }
  }
  for (node_id_type node_idx = 0; node_idx < fan_factor; ++node_idx) {
    for (size_t surfel_idx = 0; surfel_idx < input[node_idx]->length(); ++surfel_idx) {
      surfel& curr_surfel = node_surfels[node_idx][surfel_idx];
      real error = 0;
      real d = 0;
      vec4r norm = vec4r{0};
      real length = 0;
      size_t i = 0;
      for(const auto& pair : contractions.at(surfel_id_t{node_idx, surfel_idx})) {
        error += pair.second->error;
        real a = sqrt(pair.second->quadric[0]);
        vec4r n = vec4r{pair.second->quadric[0], pair.second->quadric[1], pair.second->quadric[2], pair.second->quadric[3]};
        n /= a;
        // norm += n;
        norm += pair.second->quadric.column(0);
        length = qlength(pair.second->quadric);
        d += pair.second->quadric[15];
        ++i;
      }
      error /= real(i);
      d /= real(i);
      length /= real(i);
      // magnitude of quadric
      // curr_surfel.color() = heatmap((length - length_min) / (length_max - length_min));
      // magnitude of quadric
      // curr_surfel.color() = heatmap((length - length_min) / (length_max - length_min));
      // offset of contraction quadric
      // curr_surfel.color() = heatmap((d - d_min) / (d_max - d_min));
      // offset of point quadric
      // curr_surfel.color() = heatmap((quadrics.at(surfel_id_t{node_idx, surfel_idx})[15] - pd_min) / (pd_max - pd_min));
      // error of contraction
      curr_surfel.color() = heatmap((error - error_min) / (error_max - error_min));
      // binary dir of surfel normal
      // curr_surfel.color() = n_to_c(curr_surfel.normal());
      // dir of contraction normal
      // curr_surfel.color() = n_to_c(normalize(vec3f{norm.x, norm.y, norm.z}));
      // curr_surfel.color() = vec3b{127, 127, 127};
      // write to orig data
      input[node_idx]->write_surfel(curr_surfel, surfel_idx);
    }
  }

  
  std::cout << "doing contractions" << std::endl;
  // work off queue until target num of surfels is reached
  for (size_t i = 0; i < num_surfels - surfels_per_node; ++i) {
    // update queue, cheapest contraction at the back
    std::sort(contraction_queue.begin(), contraction_queue.end(), 
      [](const std::shared_ptr<contraction_op>& a, const std::shared_ptr<contraction_op>& b){
         return *a < *b;
       }
      );

    contraction curr_contraction = *(contraction_queue.back()->cont);
    contraction_queue.pop_back();
    for (int i = contraction_queue.size() - 1; i > 0; --i) {
      if (curr_contraction.error > contraction_queue[i]->cont-> error) {
        throw std::exception();
      }
    }
    
    surfel_id_t new_id = surfel_id_t{node_surfels.size()-1, i};
    // save new surfels in vector at back
    surfel& new_surfel = curr_contraction.new_surfel;
    // save new surfel
    // new_surfel.color() = vec3b{255,255,255};
    // new_surfel.color() = heatmap((curr_contraction.error - error_min) / (error_max - error_min));
    node_surfels.back()[i] = new_surfel;
    // std::cout << node_surfels.back()[i].radius() << std::endl;

    const surfel_id_t& old_id_1 = curr_contraction.edge.a;
    const surfel_id_t& old_id_2 = curr_contraction.edge.b;
    // invalidate old surfels
    // if (old_id_1.node_idx < fan_factor) {
    //   surfel surf = node_surfels[old_id_1.node_idx][old_id_1.surfel_idx];
    //   surf.color() = vec3b{255,255,255};
    //   input[old_id_1.node_idx]->write_surfel(surf, old_id_1.surfel_idx);
    // }
    node_surfels[old_id_1.node_idx][old_id_1.surfel_idx].radius() = -1.0f;
    // if (old_id_2.node_idx < fan_factor) {
    //   surfel surf = node_surfels[old_id_2.node_idx][old_id_2.surfel_idx];
    //   surf.color() = vec3b{255,255,255};
    //   input[old_id_2.node_idx]->write_surfel(surf, old_id_2.surfel_idx);
    // }
    node_surfels[old_id_2.node_idx][old_id_2.surfel_idx].radius() = -1.0f;
    // add new point quadric
    quadrics[new_id] = curr_contraction.quadric;
    // delete old point quadrics
    quadrics.erase(curr_contraction.edge.a);
    quadrics.erase(curr_contraction.edge.b);
   
    auto update_contraction = [&create_contraction, &contractions, &curr_contraction]
      (const surfel_id_t& new_id, const surfel_id_t& old_id, const std::pair<const surfel_id_t, std::shared_ptr<contraction>>& cont) {
        edge_t new_edge = edge_t{new_id, cont.first};
        // store new contraction
        contractions[new_id][cont.first] = std::make_shared<contraction>(create_contraction(new_edge));
        // update contractions of neighbour
        // contractions.at(new_id).at(cont.first)->error = curr_contraction.error + contractions.at(cont.first).at(old_id)->error;
        assert(contractions.find(cont.first) != contractions.end());
        contractions.at(cont.first).erase(old_id);
        contractions.at(cont.first)[new_id] = contractions.at(new_id).at(cont.first);
        // get attached operation
        contraction_op* operation = cont.second->cont_op;
        // transfer contraction op
        contractions.at(new_id).at(cont.first)->cont_op = operation;
        operation->cont = contractions.at(new_id).at(cont.first).get();
        // check for pointer correctness
        assert(contractions.at(new_edge.a).at(new_edge.b).get() == contractions.at(new_edge.a).at(new_edge.b).get()->cont_op->cont);
        assert(contractions.at(new_edge.a).at(new_edge.b).get() == contractions.at(new_edge.b).at(new_edge.a).get());
    };

    for(const auto& cont : contractions.at(old_id_1)) {
      if (cont.first != old_id_2) {
        update_contraction(new_id, old_id_1, cont);
        assert(contractions.at(cont.first).find(old_id_1) == contractions.at(cont.first).end());
      }
      else {
        // invalidate operation
        cont.second->cont_op->cont = nullptr;
      }
    }
    // in case no new contractions were created yet
    if(contractions.find(new_id) == contractions.end()) {
      std::cout << old_id_1 << std::endl;
      contractions[new_id];
      // throw std::exception{};
    }
    for(const auto& cont : contractions.at(old_id_2)) {
      if (cont.first != old_id_1) {
        if(contractions.at(new_id).find(cont.first) == contractions.at(new_id).end()) {
          update_contraction(new_id, old_id_2, cont);
        }
        else {
          // already added -> remove duplicate contractions
          contractions.at(cont.first).erase(old_id_2);
          // // and invalidate respective operation
          cont.second->cont_op->cont = nullptr;
        }
        assert(contractions.at(cont.first).find(old_id_2) == contractions.at(cont.first).end());
      }
      else {
        // invalidate operation
        cont.second->cont_op->cont = nullptr;
      }
    }
    // remove old mapping
    contractions.erase(old_id_1);
    contractions.erase(old_id_2);
    //remove invalid contraction operations
    contraction_queue.erase(std::remove_if(contraction_queue.begin(), contraction_queue.end(), [](const std::shared_ptr<contraction_op>& op){return op->cont == nullptr;}), contraction_queue.end());
  }

  std::cout << "copying surfels" << std::endl;
  // save valid surfels in mem array
  // for(const auto& surf : node_surfels.back()) {
  //   std::cout << surf.radius() << std::endl;
  // }

  surfel_mem_array mem_array(std::make_shared<surfel_vector>(surfel_vector()), 0, 0);
  for (auto& node : node_surfels) {
    for (auto& surfel : node) {
      if (surfel.radius() > 0.0f) {
        // if(node == node_surfels.back()) {
        //   surfel.color() = vec3b{255};
        // }
        mem_array.mem_data()->push_back(surfel);
      }
    }
  }
  mem_array.set_length(mem_array.mem_data()->size());

  reduction_error = 0.0;

  return mem_array;
}


lamure::mat4r edge_quadric(const vec3f& normal_p1, const vec3f& normal_p2, const vec3r& p1, const vec3r& p2)
{
    vec3f f = scm::math::length_sqr(normal_p1) > scm::math::length_sqr(normal_p2) ? normal_p1 : normal_p2; 
    vec3f s = scm::math::length_sqr(normal_p1) > scm::math::length_sqr(normal_p2) ? normal_p2 : normal_p1; 
    if (dot(f, s)< 0) {
      s = -s;
    }
    vec3r edge_dir = normalize(scm::math::length_sqr(p2) > scm::math::length_sqr(p1) ? p2 - p1 : p1 - p2);
    vec3r tangent = normalize(cross(normalize(vec3r(s + f)), edge_dir));

    vec3r normal = cross(tangent, edge_dir);
    normal /= (normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
    normal = normalize(normal);
    normal = normalize(f + s);
    // if (dot(p2, normal) < 0) {
    //   normal = -normal;
    //   // std::cout << "swap" << std::endl;
    // }
    vec4r hessian = vec4r{normal, -dot(p1, normal)}; 
    // hessian /= hessian.w;
    lamure::mat4r quadric = mat4r{hessian * hessian.x,
                                  hessian * hessian.y,
                                  hessian * hessian.z,
                                  hessian * hessian.w};

    // std::cout << quadric_error((p1 + p2) * 0.5, quadric) << std::endl;
    if(sum(quadric) > 100) {
      throw std::exception();
    }
    return quadric;
}

lamure::mat4r surfel_quadric(const vec3f& norm, const vec3r& p) {
  vec3r normal = vec3r{normalize(norm)};
  vec4r hessian = vec4r{normal, -dot(p, normal)}; 
  lamure::mat4r quadric = mat4r{hessian,
                                hessian,
                                hessian,
                                hessian};  
  // lamure::mat4r quadric = mat4r{hessian * hessian.x,
  //                               hessian * hessian.y,
  //                               hessian * hessian.z,
  //                               hessian * hessian.w};  
  return quadric;
}

lamure::real quadric_error(const vec3r& p, const mat4r& quadric)
{
  // real a = sqrt(quadric[0]);
  // vec4r n = vec4r{quadric[0], quadric[1], quadric[2], quadric[3]};
  // n /= a;
  // real a = dot(quadric.column(0), vec4r{p, 1});
  // return a * a;
  // return fabs(a);
  return quadric[0] * p.x * p.x + 2 * quadric[4] * p.x * p.y  + 2 * quadric[8] * p.x * p.z + 2 * quadric[12] * p.x 
                                +     quadric[5] * p.y * p.y  + 2 * quadric[9] * p.y * p.z + 2 * quadric[13] * p.y 
                                                              +     quadric[10] * p.z * p.z+ 2 * quadric[14] * p.z 
                                                                                           +     quadric[15];  
}

// get k nearest neighbours in simplification nodes
std::vector<std::pair<surfel_id_t, real>> 
get_local_nearest_neighbours(const std::vector<surfel_mem_array*>& input,
                             size_t num_local_neighbours,
                             surfel_id_t const& target_surfel) {

    //size_t current_node = target_surfel.node_idx;
    vec3r center = input[target_surfel.node_idx]->read_surfel(target_surfel.surfel_idx).pos();

    std::vector<std::pair<surfel_id_t, real>> candidates;
    real max_candidate_distance = std::numeric_limits<real>::infinity();

    for (size_t local_node_id = 0; local_node_id < input.size(); ++local_node_id) {
        for (size_t surfel_id = 0; surfel_id < input[local_node_id]->length(); ++surfel_id) {
            if (surfel_id != target_surfel.surfel_idx || local_node_id != target_surfel.node_idx) {
                const surfel& current_surfel = input[local_node_id]->read_surfel(surfel_id);
                real distance_to_center = scm::math::length_sqr(center - current_surfel.pos());

                if (candidates.size() < num_local_neighbours || (distance_to_center < max_candidate_distance)) {
                    if (candidates.size() == num_local_neighbours)
                        candidates.pop_back();

                    candidates.push_back(std::make_pair(surfel_id_t(local_node_id, surfel_id), distance_to_center));

                    for (uint16_t k = candidates.size() - 1; k > 0; --k) {
                        if (candidates[k].second < candidates[k - 1].second) {
                            std::swap(candidates[k], candidates[k - 1]);
                        }
                        else
                            break;
                    }

                    max_candidate_distance = candidates.back().second;
                }
            }
        }
    }

    return candidates;
}

} // namespace pre
} // namespace lamure
