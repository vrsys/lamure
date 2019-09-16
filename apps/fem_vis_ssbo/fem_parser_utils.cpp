#include "fem_parser_utils.h"


void fem_attributes_per_simulation_step::serialize(uint64_t byte_offset_timestep, std::vector<float>& target) const {
    uint64_t bytes_serialized_so_far_current_timestep = 0;

    for(int attribute_index = 0; attribute_index < int(FEM_attrib::NUM_FEM_ATTRIBS); ++attribute_index) {
      auto const current_FEM_attribute = FEM_attrib(attribute_index);
      auto const current_attribute_vec_it = data.find(current_FEM_attribute);

      if(data.end() == current_attribute_vec_it) {
        std::cout << "Attribute was not in map! Exiting" << std::endl;
        exit(-1);
      }

      auto const& current_attribute_source_vector = *current_attribute_vec_it;
      size_t num_bytes_to_copy_from_source_vector = current_attribute_source_vector.second.size() * sizeof(float);


      memcpy( (char*) target.data() + byte_offset_timestep + bytes_serialized_so_far_current_timestep, 
              (char*) current_attribute_source_vector.second.data(), 
              num_bytes_to_copy_from_source_vector
            );

      bytes_serialized_so_far_current_timestep += num_bytes_to_copy_from_source_vector;

    }
}


char* fem_attributes_per_time_series::serialize_time_series()   {

	    if(serialized_time_series.empty()) { //only serialize if it was not yet serialized
      uint64_t total_num_floats_in_series = 0;

      uint64_t num_byte_per_timestep = 0;
      bool is_first_timestep = true;
      for(auto const& time_step : series) {
        for(auto const& attribute_vector : time_step.data) {
          total_num_floats_in_series += attribute_vector.second.size();

          if(is_first_timestep) {
            num_byte_per_timestep += attribute_vector.second.size() * sizeof(float);
          }
        }
        
        is_first_timestep = false;
      }


      serialized_time_series.resize(total_num_floats_in_series);

      uint64_t time_step_idx = 0;
      for(auto const& time_step : series) {
        time_step.serialize( time_step_idx * num_byte_per_timestep, serialized_time_series);

        ++time_step_idx;
      }
    }

    return (char*) serialized_time_series.data();
}