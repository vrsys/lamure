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
set_visibility(const model_t& object_id, const node_t& node_id, const bool& visible)
{
	if(visibility_.size() <= object_id)
	{
		visibility_.resize(object_id + 1);
	}

	std::vector<bool>* node_visibility = &visibility_.at(object_id);
	
	if(node_visibility->size() <= node_id)
	{
		node_visibility->resize(node_id + 1);
	}

	node_visibility->at(node_id) = visible;
}

bool view_cell_regular::
get_visibility(const model_t& object_id, const node_t& node_id) const
{
	if(visibility_.size() <= object_id)
	{
		return false;
	}

	const std::vector<bool>* node_visibility = &visibility_.at(object_id);
	
	if(node_visibility->size() <= node_id)
	{
		return false;
	}

	return node_visibility->at(node_id);
}

bool view_cell_regular::
contains_visibility_data() const
{
	return visibility_.size() > 0;
}

std::map<model_t, std::vector<node_t>> view_cell_regular::
get_visible_indices() const
{
	std::map<model_t, std::vector<node_t>> indices;

	//for(std::vector<std::vector<bool>>::const_iterator iter = visibility_.begin(); iter != visibility_.end(); ++iter)
	for(model_t model_index = 0; model_index < visibility_.size(); ++model_index)
	{
		const std::vector<bool>& node_visibility = visibility_.at(model_index);

		for(node_t node_index = 0; node_index < node_visibility.size(); ++node_index)
		{
			if(node_visibility.at(node_index))
			{
				indices[model_index].push_back(node_index);
			}
		}
	}

	return indices;
}

}
}
