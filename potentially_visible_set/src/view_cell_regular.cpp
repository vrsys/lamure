#include "lamure/pvs/view_cell_regular.h"

namespace lamure
{
namespace pvs
{

view_cell_regular::
view_cell_regular()
{
	cell_size_ = 1.0;
	position_center_ = scm::math::vec3d(0.0, 0.0, 0.0);
}

view_cell_regular::
view_cell_regular(const double& cell_size, const scm::math::vec3d& position_center)
{
	cell_size_ = cell_size;
	position_center_ = position_center;
}

view_cell_regular::
~view_cell_regular()
{
}

scm::math::vec3d view_cell_regular::
get_size() const
{
	return scm::math::vec3d(cell_size_, cell_size_, cell_size_);
}

scm::math::vec3d view_cell_regular::
get_position_center() const
{
	return position_center_;
}

void view_cell_regular::
set_visibility(const unsigned int& object_id, const unsigned int& node_id)
{
	std::vector<bool>* node_visibility = &visibility_[object_id];
	
	if(node_visibility->size() <= node_id)
	{
		node_visibility->resize(node_id + 1);
	}

	node_visibility->at(node_id) = true;
}

bool view_cell_regular::
get_visibility(const unsigned int& object_id, const unsigned int& node_id) const
{
	const std::vector<bool>* node_visibility = &visibility_.at(object_id);
	
	if(node_visibility->size() <= node_id)
	{
		return false;
	}

	return node_visibility->at(node_id);
}

std::map<unsigned int, std::vector<unsigned int>> view_cell_regular::
get_visible_indices() const
{
	std::map<unsigned int, std::vector<unsigned int>> indices;

	for(std::map<unsigned int, std::vector<bool>>::const_iterator iter = visibility_.begin(); iter != visibility_.end(); ++iter)
	{
		for(unsigned int node_index = 0; node_index < iter->second.size(); ++node_index)
		{
			if(iter->second.at(node_index))
			{
				indices[iter->first].push_back(node_index);
			}
		}
	}

	return indices;
}

}
}
