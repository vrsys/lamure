#ifndef LAMURE_PVS_GRID_H
#define LAMURE_PVS_GRID_H

#include "lamure/pvs/view_cell.h"

namespace lamure
{
namespace pvs
{

class grid
{
public:
	virtual ~grid() {}

	virtual unsigned int get_cell_count() const = 0;
	virtual view_cell& get_cell_at_index(const unsigned int& index) = 0;
};

}
}

#endif
