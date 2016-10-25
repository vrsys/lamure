
#ifndef LMR_MRP_ASTAR_H_INCLUDED
#define LMR_MRP_ASTAR_H_INCLUDED

#include <lamure/types.h>
#include <lamure/util/heap.h>
#include <lamure/mesh/types.h>

#include <vector>
#include <set>
#include <map>

namespace lamure {
namespace mesh {

struct solution_t {
  float32_t h_; //heuristic remaining cost, euclidean distance to goal node
  float32_t g_; //euclidean distance of all edges traversed so far
  float32_t cost_; //g_ + h_
  int32_t id_; //vertex id
  int32_t backpointer_;
};

typedef lamure::util::heap_base_t<solution_t> solution_heap_base_t;

class solution_heap_t : public solution_heap_base_t {
public:
  solution_heap_t();
  ~solution_heap_t();

  void update_solution(const uint32_t _id, solution_t& _solution);

protected:

};

class astar_t {
public:
  astar_t();
  ~astar_t();

void find_shortest_path(
    int32_t start_vertex_id,
    int32_t goal_vertex_id,
    std::vector<indexed_vertex_t>& unique_vertices,
    std::vector<indexed_edge_t>& unique_edges,
    std::map<int32_t, std::set<int32_t>>& vertex_edge_adjacency_list,
    std::vector<int32_t>& shortest_vertex_path);

protected:

};


}
}

#endif
