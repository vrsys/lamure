#include "lamure/pvs/grid_bounding.h"
#include "lamure/pvs/view_cell_irregular.h"

#include <climits>

namespace lamure
{
namespace pvs
{

grid_bounding::
grid_bounding() : grid_bounding(nullptr, std::vector<node_t>())
{
}

grid_bounding::
grid_bounding(const grid* core_grid, const std::vector<node_t>& ids)
{
	this->create_grid(core_grid);

	ids_.resize(ids.size());
	for(size_t index = 0; index < ids_.size(); ++index)
	{
		ids_[index] = ids[index];
	}
}

grid_bounding::
~grid_bounding()
{
}

std::string grid_bounding::
get_grid_type() const
{
	return get_grid_identifier();
}

std::string grid_bounding::
get_grid_identifier()
{
	return "bounding";
}

const view_cell* grid_bounding::
get_cell_at_position(const scm::math::vec3d& position, size_t* cell_index) const
{
	size_t general_index = 0;

	{
		std::lock_guard<std::mutex> lock(mutex_);

		// This cats is required because the parent class grid_regular stores a vector of view_cell_regular (no pointer, so no polymorphism applied).
		view_cell* center_cell = cells_[13];

		int num_cells = 3;
		scm::math::vec3d cell_size = center_cell->get_size() * 0.5;
		scm::math::vec3d distance = position - position_center_;

		int index_x = (int)(distance.x / cell_size.x);
		int index_y = (int)(distance.y / cell_size.y);
		int index_z = (int)(distance.z / cell_size.z);

		// Normalize to value -1 or 1 if not 0.
		if(index_x != 0)
		{
			index_x = index_x / std::abs(index_x);
		}
		if(index_y != 0)
		{
			index_y = index_y / std::abs(index_y);
		}
		if(index_z != 0)
		{
			index_z = index_z / std::abs(index_z);
		}

		++index_x;
		++index_y;
		++index_z;

		general_index = (size_t)((num_cells * num_cells * index_z) + (num_cells * index_y) + index_x);
	}

	// Optional second return value: the index of the view cell.
	if(cell_index != nullptr)
	{
		(*cell_index) = general_index;
	}

	return get_cell_at_index(general_index);
}

/*void grid_regular_compressed::
save_grid_to_file(const std::string& file_path) const
{
	save_regular_grid(file_path, get_grid_identifier());
}*/


/*bool grid_regular_compressed::
load_grid_from_file(const std::string& file_path)
{
	return load_regular_grid(file_path, get_grid_identifier());
}*/

void grid_bounding::
create_grid(const grid* core_grid)
{
	for(size_t cell_index = 0; cell_index < cells_.size(); ++cell_index)
	{
		delete cells_[cell_index];
	}

	cells_.clear();
	cells_.resize(27);

	size_t cell_index = 0;
	scm::math::vec3d cell_size = core_grid->get_size();	// TODO: requires irregular view cell to work for irregular grids

	for(int z_dir = -1; z_dir <= 1; ++z_dir)
	{
		for(int y_dir = -1; y_dir <= 1; ++y_dir)
		{
			for(int x_dir = -1; x_dir <= 1; ++x_dir)
			{
				scm::math::vec3d position_center = core_grid->get_position_center() + core_grid->get_size() * scm::math::vec3d(x_dir, y_dir, z_dir);
				cells_[cell_index] = new view_cell_irregular(cell_size, position_center);

				++cell_index;
			}
		}
	}

	size_ = cell_size * 3.0;
	position_center_ = scm::math::vec3d(core_grid->get_position_center());
}

}
}
