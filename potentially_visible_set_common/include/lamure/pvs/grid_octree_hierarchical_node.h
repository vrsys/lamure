#ifndef LAMURE_PVS_GRID_OCTREE_HIERARCHICAL_NODE_H
#define LAMURE_PVS_GRID_OCTREE_HIERARCHICAL_NODE_H

#include <vector>
#include <set>
#include <map>

#include <scm/core/math.h>
#include <lamure/types.h>
#include "lamure/pvs/grid_octree_node.h"

namespace lamure
{
namespace pvs
{

class grid_octree_hierarchical_node : public grid_octree_node
{
public:
	grid_octree_hierarchical_node();
	grid_octree_hierarchical_node(const double& cell_size, const scm::math::vec3d& position_center);
	~grid_octree_hierarchical_node();

	void split();

	void combine_visibility(const std::vector<node_t>& ids, const unsigned short& num_allowed_unequal_elements);

protected:
	grid_octree_hierarchical_node* get_parent_node();
};

}
}

#endif