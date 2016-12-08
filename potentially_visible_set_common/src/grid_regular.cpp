#include "lamure/pvs/grid_regular.h"

#include <fstream>
#include <stdexcept>
#include <string>
#include <climits>

namespace lamure
{
namespace pvs
{

grid_regular::
grid_regular() : grid_regular(1, 1.0, scm::math::vec3d(0.0, 0.0, 0.0))
{
}

grid_regular::
grid_regular(const size_t& number_cells, const double& cell_size, const scm::math::vec3d& position_center)
{
	create_grid(number_cells, cell_size, position_center);
}

grid_regular::
~grid_regular()
{
	cells_.clear();
}

size_t grid_regular::
get_cell_count() const
{
	return cells_.size();
}

view_cell* grid_regular::
get_cell_at_index(const size_t& index)
{
	std::lock_guard<std::mutex> lock(mutex_);

	return &cells_.at(index);
}

const view_cell* grid_regular::
get_cell_at_index_const(const size_t& index) const
{
	std::lock_guard<std::mutex> lock(mutex_);

	return  &cells_.at(index);
}

view_cell* grid_regular::
get_cell_at_position(const scm::math::vec3d& position)
{
	size_t general_index = 0;

	{
		std::lock_guard<std::mutex> lock(mutex_);

		size_t num_cells = std::pow(cells_.size(), 1.0f/3.0f);
		double half_size = cell_size_ * (double)num_cells * 0.5f;
		scm::math::vec3d distance = position - (position_center_ - half_size);

		size_t index_x = (size_t)(distance.x / cell_size_);
		size_t index_y = (size_t)(distance.y / cell_size_);
		size_t index_z = (size_t)(distance.z / cell_size_);

		// Check calculated index so we know if the position is inside the grid at all.
		if(index_x < 0 || index_x >= num_cells ||
			index_y < 0 || index_y >= num_cells ||
			index_z < 0 || index_z >= num_cells)
		{
			return nullptr;
		}

		general_index = (num_cells * num_cells * index_z) + (num_cells * index_y) + index_x;
	}

	return get_cell_at_index(general_index);
}

const view_cell* grid_regular::
get_cell_at_position_const(const scm::math::vec3d& position)
{
	return get_cell_at_position(position);
}

void grid_regular::
save_grid_to_file(const std::string& file_path, const std::vector<node_t>& ids) const
{
	std::lock_guard<std::mutex> lock(mutex_);

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

	// Save number of models, so we can later simply read the node numbers.
	file_out << ids.size() << std::endl;

	// Save the number of node ids of each model.
	for(size_t model_index = 0; model_index < ids.size(); ++model_index)
	{
		file_out << ids[model_index] << " ";
	}

	file_out.close();
}

void grid_regular::
save_visibility_to_file(const std::string& file_path, const std::vector<node_t>& ids) const
{
	std::lock_guard<std::mutex> lock(mutex_);

	std::fstream file_out;
	file_out.open(file_path, std::ios::out | std::ios::binary);

	if(!file_out.is_open())
	{
		throw std::invalid_argument("invalid file path: " + file_path);
	}

	// Iterate over view cells.
	for(size_t cell_index = 0; cell_index < cells_.size(); ++cell_index)
	{
		std::string current_cell_data = "";

		// Iterate over models in the scene.
		for(lamure::model_t model_id = 0; model_id < ids.size(); ++model_id)
		{
			node_t num_nodes = ids.at(model_id);
			char current_byte = 0x00;

			size_t line_length = num_nodes / CHAR_BIT + (num_nodes % CHAR_BIT == 0 ? 0 : 1);
			size_t character_counter = 0;
			std::string current_line_data(line_length, 0x00);

			node_t invisible_counter = 0;
			node_t node_counter = 0;

			// Iterate over nodes in the model.
			for(lamure::node_t node_id = 0; node_id < num_nodes; ++node_id)
			{
				if(cells_.at(cell_index).get_visibility(model_id, node_id))
				{
					current_byte |= 1 << (node_id % CHAR_BIT);
				}
				else
					invisible_counter++;

				node_counter++;

				// Flush character if either 8 bits are written or if the node id is the last one.
				if((node_id + 1) % CHAR_BIT == 0 || node_id == (num_nodes - 1))
				{
					current_line_data[character_counter] = current_byte;
					character_counter++;

					current_byte = 0x00;
				}
			}

			current_cell_data = current_cell_data + current_line_data;
		}

		file_out.write(current_cell_data.c_str(), current_cell_data.length());
	}

	file_out.close();
}

bool grid_regular::
load_grid_from_file(const std::string& file_path)
{
	std::lock_guard<std::mutex> lock(mutex_);

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

	size_t num_cells;
	file_in >> num_cells;

	double cell_size;
	file_in >> cell_size;

	double pos_x, pos_y, pos_z;
	file_in >> pos_x >> pos_y >> pos_z;

	create_grid(num_cells, cell_size, scm::math::vec3d(pos_x, pos_y, pos_z));

	// Read the number of models.
	size_t num_models = 0;
	file_in >> num_models;
	ids_.resize(num_models);

	// Read the number of nodes per model.
	for(size_t model_index = 0; model_index < ids_.size(); ++model_index)
	{
		node_t num_nodes = 0;
		file_in >> num_nodes;
		ids_[model_index] = num_nodes;
	}

	file_in.close();
	return true;
}


bool grid_regular::
load_visibility_from_file(const std::string& file_path)
{
	std::lock_guard<std::mutex> lock(mutex_);

	std::fstream file_in;
	file_in.open(file_path, std::ios::in | std::ios::binary);

	if(!file_in.is_open())
	{
		return false;
	}

	for(size_t cell_index = 0; cell_index < cells_.size(); ++cell_index)
	{
		view_cell* current_cell = &cells_.at(cell_index);
		
		// One line per model.
		for(model_t model_index = 0; model_index < ids_.size(); ++model_index)
		{
			node_t num_nodes = ids_.at(model_index);
			size_t line_length = num_nodes / CHAR_BIT + (num_nodes % CHAR_BIT == 0 ? 0 : 1);
			char current_line_data[line_length];

			file_in.read(current_line_data, line_length);

			// Used to avoid continuing resize within visibility data.
			current_cell->set_visibility(model_index, num_nodes - 1, false);

			for(node_t character_index = 0; character_index < line_length; ++character_index)
			{
				char current_byte = current_line_data[character_index];
				
				for(unsigned short bit_index = 0; bit_index < CHAR_BIT; ++bit_index)
				{
					bool visible = ((current_byte >> bit_index) & 1) == 0x01;
					current_cell->set_visibility(model_index, (character_index * CHAR_BIT) + bit_index, visible);
				}
			}
		}
	}

	file_in.close();
	return true;
}

void grid_regular::
create_grid(const size_t& num_cells, const double& cell_size, const scm::math::vec3d& position_center)
{
	cells_.clear();

	double half_size = (cell_size * (double)num_cells) * 0.5;		// position of grid is at grid center, so cells have a offset
	double cell_offset = cell_size * 0.5f;							// position of cell is at cell center

	for(size_t index_z = 0; index_z < num_cells; ++index_z)
	{
		for(size_t index_y = 0; index_y < num_cells; ++index_y)
		{
			for(size_t index_x = 0; index_x < num_cells; ++index_x)
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
