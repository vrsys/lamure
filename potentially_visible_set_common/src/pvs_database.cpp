#include "lamure/pvs/pvs_database.h"
#include "lamure/pvs/grid_regular.h"
#include "lamure/pvs/grid_regular_runtime.h"

namespace lamure
{
namespace pvs
{

pvs_database* pvs_database::instance_ = nullptr;

pvs_database::
pvs_database()
{
	visibility_grid_ = nullptr;
	viewer_cell_ = nullptr;
	activated_ = true;
}

pvs_database::
~pvs_database()
{
	if(visibility_grid_ != nullptr)
	{
		delete visibility_grid_;
	}
}

pvs_database* pvs_database::
get_instance()
{
	if(pvs_database::instance_ == nullptr)
	{
		pvs_database::instance_ = new pvs_database();
	}

	return pvs_database::instance_;
}

bool pvs_database::
load_pvs_from_file(const std::string& grid_file_path, const std::string& pvs_file_path)
{
	std::lock_guard<std::mutex> lock(mutex_);

	// TODO: currently there is only one grid type, but later on the created grid should depend on the type noted in the grid file.
	visibility_grid_ = new grid_regular_runtime();
	bool result = visibility_grid_->load_grid_from_file(grid_file_path);

	if(!result)
	{
		// Loading grid file failed.
		delete visibility_grid_;
		visibility_grid_ = nullptr;

		return false;
	}
	
	result = visibility_grid_->load_visibility_from_file(pvs_file_path);

	if(!result)
	{
		// Loading grid file failed.
		delete visibility_grid_;
		visibility_grid_ = nullptr;

		return false;
	}

	return true;
}

void pvs_database::
set_viewer_position(const scm::math::vec3d& position)
{
	std::lock_guard<std::mutex> lock(mutex_);

	if(activated_ && visibility_grid_ != nullptr)
	{
		if(position != position_viewer_)
		{
			position_viewer_ = position;
			viewer_cell_ = visibility_grid_->get_cell_at_position(position);
		}
	}
}

bool pvs_database::
get_viewer_visibility(const model_t& model_id, const node_t node_id) const
{
	std::lock_guard<std::mutex> lock(mutex_);

	if(!activated_ || viewer_cell_ == nullptr)
	{
		return true;
	}
	else
	{
		return viewer_cell_->get_visibility(model_id, node_id);
	}
}

void pvs_database::
activate(const bool& act)
{
	std::lock_guard<std::mutex> lock(mutex_);

	activated_ = act;
}

bool pvs_database::
is_activated() const
{
	std::lock_guard<std::mutex> lock(mutex_);

	return activated_;
}

const grid* pvs_database::
get_visibility_grid() const
{
	std::lock_guard<std::mutex> lock(mutex_);

	return visibility_grid_;
}

}
}
