#ifndef LAMURE_PVS_GRID_H
#define LAMURE_PVS_GRID_H

#include <string>

#include "lamure/pvs/view_cell.h"
#include "lamure/types.h"

namespace lamure
{
namespace pvs
{

class grid
{
public:
	virtual ~grid() {}

	virtual unsigned int get_cell_count() const = 0;
	virtual view_cell* get_cell_at_index(const unsigned int& index) = 0;
	virtual view_cell* get_cell_at_position(const scm::math::vec3d& position) = 0;

	virtual void save_grid_to_file(std::string file_path) = 0;
	virtual void save_visibility_to_file(std::string file_path) = 0;

	virtual bool load_grid_from_file(std::string file_path) = 0;
	virtual bool load_visibility_from_file(std::string file_path) = 0;
};

}
}

#endif
