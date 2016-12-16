#ifndef LAMURE_PVS_REGULAR_GRID_RUNTIME_H
#define LAMURE_PVS_REGULAR_GRID_RUNTIME_H

#include <vector>
#include <string>
#include <fstream>
#include <mutex>

#include "lamure/pvs/grid_regular.h"

namespace lamure
{
namespace pvs
{

class grid_regular_runtime : public grid_regular
{
public:
	grid_regular_runtime();
	grid_regular_runtime(const size_t& number_cells, const double& cell_size, const scm::math::vec3d& position_center, const std::vector<node_t>& ids);
	~grid_regular_runtime();

	virtual const view_cell* get_cell_at_index(const size_t& index) const;

	virtual bool load_visibility_from_file(const std::string& file_path);

protected:
	std::string file_path_pvs_;
	mutable std::fstream file_in_;
};

}
}

#endif
