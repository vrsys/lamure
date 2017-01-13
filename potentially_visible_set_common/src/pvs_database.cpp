#include <fstream>
#include <string>

#include "lamure/pvs/pvs_database.h"
#include "lamure/pvs/grid_regular.h"
#include "lamure/pvs/grid_regular_runtime.h"
#include "lamure/pvs/grid_octree.h"
#include "lamure/pvs/grid_octree_hierarchical.h"

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
	runtime_ = true;
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

	std::fstream file_in;
	file_in.open(grid_file_path, std::ios::in);

	if(!file_in.is_open())
	{
		return false;
	}

	// Read the grid file header to identify the grid type.
	std::string grid_type;
	file_in >> grid_type;

	if(grid_type == "regular")
	{
		if(runtime_)
		{
			visibility_grid_ = new grid_regular_runtime();
		}
		else
		{
			visibility_grid_ = new grid_regular();
		}
	}
	else if(grid_type == "octree")
	{
		visibility_grid_ = new grid_octree();
	}
	else if(grid_type == "octree_hierarchical")
	{
		visibility_grid_ = new grid_octree_hierarchical();
	}

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

void pvs_database::
runtime_mode(const bool& is_runtime)
{
	runtime_ = is_runtime;
}

const grid* pvs_database::
get_visibility_grid() const
{
	std::lock_guard<std::mutex> lock(mutex_);

	return visibility_grid_;
}

void pvs_database::
clear_visibility_grid()
{
	std::lock_guard<std::mutex> lock(mutex_);

	viewer_cell_ = nullptr;

	delete visibility_grid_;
	visibility_grid_ = nullptr;
}

}
}
