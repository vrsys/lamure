#include "lamure/pvs/regular_grid.h"

#include <fstream>
#include <stdexcept>
#include <string>
#include <climits>

namespace lamure
{
namespace pvs
{

regular_grid::
regular_grid() : regular_grid(1, 1.0, scm::math::vec3d(0.0, 0.0, 0.0))
{
}

regular_grid::
regular_grid(const unsigned int& number_cells, const double& cell_size, const scm::math::vec3d& position_center)
{
	create_grid(number_cells, cell_size, position_center);
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

view_cell* regular_grid::
get_cell_at_position(const scm::math::vec3d& position)
{
	int num_cells = std::pow(cells_.size(), 1.0f/3.0f);
	double half_size = cell_size_ * (double)num_cells * 0.5f;
	scm::math::vec3d distance = position - (position_center_ - half_size);

	int index_x = (int)(distance.x / cell_size_);
	int index_y = (int)(distance.y / cell_size_);
	int index_z = (int)(distance.z / cell_size_);

	// Check calculated index so we know if the position is inside the grid at all.
	if(index_x < 0 || index_x >= num_cells ||
		index_y < 0 || index_y >= num_cells ||
		index_z < 0 || index_z >= num_cells)
	{
		return nullptr;
	}

	int general_index = (num_cells * num_cells * index_z) + (num_cells * index_y) + index_x;
	return get_cell_at_index(general_index);
}

void regular_grid::
save_grid_to_file(const std::string& file_path) const
{
	std::fstream file_out;
	file_out.open(file_path, std::ios::out);

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

	file_out.close();
}

void regular_grid::
save_visibility_to_file(const std::string& file_path, const std::vector<unsigned int>& ids) const
{
	std::fstream file_out;
	file_out.open(file_path, std::ios::out | std::ios::binary);

	if(!file_out.is_open())
	{
		throw std::invalid_argument("invalid file path: " + file_path);
	}

	// Iterate over view cells.
	for(unsigned int cell_index = 0; cell_index < cells_.size(); ++cell_index)
	{
		// Iterate over models in the scene.
		for(lamure::model_t model_id = 0; model_id < ids.size(); ++model_id)
		{
			unsigned int num_nodes = ids.at(model_id);
			char current_byte = 0x00;

			// Iterate over nodes in the model.
			for(lamure::node_t node_id = 0; node_id < num_nodes; ++node_id)
			{
				if(cells_.at(cell_index).get_visibility(model_id, node_id))
				{
					current_byte |= 1 << (node_id % CHAR_BIT);
				}

				// Flush character if either 8 bits are written or if the node id is the last one.
				if((node_id + 1) % CHAR_BIT == 0 || node_id == (num_nodes - 1))
				{
					file_out.write(&current_byte, 1);
					current_byte = 0x00;
				}
			}

			// Does not save bitwise, but bytewise. (Only here because I do not 100% believe in the bitwise code.)
			/*for(lamure::node_t node_id = 0; node_id < num_nodes; ++node_id)
			{
				file_out << (cells_.at(cell_index).get_visibility(model_id, node_id) == true ? 1 : 0);
			}*/

			// Exactly one model per line.
			//file_out << std::endl;
		}
	}

	file_out.close();
}

bool regular_grid::
load_grid_from_file(const std::string& file_path)
{
	std::fstream file_in;
	file_in.open(file_path, std::ios::in);

	if(!file_in.is_open())
	{
		return false;
	}

	// Start reading the header info which is used to recreate the grid.
	std::string grid_type;
	file_in >> grid_type;
	if(grid_type != "regular")
	{
		return false;
	}

	unsigned int num_cells;
	file_in >> num_cells;

	double cell_size;
	file_in >> cell_size;

	double pos_x, pos_y, pos_z;
	file_in >> pos_x >> pos_y >> pos_z;

	create_grid(num_cells, cell_size, scm::math::vec3d(pos_x, pos_y, pos_z));

	file_in.close();
	return true;
}


bool regular_grid::
load_visibility_from_file(const std::string& file_path, const std::vector<unsigned int>& ids)
{
	std::fstream file_in;
	file_in.open(file_path, std::ios::in | std::ios::binary);

	if(!file_in.is_open())
	{
		return false;
	}

	for(unsigned int cell_index = 0; cell_index < cells_.size(); ++cell_index)
	{
		view_cell* current_cell = &cells_.at(cell_index);
		
		// One line per model.
		for(model_t model_index = 0; model_index < ids.size(); ++model_index)
		{
			unsigned int num_nodes = ids.at(model_index);

			// If the number of node IDs is not dividable by 8 there is one additional character.
			unsigned int addition = 0;
			if(num_nodes % CHAR_BIT != 0)
			{
				addition = 1;
			}

			unsigned int line_size = (num_nodes / CHAR_BIT) + addition;

			for(unsigned int character_index = 0; character_index < line_size; ++character_index)
			{
				char current_byte = 0x00;
				file_in.read(&current_byte, 1);
				
				for(unsigned int bit_index = 0; bit_index < CHAR_BIT; ++bit_index)
				{
					bool visible = ((current_byte >> bit_index) & 1) == 0x01;
					current_cell->set_visibility(model_index, (character_index * CHAR_BIT) + bit_index, visible);
				}
			}

			// Does not load bitwise, but bytewise. (Only here because I do not 100% believe in the bitwise code.)
			/*for(unsigned int node_index = 0; node_index < num_nodes; ++node_index)
			{
				char input;
				file_in >> input;
				current_cell->set_visibility(model_index, node_index, input == '1');
			}*/
		}
	}

	file_in.close();
	return true;
}

void regular_grid::
create_grid(const unsigned int& num_cells, const double& cell_size, const scm::math::vec3d& position_center)
{
	cells_.clear();

	double half_size = (cell_size * (double)num_cells) * 0.5;		// position of grid is at grid center, so cells have a offset
	double cell_offset = cell_size * 0.5f;							// position of cell is at cell center

	for(unsigned int index_z = 0; index_z < num_cells; ++index_z)
	{
		for(unsigned int index_y = 0; index_y < num_cells; ++index_y)
		{
			for(unsigned int index_x = 0; index_x < num_cells; ++index_x)
			{
				scm::math::vec3d pos = position_center + (scm::math::vec3d(index_x , index_y, index_z) * cell_size) - half_size + cell_offset;
				cells_.push_back(view_cell_regular(cell_size, pos));
			}
		}
	}

	cell_size_ = cell_size;
	position_center_ = scm::math::vec3d(position_center);
}

}
}
