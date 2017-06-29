#ifndef LAMURE_PVS_IRREGULAR_COMPRESSED_GRID_H
#define LAMURE_PVS_IRREGULAR_COMPRESSED_GRID_H

#include <lamure/pvs/pvs.h>
#include "lamure/pvs/grid_irregular.h"

namespace lamure
{
namespace pvs
{

class PVS_COMMON_DLL grid_irregular_compressed : public grid_irregular
{
public:
	grid_irregular_compressed();
	grid_irregular_compressed(const size_t& number_cells_x, const size_t& number_cells_y, const size_t& number_cells_z, const double& cell_size, const scm::math::vec3d& position_center, const std::vector<node_t>& ids);
	~grid_irregular_compressed();

	virtual std::string get_grid_type() const;
	static std::string get_grid_identifier();

	virtual void save_grid_to_file(const std::string& file_path) const;
	virtual void save_visibility_to_file(const std::string& file_path) const;

	virtual bool load_grid_from_file(const std::string& file_path);
	virtual bool load_visibility_from_file(const std::string& file_path);

	virtual bool load_cell_visibility_from_file(const std::string& file_path, const size_t& cell_index);

protected:
	std::vector<uint64_t> visibility_block_sizes_;
};

}
}

#endif
