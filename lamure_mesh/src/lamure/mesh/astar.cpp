

#include <lamure/mesh/astar.h>

#include <algorithm>

namespace lamure {
namespace mesh {

solution_heap_t::
solution_heap_t()
: heap_base_t() {

}

solution_heap_t::
~solution_heap_t() {

}

void solution_heap_t::
update_solution(const uint32_t _id, solution_t& _solution) {

  if (slot_map_.find(_id) != slot_map_.end()) {
    solution_t& existing_solution = slots_[slot_map_[_id]];

    if (existing_solution.g_ > _solution.g_) {
      existing_solution.g_ = _solution.g_;
      existing_solution.backpointer_ = _solution.backpointer_;
      existing_solution.cost_ = existing_solution.g_ + existing_solution.h_;
      shuffle_up(slot_map_[_id]);
    }
  }

}

astar_t::astar_t() {

}

astar_t::~astar_t() {


}

void astar_t::find_shortest_path(
    int32_t start_vertex_id,
    int32_t goal_vertex_id,
    std::vector<indexed_vertex_t>& unique_vertices,
    std::vector<indexed_edge_t>& unique_edges,
    std::map<int32_t, std::set<int32_t>>& vertex_edge_adjacency_list,
    std::vector<int32_t>& shortest_vertex_path) {

    bool success = false;

    if (start_vertex_id == goal_vertex_id) {
        shortest_vertex_path.clear();
        return;
    }

    vec3r_t start_vertex = unique_vertices[start_vertex_id].v_;
    vec3r_t goal_vertex = unique_vertices[goal_vertex_id].v_;

    //open list
    solution_heap_t open_list;

    //closed list: vertex_id -> solution
    std::map<int32_t, solution_t> closed_list;

    solution_t start_node;
    start_node.h_ = math::length(start_vertex - goal_vertex);
    start_node.g_ = 0.f;
    start_node.id_ = start_vertex_id;
    start_node.backpointer_ = -1;

    open_list.push(start_node);

    uint32_t num_expansions = 0;

    while (true) {
        if (open_list.get_num_slots() == 0) {
            std::cout << "WARNING! no admissible edges found during astar search" << std::endl;
            success = false;
            break;
        }

        //fetch solution base with minimal cost
        solution_t most_promising_solution = open_list.top();
        open_list.pop();
        closed_list.insert(std::make_pair(most_promising_solution.id_, most_promising_solution));

        //check for goal node
        if (most_promising_solution.id_ == goal_vertex_id) {
            //std::cout << "LOG: astar success after " << num_expansions << std::endl;
            success = true;
            break;
        }
        
        vec3r_t& current_vertex = unique_vertices[most_promising_solution.id_].v_;

        //expand solution base
        ++num_expansions;

        //compile succssor candidates
        std::vector<int32_t> successor_vertex_ids;

        for (auto& edge_id : vertex_edge_adjacency_list[most_promising_solution.id_]) {
            auto& edge = unique_edges[edge_id];
            if (edge.v0_.id_ != most_promising_solution.id_) {
               successor_vertex_ids.push_back(edge.v0_.id_);
            }
            if (edge.v1_.id_ != most_promising_solution.id_) {
               successor_vertex_ids.push_back(edge.v1_.id_);
            }
        }

        for (auto& successor_vertex_id : successor_vertex_ids) {
            vec3r_t& successor_vertex = unique_vertices[successor_vertex_id].v_;

            solution_t successor;
            successor.h_ = math::length(successor_vertex - goal_vertex);
            successor.g_ = most_promising_solution.g_ + math::length(successor_vertex - current_vertex);
            successor.cost_ = successor.g_ + successor.h_; 
            successor.id_ = successor_vertex_id;
            successor.backpointer_ = most_promising_solution.id_;

            if (open_list.is_item_indexed(successor.id_)) {
                open_list.update_solution(successor.id_, successor);
                continue;
            }

            if (closed_list.find(successor.id_) != closed_list.end()) {
                solution_t& closed_solution = closed_list[successor.id_];

                if (closed_solution.g_ > successor.g_) {
                    closed_solution.g_ = successor.g_;
                    closed_solution.backpointer_ = successor.backpointer_;
                }
                continue;
            }

            //here, we know the solution base does not exist yet
            open_list.push(successor);

        }
    }

    if (success) {
        shortest_vertex_path.clear();

        int32_t current_vertex_id = goal_vertex_id;
        while (current_vertex_id != start_vertex_id) {
            shortest_vertex_path.push_back(current_vertex_id);
            solution_t& current_base = closed_list[current_vertex_id];
            current_vertex_id = current_base.backpointer_;
        }

        std::reverse(shortest_vertex_path.begin(), shortest_vertex_path.end());
    }


}


}
}


