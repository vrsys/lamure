#include <iostream>

#include "lamure/pvs/grid_optimizer_irregular.h"
#include "lamure/pvs/grid_irregular.h"

namespace lamure
{
namespace pvs
{

void grid_optimizer_irregular::
optimize_grid(grid* input_grid, const float& equality_threshold)
{
	grid_irregular* irr_grid = (grid_irregular*)input_grid;

	for(long current_cell_index = 0; current_cell_index < irr_grid->get_cell_count(); ++current_cell_index)
	{
		bool is_original_cell = irr_grid->is_cell_at_index_original(current_cell_index);
		const view_cell* current_cell = irr_grid->get_cell_at_index(current_cell_index);

		for(long compare_cell_index = 0; compare_cell_index < irr_grid->get_cell_count(); ++compare_cell_index)
		{
			if(compare_cell_index == current_cell_index)
			{
				continue;
			}

			const view_cell* compare_cell = irr_grid->get_cell_at_index(compare_cell_index);

			// Check equality.
			size_t difference_counter = 0;
			size_t total_nodes = 0;

			for(model_t model_index = 0; model_index < irr_grid->get_num_models(); ++model_index)
			{
				for(node_t node_index = 0; node_index < irr_grid->get_num_nodes(model_index); ++node_index)
				{
					if(current_cell->get_visibility(model_index, node_index) != compare_cell->get_visibility(model_index, node_index))
					{
						++difference_counter;
					}

					++total_nodes;
				}
			}

			float error = (float)difference_counter / (float)total_nodes;
			float equality = 1.0f - error;
			
			// This test will already cancel some unecessary tests, yet a further threshold test will be done within join_cells.
			if(equality >= equality_threshold)
			{
				if(irr_grid->join_cells(current_cell_index, compare_cell_index, error, equality_threshold) && is_original_cell)
				{
					--current_cell_index;
					compare_cell_index = irr_grid->get_cell_count();
				}
			}
		}

		double current_percentage_done = ((double)current_cell_index / (double)irr_grid->get_cell_count()) * 100.0;
		std::cout << "\rgrid optimization in progress [" << current_percentage_done << "]       " << std::flush;
	}

	std::cout << "\rgrid optimization in progress [100]       " << std::endl;
}

}
}