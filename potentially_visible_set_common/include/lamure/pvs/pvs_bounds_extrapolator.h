#ifndef LAMURE_PVS_BOUNDS_EXTRAPOLATOR_H
#define LAMURE_PVS_BOUNDS_EXTRAPOLATOR_H

#include "lamure/pvs/grid.h"
#include "lamure/pvs/grid_bounding.h"

namespace lamure
{
namespace pvs
{

class pvs_bounds_extrapolator
{
public:
	virtual ~pvs_bounds_extrapolator() {}

	virtual grid_bounding* extrapolate_from_grid(const grid* input_grid) const = 0;
};

}
}

#endif
