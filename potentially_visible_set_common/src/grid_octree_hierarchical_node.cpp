#include "lamure/pvs/grid_octree_hierarchical_node.h"

#include <iostream>

namespace lamure
{
namespace pvs
{

grid_octree_hierarchical_node::
grid_octree_hierarchical_node() : grid_octree_node()
{
}

grid_octree_hierarchical_node::
grid_octree_hierarchical_node(const double& cell_size, const scm::math::vec3d& position_center) : grid_octree_node(cell_size, position_center)
{
}

grid_octree_hierarchical_node::
~grid_octree_hierarchical_node()
{
}

void grid_octree_hierarchical_node::
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

			child_nodes_[child_index] = grid_octree_hierarchical_node(new_size, new_pos);
		}
	}
}

void grid_octree_hierarchical_node::
combine_visibility(const std::vector<node_t>& ids, const unsigned short& num_allowed_unequal_elements)
{
	if(this->has_children())
	{
		// Make sure all children have been processed recursively.
		for(size_t child_index = 0; child_index < 8; ++child_index)
		{
			grid_octree_hierarchical_node* current_node = (grid_octree_hierarchical_node*)this->get_child_at_index(child_index);
			current_node->combine_visibility(ids, num_allowed_unequal_elements);
		}

		size_t mod_counter = 0;

		for(model_t model_index = 0; model_index < ids.size(); ++model_index)
		{
			for(node_t node_index = 0; node_index < ids[model_index]; ++node_index)
			{
				unsigned short appearance_counter = 0;

				for(size_t child_index = 0; child_index < 8; ++child_index)
				{
					grid_octree_hierarchical_node* current_node = (grid_octree_hierarchical_node*)this->get_child_at_index(child_index);
					if(current_node->get_visibility(model_index, node_index))
					{
						appearance_counter++;
					}
				}

				// If an element is common among all children (or a given threshold of children) it is moved to the parent.
				if((8 - appearance_counter) <= num_allowed_unequal_elements)
				{
					this->set_visibility(model_index, node_index, true);

					for(size_t child_index = 0; child_index < 8; ++child_index)
					{
						this->get_child_at_index(child_index)->set_visibility(model_index, node_index, false);
					}
 
					mod_counter++;
				}
			}
		}

		std::cout << "mods done: " << mod_counter << std::endl;
	}
}

}
}
