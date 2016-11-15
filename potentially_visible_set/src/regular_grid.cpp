#include "lamure/pvs/regular_grid.h"

namespace lamure
{
namespace pvs
{

regular_grid::
regular_grid(const int& number_cells, const double& cell_size, const scm::math::vec3d& position_center)
{
	cells_.clear();

	for(int index_z = 0; index_z < number_cells; ++index_z)
	{
		for(int index_y = 0; index_y < number_cells; ++index_y)
		{
			for(int index_x = 0; index_x < number_cells; ++index_x)
			{
				double half_size = (cell_size * (double)number_cells) / 2.0;
				scm::math::vec3d pos = (position_center + scm::math::vec3d(index_x, index_y, index_z) * cell_size) - half_size;
				cells_.push_back(view_cell_regular(cell_size, pos));
			}
		}
	}
}

regular_grid::
~regular_grid()
{
	cells_.clear();
}

unsigned int regular_grid::
get_cell_count() const
{
	return cells_.size();
}

view_cell& regular_grid::
get_cell_at_index(const unsigned int& index)
{
	return cells_.at(index);
}

}
}
