#ifndef LAMURE_PVS_VIEW_CELL_REGULAR_H
#define LAMURE_PVS_VIEW_CELL_REGULAR_H

#include <vector>
#include <map>

#include <scm/core/math.h>

#include "lamure/pvs/view_cell.h"

namespace lamure
{
namespace pvs
{

class view_cell_regular : public view_cell
{
public:
	view_cell_regular();
	view_cell_regular(const double& cell_size, const scm::math::vec3d& position_center);
	~view_cell_regular();

	virtual scm::math::vec3d get_size() const;
	virtual scm::math::vec3d get_position_center() const;

	virtual void set_visibility(const unsigned int& object_id, const unsigned int& node_id, const bool& visible);
	virtual bool get_visibility(const unsigned int& object_id, const unsigned int& node_id) const;
	virtual std::map<unsigned int, std::vector<unsigned int>> get_visible_indices() const;

private:
	double cell_size_;
	scm::math::vec3d position_center_;

	std::vector<std::vector<bool>> visibility_;
};

}
}

#endif
