#ifndef LAMURE_PVS_VIEW_CELL_IRREGULAR_H
#define LAMURE_PVS_VIEW_CELL_IRREGULAR_H

#include <lamure/pvs/pvs.h>
#include "lamure/pvs/view_cell_regular.h"

namespace lamure
{
namespace pvs
{

class PVS_COMMON_DLL view_cell_irregular : public view_cell_regular
{
public:
	view_cell_irregular();
	view_cell_irregular(const scm::math::vec3d& cell_size, const scm::math::vec3d& position_center);
	~view_cell_irregular();

	virtual std::string get_cell_type() const;
	static std::string get_cell_identifier();

	virtual scm::math::vec3d get_size() const;

private:
	scm::math::vec3d cell_size_;
};

}
}

#endif
