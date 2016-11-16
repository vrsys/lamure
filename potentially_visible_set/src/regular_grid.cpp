#include "lamure/pvs/regular_grid.h"

#include <fstream>
#include <stdexcept>

#include <iostream>

namespace lamure
{
namespace pvs
{

regular_grid::
regular_grid(const int& number_cells, const double& cell_size, const scm::math::vec3d& position_center)
{
	cells_.clear();

	cell_size_ = cell_size;
	position_center_ = scm::math::vec3d(position_center);

	double half_size = (cell_size * (double)number_cells) * 0.5;	// position of grid is at grid center, so cells have a offset
	double cell_offset = cell_size_ * 0.5f;							// position of cell is at cell center

	for(int index_z = 0; index_z < number_cells; ++index_z)
	{
		for(int index_y = 0; index_y < number_cells; ++index_y)
		{
			for(int index_x = 0; index_x < number_cells; ++index_x)
			{
				scm::math::vec3d pos = position_center + (scm::math::vec3d(index_x , index_y, index_z) * cell_size) - half_size + cell_offset;
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

void regular_grid::
save_to_file(std::string file_path)
{
	std::fstream file_out;
	file_out.open(file_path, std::ios::out);

	std::cout << "called 1" << std::endl;

	if(!file_out.is_open())
	{
		throw std::invalid_argument("invalid file path: " + file_path);
	}

	// Grid file type.
	file_out << "regular" << std::endl;

	// Grid size and position
	file_out << cell_size_ << std::endl;
	file_out << position_center_.x << position_center_.y << position_center_.z << std::endl;

	file_out.close();

	std::cout << "called 2" << std::endl;
}

bool regular_grid::
load_from_file(std::string file_path)
{
	return false;
}

}
}
