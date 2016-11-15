#include "lamure/pvs/view_cell_regular.h"

namespace lamure
{
namespace pvs
{

view_cell_regular::
view_cell_regular()
{
	cell_size_ = 1.0;
	position_center_ = scm::math::vec3d(0.0, 0.0, 0.0);
}

view_cell_regular::
view_cell_regular(const double& cell_size, const scm::math::vec3d& position_center)
{
	cell_size_ = cell_size;
	position_center_ = position_center;
}

view_cell_regular::
~view_cell_regular()
{
}

scm::math::vec3d view_cell_regular::
get_size() const
{
	return scm::math::vec3d(cell_size_, cell_size_, cell_size_);
}

scm::math::vec3d view_cell_regular::
get_position_center() const
{
	return position_center_;
}

}
}
