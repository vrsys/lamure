#include <fstream>
#include <deque>

#include "lamure/bounding_box.h"
#include "lamure/pvs/grid_octree_hierarchical.h"

namespace lamure
{
namespace pvs
{

grid_octree_hierarchical::
grid_octree_hierarchical() : grid_octree_hierarchical(1, 1.0, scm::math::vec3d(0.0, 0.0, 0.0), std::vector<node_t>())
{
}

grid_octree_hierarchical::
grid_octree_hierarchical(const size_t& octree_depth, const double& size, const scm::math::vec3d& position_center, const std::vector<node_t>& ids)
{
	root_node_ = new grid_octree_hierarchical_node(size, position_center);
	
	create_grid(root_node_, octree_depth);
	
	compute_index_access();

	ids_.resize(ids.size());
	for(size_t index = 0; index < ids_.size(); ++index)
	{
		ids_[index] = ids[index];
	}
}

grid_octree_hierarchical::
~grid_octree_hierarchical()
{
	if(root_node_ != nullptr)
	{
		delete root_node_;
	}
}

void grid_octree_hierarchical::
save_grid_to_file(const std::string& file_path) const
{
	std::lock_guard<std::mutex> lock(mutex_);

	std::fstream file_out;
	file_out.open(file_path, std::ios::out);

	if(!file_out.is_open())
	{
		throw std::invalid_argument("invalid file path: " + file_path);
	}

	// Grid file type.
	file_out << "octree_hierarchical" << std::endl;

	// Root size and position
	file_out << root_node_->get_size().x << std::endl;
	file_out << root_node_->get_position_center().x << " " << root_node_->get_position_center().y << " " << root_node_->get_position_center().z << std::endl;

	// Write octree structure by writing split commands (0 = don't split, 1 = split).
	std::deque<const grid_octree_node*> unwritten_nodes;
	unwritten_nodes.push_back(root_node_);

	while(unwritten_nodes.size() != 0)
	{
		const grid_octree_node* top_node = unwritten_nodes[0];
		unwritten_nodes.pop_front();
		
		if(top_node->has_children())
		{
			file_out << '1';
			
			for(size_t child_index = 0; child_index < 8; ++child_index)
			{
				unwritten_nodes.push_back(top_node->get_child_at_index_const(child_index));
			}
		}
		else
		{
			file_out << '0';
		}
	}
	file_out << std::endl;

	// Save number of models, so we can later simply read the node numbers.
	file_out << ids_.size() << std::endl;

	// Save the number of node ids of each model.
	for(size_t model_index = 0; model_index < ids_.size(); ++model_index)
	{
		file_out << ids_[model_index] << " ";
	}

	file_out.close();
}

void grid_octree_hierarchical::
save_visibility_to_file(const std::string& file_path) const
{
	std::lock_guard<std::mutex> lock(mutex_);

	std::fstream file_out;
	file_out.open(file_path, std::ios::out | std::ios::binary);

	if(!file_out.is_open())
	{
		throw std::invalid_argument("invalid file path: " + file_path);
	}

	// Iterate over view cells.
	std::deque<const grid_octree_node*> unwritten_nodes;
	unwritten_nodes.push_back(root_node_);

	while(unwritten_nodes.size() != 0)
	{
		const grid_octree_hierarchical_node* current_node = (const grid_octree_hierarchical_node*)unwritten_nodes[0];
		unwritten_nodes.pop_front();

		std::map<model_t, std::vector<node_t>> visible_indices = current_node->get_visible_indices();

		for(model_t model_index = 0; model_index < ids_.size(); ++model_index)
		{
			const std::vector<node_t>& model_visibility = visible_indices[model_index];

			// Write number of values which will be written so it can be read later.
			node_t number_visibility_elements = model_visibility.size();
			file_out.write(reinterpret_cast<char*>(&number_visibility_elements), sizeof(number_visibility_elements));

			// Write visibility data.
			for(node_t node_index = 0; node_index < number_visibility_elements; ++node_index)
			{
				node_t visibility_node = model_visibility[node_index];
				file_out.write(reinterpret_cast<char*>(&visibility_node), sizeof(visibility_node));
			}
		}

		// Add child nodes of current nodes to queue.
		if(current_node->has_children())
		{
			for(size_t child_index = 0; child_index < 8; ++child_index)
			{
				unwritten_nodes.push_back(current_node->get_child_at_index_const(child_index));
			}
		}
	}

	file_out.close();
}

bool grid_octree_hierarchical::
load_grid_from_file(const std::string& file_path)
{
	std::lock_guard<std::mutex> lock(mutex_);

	std::fstream file_in;
	file_in.open(file_path, std::ios::in);

	if(!file_in.is_open())
	{
		return false;
	}

	// Start reading the header info which is used to recreate the grid.
	std::string grid_type;
	file_in >> grid_type;
	if(grid_type != "octree_hierarchical")
	{
		return false;
	}

	// Read data to create root node.
	double root_size;
	file_in >> root_size;

	double pos_x, pos_y, pos_z;
	file_in >> pos_x >> pos_y >> pos_z;

	if(root_node_ != nullptr)
	{
		delete root_node_;
	}

	root_node_ = new grid_octree_node(root_size, scm::math::vec3d(pos_x, pos_y, pos_z));

	// Read and create octree structure.
	std::deque<grid_octree_node*> nodes_to_check;
	nodes_to_check.push_back(root_node_);

	while(nodes_to_check.size() != 0)
	{
		grid_octree_node* top_node = nodes_to_check[0];
		nodes_to_check.pop_front();
		
		char split_command;
		file_in >> split_command;

		if(split_command == '1')
		{
			top_node->split();
			
			for(size_t child_index = 0; child_index < 8; ++child_index)
			{
				nodes_to_check.push_back(top_node->get_child_at_index(child_index));
			}
		}
	}

	// Make sure view cells are accessible.
	compute_index_access();

	// Read the number of models.
	size_t num_models = 0;
	file_in >> num_models;
	ids_.resize(num_models);

	// Read the number of nodes per model.
	for(size_t model_index = 0; model_index < ids_.size(); ++model_index)
	{
		node_t num_nodes = 0;
		file_in >> num_nodes;
		ids_[model_index] = num_nodes;
	}

	file_in.close();
	return true;
}


bool grid_octree_hierarchical::
load_visibility_from_file(const std::string& file_path)
{
	std::lock_guard<std::mutex> lock(mutex_);

	std::fstream file_in;
	file_in.open(file_path, std::ios::in | std::ios::binary);

	if(!file_in.is_open())
	{
		return false;
	}

	// Iterate over view cells.
	std::deque<grid_octree_node*> unread_nodes;
	unread_nodes.push_back(root_node_);

	while(unread_nodes.size() != 0)
	{
		grid_octree_hierarchical_node* current_node = (grid_octree_hierarchical_node*)unread_nodes[0];
		unread_nodes.pop_front();

		for(model_t model_index = 0; model_index < ids_.size(); ++model_index)
		{
			// Read number of values to be read afterwards.
			node_t number_visibility_elements;
			file_in.read(reinterpret_cast<char*>(&number_visibility_elements), sizeof(number_visibility_elements));

			for(node_t node_index = 0; node_index < number_visibility_elements; ++node_index)
			{
				node_t visibility_index;
				file_in.read(reinterpret_cast<char*>(&visibility_index), sizeof(visibility_index));
				current_node->set_visibility(model_index, visibility_index, true);
			}
		}

		// Add child nodes of current nodes to queue.
		if(current_node->has_children())
		{
			for(size_t child_index = 0; child_index < 8; ++child_index)
			{
				unread_nodes.push_back(current_node->get_child_at_index(child_index));
			}
		}
	}

	// Iterate over view cells.
	std::deque<grid_octree_node*> unpropagated_nodes;
	unpropagated_nodes.push_back(root_node_);

	while(unpropagated_nodes.size() != 0)
	{
		grid_octree_hierarchical_node* current_node = (grid_octree_hierarchical_node*)unpropagated_nodes[0];
		unpropagated_nodes.pop_front();

		if(current_node->has_children())
		{
			for(size_t child_index = 0; child_index < 8; ++child_index)
			{
				grid_octree_node* child_node = current_node->get_child_at_index(child_index);
				std::map<model_t, std::vector<node_t>> visible_indices = current_node->get_visible_indices();

				for(model_t model_index = 0; model_index < ids_.size(); ++model_index)
				{
					const std::vector<node_t>& model_visibility = visible_indices[model_index];

					for(node_t node_index = 0; node_index < model_visibility.size(); ++node_index)
					{
						child_node->set_visibility(model_index, model_visibility[node_index], true);
					}
				}

				// Add child nodes of current nodes to queue.
				unpropagated_nodes.push_back(child_node);
			}

			// Clear node visibility since it has been propagated to children.
			current_node->clear_visibility_data();
		}
	}

	file_in.close();
	return true;
}

void grid_octree_hierarchical::
combine_visibility(const unsigned short& num_allowed_unequal_elements)
{
	grid_octree_hierarchical_node* root_hierarchical_node = (grid_octree_hierarchical_node*)root_node_;
	root_hierarchical_node->combine_visibility(ids_, num_allowed_unequal_elements);
}

}
}