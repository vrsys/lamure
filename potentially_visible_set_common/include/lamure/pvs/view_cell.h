#ifndef LAMURE_PVS_VIEW_CELL_H
#define LAMURE_PVS_VIEW_CELL_H

#include <map>
#include <vector>
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

	virtual void set_visibility(const unsigned int& object_id, const unsigned int& node_id, const bool& visible) = 0;
	virtual bool get_visibility(const unsigned int& object_id, const unsigned int& node_id) const = 0;
	virtual std::map<unsigned int, std::vector<unsigned int>> get_visible_indices() const = 0;
};

}
}

#endif
