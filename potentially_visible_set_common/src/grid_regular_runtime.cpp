#include "lamure/pvs/grid_regular_runtime.h"

#include <stdexcept>
#include <string>
#include <climits>

namespace lamure
{
namespace pvs
{

grid_regular_runtime::
grid_regular_runtime() : grid_regular_runtime(1, 1.0, scm::math::vec3d(0.0, 0.0, 0.0), std::vector<node_t>())
{
}

grid_regular_runtime::
grid_regular_runtime(const size_t& number_cells, const double& cell_size, const scm::math::vec3d& position_center, const std::vector<node_t>& ids)
{
	file_path_pvs_ = "";
	create_grid(number_cells, cell_size, position_center);

	ids_.resize(ids.size());
	for(size_t index = 0; index < ids_.size(); ++index)
	{
		ids_[index] = ids[index];
	}
}

grid_regular_runtime::
~grid_regular_runtime()
{
	cells_.clear();
	file_in_.close();
}

const view_cell* grid_regular_runtime::
get_cell_at_index(const size_t& index) const
{
	std::lock_guard<std::mutex> lock(mutex_);

	view_cell* current_cell = &cells_.at(index);
	if(current_cell->contains_visibility_data() || file_path_pvs_ == "")
	{
		return current_cell;
	}

	// One line per model.
	node_t single_model_bytes = 0;

	for(model_t model_index = 0; model_index < ids_.size(); ++model_index)
	{
		node_t num_nodes = ids_.at(model_index);

		// If the number of node IDs is not dividable by 8 there is one additional character.
		node_t addition = 0;
		if(num_nodes % CHAR_BIT != 0)
		{
			addition = 1;
		}

		node_t line_size = (num_nodes / CHAR_BIT) + addition;
		single_model_bytes += line_size;
	}

	if(!file_in_.is_open())
	{
		file_in_.open(file_path_pvs_, std::ios::in | std::ios::binary);

		if(!file_in_.is_open())
		{
			throw std::invalid_argument("invalid file path: " + file_path_pvs_);
		}
	}

	file_in_.seekg(index * single_model_bytes);

	// One line per model.
	for(model_t model_index = 0; model_index < ids_.size(); ++model_index)
	{
		node_t num_nodes = ids_.at(model_index);
		size_t line_length = num_nodes / CHAR_BIT + (num_nodes % CHAR_BIT == 0 ? 0 : 1);
		char current_line_data[line_length];

		file_in_.read(current_line_data, line_length);

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

	return current_cell;
}

bool grid_regular_runtime::
load_visibility_from_file(const std::string& file_path)
{
	std::lock_guard<std::mutex> lock(mutex_);

	std::fstream file_in;
	file_in.open(file_path, std::ios::in | std::ios::binary);

	if(!file_in.is_open())
	{
		return false;
	}

	file_in.close();

	file_path_pvs_ = file_path;

	return true;
}

}
}
