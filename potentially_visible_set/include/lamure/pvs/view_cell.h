#ifndef LAMURE_PVS_VIEW_CELL_H
#define LAMURE_PVS_VIEW_CELL_H

#include <scm/core/math.h>

namespace lamure
{
namespace pvs
{

class view_cell
{
public:
	virtual ~view_cell() {}

	virtual scm::math::vec3d get_size() const = 0;
	virtual scm::math::vec3d get_position_center() const = 0;
};

}
}

#endif
