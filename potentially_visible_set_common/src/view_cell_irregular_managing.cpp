#include "lamure/pvs/view_cell_irregular_managing.h"
#include "lamure/bounding_box.h"

namespace lamure
{
namespace pvs
{

view_cell_irregular_managing::
view_cell_irregular_managing() : view_cell_irregular_managing(0.0, scm::math::vec3d(0.0, 0.0, 0.0))
{
}

view_cell_irregular_managing::
view_cell_irregular_managing(const double& cell_size, const scm::math::vec3d& position_center) : view_cell_regular(cell_size, position_center)
{
}

view_cell_irregular_managing::
~view_cell_irregular_managing()
{
}

scm::math::vec3d view_cell_irregular_managing::
get_size() const
{
	const view_cell* cell = managed_view_cells_[0];
	bounding_box bb(cell->get_position_center() - cell->get_size() * 0.5, cell->get_position_center() + cell->get_size() * 0.5);

	for(size_t original_cell_index = 1; original_cell_index < managed_view_cells_.size(); ++original_cell_index)
	{
		cell = managed_view_cells_[original_cell_index];
		bounding_box vc_bb(cell->get_position_center() - cell->get_size() * 0.5, cell->get_position_center() + cell->get_size() * 0.5);
		bb.expand(vc_bb);
	}

	return bb.get_dimensions();
}

scm::math::vec3d view_cell_irregular_managing::
get_position_center() const
{
	const view_cell* cell = managed_view_cells_[0];
	bounding_box bb(cell->get_position_center() - cell->get_size() * 0.5, cell->get_position_center() + cell->get_size() * 0.5);

	for(size_t original_cell_index = 1; original_cell_index < managed_view_cells_.size(); ++original_cell_index)
	{
		cell = managed_view_cells_[original_cell_index];
		bounding_box vc_bb(cell->get_position_center() - cell->get_size() * 0.5, cell->get_position_center() + cell->get_size() * 0.5);
		bb.expand(vc_bb);
	}

	return bb.get_center();
}

void view_cell_irregular_managing::
add_cell(const view_cell* cell)
{
	managed_view_cells_.push_back(cell);
}

void view_cell_irregular_managing::
set_error(const float& error)
{
	current_error_ = error;
}

float view_cell_irregular_managing::
get_error() const
{
	return current_error_;
}

}
}
