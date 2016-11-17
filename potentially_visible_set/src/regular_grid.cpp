#include "lamure/pvs/regular_grid.h"

#include <fstream>
#include <stdexcept>

#include <lamure/ren/model_database.h>

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

view_cell* regular_grid::
get_cell_at_index(const unsigned int& index)
{
	return &cells_.at(index);
}

void regular_grid::
save_to_file(std::string file_path)
{
	std::fstream file_out;
	file_out.open(file_path, std::ios::out /*| std::ios::binary*/);

	if(!file_out.is_open())
	{
		throw std::invalid_argument("invalid file path: " + file_path);
	}

	// Grid file type.
	file_out << "regular" << std::endl;

	// Number of grid cells per dimension.
	file_out << std::pow(cells_.size(), 1.0f/3.0f) << std::endl;

	// Grid size and position
	file_out << cell_size_ << std::endl;
	file_out << position_center_.x << " " << position_center_.y << " " << position_center_.z << std::endl;

	lamure::ren::model_database* database = lamure::ren::model_database::get_instance();

	// Iterate over view cells.
	for(unsigned int cell_index = 0; cell_index < cells_.size(); ++cell_index)
	{
		// Iterate over models in the scene.
		for(lamure::model_t model_id = 0; model_id < database->num_models(); ++model_id)
		{
			char current_byte = 0x00;

			// Iterate over nodes in the model.
			for(lamure::node_t node_id = 0; node_id < database->get_model(model_id)->get_bvh()->get_num_nodes(); ++node_id)
			{
				if(node_id % 8 == 0)
				{
					current_byte = 0x00;
				}

				if(cells_.at(cell_index).get_visibility(model_id, node_id))
				{
					current_byte |= 1 << (7 - (node_id % 8));
				}

				if((node_id + 1) % 8 == 0 || node_id == database->get_model(model_id)->get_bvh()->get_num_nodes() - 1)
				{
					file_out << current_byte;
				}
			}

			file_out << std::endl;
		}
		file_out << std::endl;
	}

	file_out.close();
}

bool regular_grid::
load_from_file(std::string file_path)
{
	return false;
}

}
}
