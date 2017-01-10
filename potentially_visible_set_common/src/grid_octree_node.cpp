#include "lamure/pvs/grid_octree_node.h"

namespace lamure
{
namespace pvs
{

grid_octree_node::
grid_octree_node() : grid_octree_node(1.0, scm::math::vec3d(0.0, 0.0, 0.0))
{
}

grid_octree_node::
grid_octree_node(const double& cell_size, const scm::math::vec3d& position_center) : view_cell_regular(cell_size, position_center)
{
	child_nodes_ = nullptr;
}

grid_octree_node::
~grid_octree_node()
{
	collapse();
}

void grid_octree_node::
split()
{
	if(child_nodes_ == nullptr)
	{
		child_nodes_ = new grid_octree_node[8];
		
		for(size_t child_index = 0; child_index < 8; ++child_index)
		{
			scm::math::vec3d new_pos = get_position_center();
			
			scm::math::vec3d multiplier(1.0, 1.0, 1.0);

			if(child_index % 2 == 0)
			{
				multiplier.x = -1.0;
			}

			if(child_index / 4 == 1)
			{
				multiplier.y = -1.0;
			}

			if(child_index % 4 >= 2)
			{
				multiplier.z = -1.0;
			}

			double new_size = get_size().x * 0.5;
			new_pos = new_pos + (multiplier * get_size().x * 0.25);

			child_nodes_[child_index] = grid_octree_node(new_size, new_pos);
		}
	}
}

void grid_octree_node::
collapse()
{
	if(child_nodes_ != nullptr)
	{
		delete[] child_nodes_;
		child_nodes_ = nullptr;
	}
}

bool grid_octree_node::
has_children() const
{
	return child_nodes_ != nullptr;
}

grid_octree_node* grid_octree_node::
get_child_at_index(const size_t& index)
{
	if(!has_children() || index > 8)
	{
		return nullptr;
	}
	
	return &child_nodes_[index];
}

const grid_octree_node* grid_octree_node::
get_child_at_index_const(const size_t& index) const
{
	if(!has_children() || index > 8)
	{
		return nullptr;
	}
	
	return &child_nodes_[index];
}

}
}