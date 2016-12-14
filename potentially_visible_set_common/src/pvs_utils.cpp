#include <fstream>

#include "lamure/pvs/pvs_utils.h"
#include "lamure/types.h"

namespace lamure
{
namespace pvs
{

void analyze_grid_visibility(const grid* input_grid, const unsigned int& num_steps, const std::string& output_file_name)
{
	std::ofstream file_out;
    file_out.open(output_file_name);

	std::vector<size_t> occlusion_percent_interval_counter;
    occlusion_percent_interval_counter.resize(num_steps);

    // Iterate over view cells and collect visibility data.
    size_t num_cells = input_grid->get_cell_count();
    lamure::node_t total_num_nodes = 0;
    lamure::node_t total_visible_nodes = 0;

    for(size_t cell_index = 0; cell_index < num_cells; ++cell_index)
    {
        lamure::model_t num_models = input_grid->get_num_models();
        std::map<lamure::model_t, std::vector<lamure::node_t>> visibility = input_grid->get_cell_at_index_const(cell_index)->get_visible_indices();
        
        lamure::node_t model_num_nodes = 0;
        lamure::node_t model_visible_nodes = 0;

        for(lamure::model_t model_index = 0; model_index < num_models; ++model_index)
        {
            lamure::node_t num_nodes = input_grid->get_num_nodes(model_index);

            model_num_nodes += num_nodes;
            model_visible_nodes += visibility[model_index].size();
        }

        float occlusion = (1.0f - (float)model_visible_nodes / (float)model_num_nodes) * 100.0f;
        
        float interval_size = 100.0f / (float)num_steps;
        int occlusion_index = std::round((occlusion - interval_size * 0.5f) / interval_size);
        occlusion_index = std::max(0, std::min(occlusion_index, (int)(num_steps - 1)));

        occlusion_percent_interval_counter[occlusion_index] += 1;

        file_out << "cell: " << cell_index << " occlusion: " << occlusion << "   " << model_visible_nodes << "/" << model_num_nodes << std::endl;

        total_num_nodes += model_num_nodes;
        total_visible_nodes += model_visible_nodes;
    }

    float occlusion = (1.0f - (float)total_visible_nodes / (float)total_num_nodes) * 100.0f;

    // Output resulting data.
    file_out << "\ntotal: " << " occlusion: " << occlusion << "   " << total_visible_nodes << "/" << total_num_nodes << std::endl << std::endl;
    
    for(size_t occlusion_index = 0; occlusion_index < occlusion_percent_interval_counter.size(); ++occlusion_index)
    {
        float occlusion_local = (float)occlusion_percent_interval_counter[occlusion_index] / (float)num_cells;
        file_out << "occlusion interval " << occlusion_index << ": " << occlusion_local << "   (" << occlusion_percent_interval_counter[occlusion_index] << "/" << num_cells << ")" << std::endl;
    }

    file_out.close();
}

}
}