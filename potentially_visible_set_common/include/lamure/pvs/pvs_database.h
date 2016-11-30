#ifndef LAMURE_PVS_PVS_DATABASE_H
#define LAMURE_PVS_PVS_DATABASE_H

#include <string>
#include <vector>

#include "lamure/pvs/grid.h"

namespace lamure
{
namespace pvs
{

class pvs_database
{
public:
	~pvs_database();
	static pvs_database* get_instance();

	bool load_pvs_from_file(const std::string& grid_file_path, const std::string& pvs_file_path, const std::vector<node_t>& ids);

	virtual void set_viewer_position(const scm::math::vec3d& position);
	virtual bool get_viewer_visibility(const model_t& model_id, const node_t node_id) const;

	void activate(const bool& act);
	bool is_activated() const;

protected:
	pvs_database();

	static pvs_database* instance_;

private:
	grid* visibility_grid_;

	scm::math::vec3d position_viewer_;
	view_cell* viewer_cell_;

	bool activated_;
};

}
}

#endif
