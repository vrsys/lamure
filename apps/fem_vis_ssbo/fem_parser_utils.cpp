#include "fem_parser_utils.h"

#include <fstream>
#include <sstream>
#include <string>

//boost
#include <boost/assign/list_of.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>



//entire set of attributes for one simulation (frame, e.g. time step 58 of Eigenform_003)
void fem_attributes_per_simulation_step::
serialize(uint64_t byte_offset_timestep, std::vector<float>& target) const {
    uint64_t bytes_serialized_so_far_current_timestep = 0;

    uint32_t already_serialized_attributes = 0;

    for(int attribute_index = 0; attribute_index < int(FEM_attrib::NUM_FEM_ATTRIBS); ++attribute_index) {
      auto const current_FEM_attribute = FEM_attrib(attribute_index);
      auto const current_attribute_vec_it = data.find(current_FEM_attribute);

      if(FEM_attrib(attribute_index) == FEM_attrib::MAG_U) {
        //std::cout << "Attribute index of MAG U " << already_serialized_attributes << std::endl;
      }

      if(data.end() == current_attribute_vec_it) {
        std::cout << "Attribute was not in map! Exiting" << std::endl;
        exit(-1);
      }

      auto const& current_attribute_source_vector = *current_attribute_vec_it;
      size_t num_bytes_to_copy_from_source_vector = current_attribute_source_vector.second.size() * sizeof(float);


      //std::cout << " " ;

      memcpy( (char*) target.data() + byte_offset_timestep + bytes_serialized_so_far_current_timestep, 
              (char*) current_attribute_source_vector.second.data(), 
              num_bytes_to_copy_from_source_vector
            );

      bytes_serialized_so_far_current_timestep += num_bytes_to_copy_from_source_vector;


      ++already_serialized_attributes;
    }
}


//time series for individual simulations (e.g. Series of fem_attributes_per_simulation_step for Eigenform_003)
char* fem_attributes_per_time_series::
serialize_time_series()   {

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


// entire collection, (all fem_attributes_per_time_series for Temperatur, Ausbaulast, Eigenform_001, Eigenform_002, ...)
uint64_t fem_attribute_collection::
get_max_num_timesteps_in_collection() const {
uint64_t max_num_timesteps = 0;
for(auto const& simulation : data) {
  max_num_timesteps = std::max(max_num_timesteps, simulation.second.series.size());
}
return max_num_timesteps; 
}


uint64_t fem_attribute_collection::
get_max_num_elements_per_simulation() const {
	uint64_t max_num_elements_per_simulation = 0;
	for(auto const& simulation : data) {
	  size_t num_elements_for_current_simulation = 0;
	  for(auto const& simulation_series : simulation.second.series) {
	    for(auto const& simulation_attribute : simulation_series.data) {
	      //for(auto const& simulation_attribute : simulation_frame) {
	        num_elements_for_current_simulation += simulation_attribute.second.size();
	      //}

	    }
	  }
	  max_num_elements_per_simulation = std::max(num_elements_for_current_simulation, max_num_elements_per_simulation);
	  //max_num_timesteps = std::max(max_num_timesteps, simulation.second.series.size());
	}
	return max_num_elements_per_simulation; 
}


void throw_annotated_simulation_not_parsed_exception(std::string const& simulation_name) {
	std::cout << "Exception regarding simulation: " << simulation_name << std::endl;
	throw simulation_not_parsed_exception();
}

char* fem_attribute_collection::
get_data_ptr_to_simulation_data(std::string const& simulation_name) { // simulation_name is for instance "Temperatur", "Eigenform_003", ....

	auto time_series_iterator = data.find(simulation_name);

	if(data.end() == time_series_iterator) {
		throw_annotated_simulation_not_parsed_exception(simulation_name);
	}

	return time_series_iterator->second.serialize_time_series();
}


uint64_t fem_attribute_collection::
get_num_vertices_per_simulation(std::string const& simulation_name) const {
	auto time_series_iterator = data.find(simulation_name);

	if(data.end() == time_series_iterator) {
		throw_annotated_simulation_not_parsed_exception(simulation_name);
	}

 	return time_series_iterator->second.num_vertices_in_fem_model;
 }


uint64_t fem_attribute_collection::
get_num_timesteps_per_simulation(std::string const& simulation_name) const {
	auto time_series_iterator = data.find(simulation_name);

	if(data.end() == time_series_iterator) {
		throw_annotated_simulation_not_parsed_exception(simulation_name);
	}

 	return time_series_iterator->second.num_timesteps_in_series;
}

// to be implemented on demand
/*
std::pair<float, float>  fem_attribute_collection::
get_local_extrema_for_attribute_in_timestep(FEM_attrib const& simulation_attrib, std::string const& simulation_name, int32_t time_step) const {
	auto time_series_iterator = data.find(simulation_name);

	if(data.end() == time_series_iterator) {
		throw_annotated_simulation_not_parsed_exception(simulation_name);
	}
}*/

std::pair<float, float>  fem_attribute_collection::
get_global_extrema_for_attribute_in_series(FEM_attrib const& simulation_attrib, std::string const& simulation_name) const {
	auto time_series_iterator = data.find(simulation_name);

	if(data.end() == time_series_iterator) {
		throw_annotated_simulation_not_parsed_exception(simulation_name);
	}

	auto const& current_minima_map = time_series_iterator->second.global_min_val;

	auto min_attrib_iterator_for_series = current_minima_map.find(simulation_attrib);
	if(current_minima_map.end() == min_attrib_iterator_for_series) {
		throw attribute_not_present_in_series();
	}

	auto const& current_maxima_map = time_series_iterator->second.global_max_val;
	auto max_attrib_iterator_for_series = current_maxima_map.find(simulation_attrib);
	if(current_maxima_map.end() == max_attrib_iterator_for_series) {
		throw attribute_not_present_in_series();
	}

	std::pair<float, float> global_extrema {min_attrib_iterator_for_series->second, max_attrib_iterator_for_series->second};

	return global_extrema;
}

float fem_attribute_collection::get_simulation_duration(std::string const& simulation_name) const {
  auto time_series_iterator = data.find(simulation_name);

  if(data.end() == time_series_iterator) {
    throw_annotated_simulation_not_parsed_exception(simulation_name);
  }

  auto  current_simulation_duration = time_series_iterator->second.max_simulation_timestamp_in_milliseconds;

  return current_simulation_duration; 
}


/* free Functions for parsing:

	collections consisting of
		time series  consisting of
			time steps consisting of a number of attributes per FEM vertex
*/

void calculate_extrema_for_series(fem_attributes_per_simulation_step& current_attributes, fem_attributes_per_time_series& current_time_series) {

    for(int FEM_attrib_idx = 0; FEM_attrib_idx < int(FEM_attrib::NUM_FEM_ATTRIBS); ++FEM_attrib_idx) {

      auto const current_FEM_attrib = FEM_attrib(FEM_attrib_idx); //cast int -> enum class object
      current_attributes.local_min_val[current_FEM_attrib] = std::numeric_limits<float>::max();
      current_attributes.local_max_val[current_FEM_attrib] = std::numeric_limits<float>::lowest();

      auto const& current_attribute_vector = current_attributes.data[current_FEM_attrib];


      //get min element iterator in vector 
      auto min_element_it = std::min_element(current_attribute_vector.begin(), current_attribute_vector.end());
      //update local minimum (for time step)
      current_attributes.local_min_val[current_FEM_attrib] = std::min(current_attributes.local_min_val[current_FEM_attrib], *min_element_it);
      //update global minimum (for time series)
      current_time_series.global_min_val[current_FEM_attrib] = std::min(current_time_series.global_min_val[current_FEM_attrib], *min_element_it);
      

      //get max element iterator in vector
      auto max_element_it = std::max_element(current_attribute_vector.begin(), current_attribute_vector.end());
      //update local maximum (for time step)
      current_attributes.local_max_val[current_FEM_attrib] = std::max(current_attributes.local_max_val[current_FEM_attrib], *max_element_it);
      //update global maximum (for time series)

      current_time_series.global_max_val[current_FEM_attrib] = std::max(current_time_series.global_max_val[current_FEM_attrib], *max_element_it);

      //std::cout << "Attrib " << int(current_FEM_attrib) << std::endl;
      //if(2 == FEM_attrib_idx) {
      //  std::cout << *max_element_it << std::endl;
     // }

      if(FEM_attrib(FEM_attrib_idx) == FEM_attrib::MAG_U) {
        //std::cout << "Lokales Minimum Magnitude der Verformung: " << current_attributes.local_min_val[current_FEM_attrib] << std::endl;
        //std::cout << "Lokales Maximum Magnitude der Verformung: " << current_attributes.local_max_val[current_FEM_attrib] << std::endl;

        //std::cout << "Globales Minimum Magnitude der Verformung: " << current_time_series.global_min_val[current_FEM_attrib] << std::endl;
        //std::cout << "Globales Maximum Magnitude der Verformung: " << current_time_series.global_max_val[current_FEM_attrib] << std::endl;
      }
    }





}

void dump_file_to_fem_bin(std::string const& sorted_fem_time_series_file, 
                          fem_attributes_per_simulation_step& current_attributes_per_timestep_of_simulation) {
  std::string const out_fem_bin_file_string = sorted_fem_time_series_file + ".bin";

  std::ofstream out_fem_bin_file(out_fem_bin_file_string, std::ios::binary | std::ios::out);

/*
  U_X     = 0,  // deformation entlang X-Achse
  U_Y     = 1,  // deformation entlang Y-Achse
  U_Z     = 2,  // deformation entlang Z-Achse
  MAG_U   = 3,  // magnitude of U, is computed from U_XYZ (not parsed)
  SIG_XX  = 4, // Normalspannung; Kraft bezogen auf eine Fläche in lokale x-Richtung (Richtung eines Stabes); 1 attribut
  TAU_XY  = 5, //Schubspannung entlang Flaechen xy 
  TAU_XZ  = 6, //Schubspannung entlang Flaechen xz
  TAU_ABS = 7,  //Betrag addierter Schubspannungsvektoren xy und xz
  SIG_V   = 8, //Vergleichsspannung
  EPS_X   = 9,    // Dehnung
*/

  // make sure that the order strictly is the same as in the python script for parsing mat files for consitence
  out_fem_bin_file.write((char*)current_attributes_per_timestep_of_simulation.data[FEM_attrib::U_X].data(), 
                                current_attributes_per_timestep_of_simulation.data[FEM_attrib::U_X].size() * sizeof(float));
  out_fem_bin_file.write((char*)current_attributes_per_timestep_of_simulation.data[FEM_attrib::U_Y].data(), 
                                current_attributes_per_timestep_of_simulation.data[FEM_attrib::U_Y].size() * sizeof(float));
  out_fem_bin_file.write((char*)current_attributes_per_timestep_of_simulation.data[FEM_attrib::U_Z].data(), 
                                current_attributes_per_timestep_of_simulation.data[FEM_attrib::U_Z].size() * sizeof(float));
  out_fem_bin_file.write((char*)current_attributes_per_timestep_of_simulation.data[FEM_attrib::MAG_U].data(), 
                                current_attributes_per_timestep_of_simulation.data[FEM_attrib::MAG_U].size() * sizeof(float));
  out_fem_bin_file.write((char*)current_attributes_per_timestep_of_simulation.data[FEM_attrib::SIG_XX].data(), 
                                current_attributes_per_timestep_of_simulation.data[FEM_attrib::SIG_XX].size() * sizeof(float));
  out_fem_bin_file.write((char*)current_attributes_per_timestep_of_simulation.data[FEM_attrib::TAU_XY].data(), 
                                current_attributes_per_timestep_of_simulation.data[FEM_attrib::TAU_XY].size() * sizeof(float));
  out_fem_bin_file.write((char*)current_attributes_per_timestep_of_simulation.data[FEM_attrib::TAU_XZ].data(), 
                                current_attributes_per_timestep_of_simulation.data[FEM_attrib::TAU_XZ].size() * sizeof(float));
  out_fem_bin_file.write((char*)current_attributes_per_timestep_of_simulation.data[FEM_attrib::TAU_ABS].data(), 
                                current_attributes_per_timestep_of_simulation.data[FEM_attrib::TAU_ABS].size() * sizeof(float));
  out_fem_bin_file.write((char*)current_attributes_per_timestep_of_simulation.data[FEM_attrib::SIG_V].data(), 
                                current_attributes_per_timestep_of_simulation.data[FEM_attrib::SIG_V].size() * sizeof(float));
  out_fem_bin_file.write((char*)current_attributes_per_timestep_of_simulation.data[FEM_attrib::EPS_X].data(), 
                                current_attributes_per_timestep_of_simulation.data[FEM_attrib::EPS_X].size() * sizeof(float));


  /*
  for(int fem_attrib_idx = 0; fem_attrib_idx < int(FEM_attrib::NUM_FEM_ATTRIBS); ++fem_attrib_idx) {
    FEM_attrib const current_fem_attrib = FEM_attrib(fem_attrib_idx);

    out_fem_bin_file.write((char*)current_attributes_per_timestep_of_simulation.data[current_fem_attrib].data(), current_attributes_per_timestep_of_simulation.data[current_fem_attrib].size() * sizeof(float));
  }
  */
  out_fem_bin_file.close();
}


//gets called if bin file was available for fem simulation
void read_fem_bin_file_to_time_step(std::string const& simulation_name,
                                    std::string const& sorted_fem_time_series_file_bin, 
                                    fem_attribute_collection& fem_collection,
                                    scm::math::mat4f const& fem_to_pcl_transform) {

  std::string const in_fem_bin_file_string = sorted_fem_time_series_file_bin;

  // open file in binary mode at the end
  std::ifstream in_fem_bin_file(in_fem_bin_file_string, std::ios::binary | std::ios::in | std::ios::ate);

  // get number of byte in the file
  int64_t const total_num_bytes_in_file = in_fem_bin_file.tellg();


  // go back to beginning of file

  in_fem_bin_file.seekg(0);

  int64_t const total_num_bytes_per_attribute = total_num_bytes_in_file / int(FEM_attrib::NUM_FEM_ATTRIBS);
  int64_t const total_num_floats_per_attribute = total_num_bytes_per_attribute / sizeof(float);


  auto& current_time_series = fem_collection.data[simulation_name];

  current_time_series.series.push_back(fem_attributes_per_simulation_step{}); 

  auto& current_attributes = current_time_series.series.back();


  if(current_time_series.num_vertices_in_fem_model < 0) {
    current_time_series.num_vertices_in_fem_model = total_num_floats_per_attribute;
  } else {
    if(current_time_series.num_vertices_in_fem_model != total_num_floats_per_attribute) {
      std::cout << "Exception regarding simulation " << simulation_name << ": " << std::endl;
      throw unequal_number_of_FEM_vertices_in_time_series();
    }
  }

  current_attributes.data[FEM_attrib::U_X].resize(total_num_floats_per_attribute);
  in_fem_bin_file.read((char*)current_attributes.data[FEM_attrib::U_X].data(), total_num_bytes_per_attribute);
  current_attributes.data[FEM_attrib::U_Y].resize(total_num_floats_per_attribute);
  in_fem_bin_file.read((char*)current_attributes.data[FEM_attrib::U_Y].data(), total_num_bytes_per_attribute);
  current_attributes.data[FEM_attrib::U_Z].resize(total_num_floats_per_attribute);
  in_fem_bin_file.read((char*)current_attributes.data[FEM_attrib::U_Z].data(), total_num_bytes_per_attribute);
  current_attributes.data[FEM_attrib::MAG_U].resize(total_num_floats_per_attribute);
  in_fem_bin_file.read((char*)current_attributes.data[FEM_attrib::MAG_U].data(), total_num_bytes_per_attribute);
  current_attributes.data[FEM_attrib::SIG_XX].resize(total_num_floats_per_attribute);
  in_fem_bin_file.read((char*)current_attributes.data[FEM_attrib::SIG_XX].data(), total_num_bytes_per_attribute);
  current_attributes.data[FEM_attrib::TAU_XY].resize(total_num_floats_per_attribute);
  in_fem_bin_file.read((char*)current_attributes.data[FEM_attrib::TAU_XY].data(), total_num_bytes_per_attribute);
  current_attributes.data[FEM_attrib::TAU_XZ].resize(total_num_floats_per_attribute);
  in_fem_bin_file.read((char*)current_attributes.data[FEM_attrib::TAU_XZ].data(), total_num_bytes_per_attribute);
  current_attributes.data[FEM_attrib::TAU_ABS].resize(total_num_floats_per_attribute);
  in_fem_bin_file.read((char*)current_attributes.data[FEM_attrib::TAU_ABS].data(), total_num_bytes_per_attribute);
  current_attributes.data[FEM_attrib::SIG_V].resize(total_num_floats_per_attribute);
  in_fem_bin_file.read((char*)current_attributes.data[FEM_attrib::SIG_V].data(), total_num_bytes_per_attribute);
  current_attributes.data[FEM_attrib::EPS_X].resize(total_num_floats_per_attribute);
  in_fem_bin_file.read((char*)current_attributes.data[FEM_attrib::EPS_X].data(), total_num_bytes_per_attribute);


//transform
  for(int64_t element_idx = 0; element_idx < total_num_floats_per_attribute; ++element_idx) {

    scm::math::vec4f untransformed_deformation_u_xyz{current_attributes.data[FEM_attrib::U_X][element_idx], 
                                                     current_attributes.data[FEM_attrib::U_Y][element_idx], 
                                                     current_attributes.data[FEM_attrib::U_Z][element_idx], 0.0f};


    scm::math::vec4f transformed_deformation_u_xyz = fem_to_pcl_transform * untransformed_deformation_u_xyz;

    current_attributes.data[FEM_attrib::U_X][element_idx] = transformed_deformation_u_xyz[0];
    current_attributes.data[FEM_attrib::U_Y][element_idx] = transformed_deformation_u_xyz[1]; 
    current_attributes.data[FEM_attrib::U_Z][element_idx] = transformed_deformation_u_xyz[2];



  }


  ++current_time_series.num_timesteps_in_series;

  calculate_extrema_for_series(current_attributes, current_time_series);

  in_fem_bin_file.close();
}


//gets called if no bin file was available for fem simulation
void parse_file_to_fem(std::string const& simulation_name, 
					             std::string const& sorted_fem_time_series_files, 
					             fem_attribute_collection& fem_collection, scm::math::mat4f const& fem_to_pcl_transform) {



  auto& current_time_series = fem_collection.data[simulation_name];


  int total_num_lines = 0;

  {
    std::ifstream in_file_stream(sorted_fem_time_series_files);
    std::string line_buffer;

    while(std::getline(in_file_stream,line_buffer)) {
      ++total_num_lines;
    }

    //std::cout << "Total num lines in file: " << total_num_lines << std::endl;
  
    int64_t total_num_attribute_lines = total_num_lines - 1;



    current_time_series.series.push_back(fem_attributes_per_simulation_step{}); 

    auto& current_attributes = current_time_series.series.back();

    if(current_time_series.num_vertices_in_fem_model < 0) {
    	current_time_series.num_vertices_in_fem_model = total_num_attribute_lines;
    } else {
    	if(current_time_series.num_vertices_in_fem_model != total_num_attribute_lines) {
    		std::cout << "Exception regarding simulation " << simulation_name << ": " << std::endl;
    		throw unequal_number_of_FEM_vertices_in_time_series();
    	}
    }

    current_attributes.data[FEM_attrib::U_X].resize(total_num_attribute_lines);
    current_attributes.data[FEM_attrib::U_Y].resize(total_num_attribute_lines);
    current_attributes.data[FEM_attrib::U_Z].resize(total_num_attribute_lines);
    current_attributes.data[FEM_attrib::MAG_U].resize(total_num_attribute_lines);
    current_attributes.data[FEM_attrib::SIG_XX].resize(total_num_attribute_lines);
    current_attributes.data[FEM_attrib::TAU_XY].resize(total_num_attribute_lines);
    current_attributes.data[FEM_attrib::TAU_XZ].resize(total_num_attribute_lines);
    current_attributes.data[FEM_attrib::TAU_ABS].resize(total_num_attribute_lines);
    current_attributes.data[FEM_attrib::SIG_V].resize(total_num_attribute_lines);
    current_attributes.data[FEM_attrib::EPS_X].resize(total_num_attribute_lines);



    auto transformed_U_X = current_attributes.data[FEM_attrib::U_X];
    auto transformed_U_Y = current_attributes.data[FEM_attrib::U_Y];
    auto transformed_U_Z = current_attributes.data[FEM_attrib::U_Z];


    // rewind file
    in_file_stream.clear();
    in_file_stream.seekg(0, std::ios::beg);

    int current_line_count = 0;


    //in_line_stringstream.clear();


    while(std::getline(in_file_stream, line_buffer)) {
      //ignore first line

        if(0 != current_line_count) {
          std::stringstream in_line_stringstream(line_buffer);
          //in_line_stringstream.str(line_buffer);

          int64_t const line_write_count = current_line_count - 1;

          float vertex_idx; //parse and throw away
          in_line_stringstream >> vertex_idx;

          
          scm::math::vec4f untransformed_deformation_u_xyz{0.0f, 0.0f, 0.0f, 0.0f};

          in_line_stringstream >> untransformed_deformation_u_xyz[0];
          in_line_stringstream >> untransformed_deformation_u_xyz[1];
          in_line_stringstream >> untransformed_deformation_u_xyz[2];
          

          current_attributes.data[FEM_attrib::U_X][line_write_count] = untransformed_deformation_u_xyz[0];
          current_attributes.data[FEM_attrib::U_Y][line_write_count] = untransformed_deformation_u_xyz[1];
          current_attributes.data[FEM_attrib::U_Z][line_write_count] = untransformed_deformation_u_xyz[2];

          scm::math::vec4f transformed_deformation_u_xyz = fem_to_pcl_transform * untransformed_deformation_u_xyz;

          transformed_U_X[line_write_count] = transformed_deformation_u_xyz[0];
          transformed_U_Y[line_write_count] = transformed_deformation_u_xyz[1]; 
          transformed_U_Z[line_write_count] = transformed_deformation_u_xyz[2];

          scm::math::vec3 deformation_vector = scm::math::vec3{current_attributes.data[FEM_attrib::U_X][line_write_count],
                                                               current_attributes.data[FEM_attrib::U_Y][line_write_count],
                                                               current_attributes.data[FEM_attrib::U_Z][line_write_count]};


          float length_of_vector = scm::math::length(deformation_vector);    

          //if(std::isinf(length_of_vector) || std::isnan(length_of_vector)) { 
          //  std::cout << "INVALID VALUE" << std::endl;
          //}

          current_attributes.data[FEM_attrib::MAG_U][line_write_count] =   length_of_vector;                                       

          //std::cout << length_of_vector << std::endl;



          in_line_stringstream >> current_attributes.data[FEM_attrib::SIG_XX][line_write_count];
          

          //std::cout << current_attributes.data[FEM_attrib::SIG_XX][line_write_count] << std::endl;


          in_line_stringstream >> current_attributes.data[FEM_attrib::TAU_XY][line_write_count];
          in_line_stringstream >> current_attributes.data[FEM_attrib::TAU_XZ][line_write_count];
          in_line_stringstream >> current_attributes.data[FEM_attrib::TAU_ABS][line_write_count];
          in_line_stringstream >> current_attributes.data[FEM_attrib::SIG_V][line_write_count];
          in_line_stringstream >> current_attributes.data[FEM_attrib::EPS_X][line_write_count]; 
        }

        ++current_line_count;

      }

    
    //we assume that the file has been parsed successfully at this point
    ++current_time_series.num_timesteps_in_series;

    in_file_stream.close();



    // old extrema 
    calculate_extrema_for_series(current_attributes, current_time_series);

    // serialize data (without transformed deformation)
    dump_file_to_fem_bin(sorted_fem_time_series_files, current_attributes);


    //after dumping the files to binaries, overwrite old U_X, U_Y and U_Z with transformed deformation
    current_attributes.data[FEM_attrib::U_X] = transformed_U_X;
    current_attributes.data[FEM_attrib::U_Y] = transformed_U_Y;
    current_attributes.data[FEM_attrib::U_Z] = transformed_U_Z;

  }



}

void parse_directory_to_fem(std::string const& simulation_name, // e.g. "Temperatur"
                            std::vector<std::string> const& sorted_fem_time_series_files,
                            fem_attribute_collection& fem_collection, scm::math::mat4f const& fem_to_pcl_transform,
                            std::string const& timesteps_file_path) {


  std::cout << "Parsing simulation with name: " << simulation_name << std::endl;
  if(fem_collection.data.end() == fem_collection.data.find(simulation_name)) {
  //  std::cout << "Parsing time series data for simulation \"" << simulation_name << "\""<< std::endl;

    fem_collection.data.insert(std::make_pair(simulation_name, fem_attributes_per_time_series{}) );


    auto& current_time_series = fem_collection.data[simulation_name];

    for(int FEM_attrib_idx = 0; FEM_attrib_idx < int(FEM_attrib::NUM_FEM_ATTRIBS); ++FEM_attrib_idx) {
      auto const current_FEM_attrib = FEM_attrib(FEM_attrib_idx); //cast int -> enum class object
      current_time_series.global_min_val[current_FEM_attrib] = std::numeric_limits<float>::max();
      current_time_series.global_max_val[current_FEM_attrib] = std::numeric_limits<float>::lowest(); 
    } //initiliaze global min and max vals


    for(auto const& file_path : sorted_fem_time_series_files) {

      std::string const bin_fem_file_path = file_path + ".bin";

      if(boost::filesystem::exists( bin_fem_file_path )) {
        read_fem_bin_file_to_time_step(simulation_name, bin_fem_file_path, fem_collection, fem_to_pcl_transform);

        //std::cout << "Trying to read stuff from bin file for simulation: " << simulation_name << std::endl;
      } else {

        if(boost::algorithm::ends_with(file_path, ".txt")) { // old ASCII format, we can parse this and dump out the corresponding bin file
          parse_file_to_fem(simulation_name, file_path, fem_collection, fem_to_pcl_transform);
        } else if(boost::algorithm::ends_with(file_path, ".mat")) { // we encountered a .mat file but do not have the corresponding .mat.bin file -- abort

          std::cout << "Regarding file " << file_path << ": " << std::endl;
          throw no_mat_file_parser_exception{};
        } else {
          std::cout << "Regarding file " << file_path << ": " << std::endl;
          throw unknown_file_exception{};
        }
      }


    

    }


    std::cout << "Global Minimum Normalspannung: " << fem_collection.data[simulation_name].global_min_val[FEM_attrib::SIG_XX] << std::endl;
    std::cout << "Global Maximum Normalspannung: " << fem_collection.data[simulation_name].global_max_val[FEM_attrib::SIG_XX] << std::endl;


    if("" != timesteps_file_path) {
      std::string timesteps_line_buffer;
      std::ifstream timesteps_filestream(timesteps_file_path);

      uint32_t line_counter = 0;
      while(std::getline(timesteps_filestream, timesteps_line_buffer)) {
        if(line_counter == sorted_fem_time_series_files.size()) {
          break;
        } 

        ++line_counter;
      }
      //std::cout << "Last line: " << timesteps_line_buffer << std::endl;

      std::istringstream last_timestep_line_buffer(timesteps_line_buffer);
      last_timestep_line_buffer >> fem_collection.data[simulation_name].max_simulation_timestamp_in_milliseconds;

      timesteps_filestream.close();
    } else {
      std::cout << "timesteps.txt was not available. Assuming static simulation" << std::endl << std::endl;
    }

  } else {
    std::cout << "Regarding attribute: " << simulation_name << std::endl;
    throw time_series_already_parsed_exception();
  }
}


std::vector<std::string> parse_fem_collection(std::string const& fem_mapping_file_path,
                          fem_attribute_collection& fem_collection, scm::math::mat4f const& fem_to_pcl_transform) {
  std::cout << "Starting to parse files defined in " << fem_mapping_file_path << std::endl;

  std::ifstream in_fem_mapping_file(fem_mapping_file_path);

  std::string fem_path_line_buffer;

  std::vector<std::string> successfully_parsed_simulation_names; 

  while(std::getline(in_fem_mapping_file, fem_path_line_buffer)) {
    //std::cout << "Read line: " <<  fem_path_line_buffer  << std::endl;

    if ( !boost::filesystem::exists( fem_path_line_buffer ) )
    {
      std::cout << "Can't find my file!" << std::endl;
    } else {
      //std::cout << "File or path exists - starting to parse it to FEM attribute series!" << std::endl;

      bool is_file = boost::filesystem::is_regular_file(fem_path_line_buffer);

      //std::cout << "Is: " <<  ( is_file ?  "File!" : "Directory!") << std::endl; 

      if(is_file) {
        throw FEM_path_not_a_directory_exception();
      } else {

        std::string const path_to_time_series = fem_path_line_buffer;
        boost::filesystem::path p{fem_path_line_buffer};


        std::string const FEM_simulation_name(p.filename().c_str());



        const std::string directory_name_only( FEM_simulation_name );


        std::vector<std::string> sorted_fem_time_series_files;

        bool is_first_file_of_simulation = true;

        boost::filesystem::directory_iterator end_itr; // Default ctor yields past-the-end
        for( boost::filesystem::directory_iterator dir_iterator( fem_path_line_buffer ); dir_iterator != end_itr; ++dir_iterator )
        {
            //std::cout << "Trying: " << *dir_iterator << std::endl;
            std::string const currently_iterated_filename = dir_iterator->path().filename().string();

            std::string const full_file_path = path_to_time_series + "/" + currently_iterated_filename;
            if(boost::filesystem::is_regular_file(full_file_path)) {  //one last check for whether we are really holding a file in our hands
                if(currently_iterated_filename.rfind(FEM_simulation_name) == 0) { // < ---- starts with time series prefix

                  if(!boost::algorithm::ends_with(currently_iterated_filename, ".bin")) {

                    sorted_fem_time_series_files.push_back(full_file_path);

                    if(is_first_file_of_simulation) {
                  	 is_first_file_of_simulation = false;
                  	 successfully_parsed_simulation_names.push_back(FEM_simulation_name);
                    }
                  }
                }
            }
        }

        std::sort(sorted_fem_time_series_files.begin(), sorted_fem_time_series_files.end());

        std::string timesteps_filepath = path_to_time_series + "/timesteps.txt";

        if ( !boost::filesystem::exists( timesteps_filepath ) ) {
          std::cout << "Could not find \"timesteps.txt\" file for simulation series." << std::endl;
          timesteps_filepath = "";
        }

        parse_directory_to_fem(FEM_simulation_name, sorted_fem_time_series_files, fem_collection, fem_to_pcl_transform, timesteps_filepath);

        //for(auto const& time_series_path : sorted_fem_time_series_files) {
        //  std::cout << "X: " << time_series_path << std::endl;
        //}

        //std::cout << dir_iterator->path().filename().string() << std::endl;



      }
    } 
  }

  in_fem_mapping_file.close();

  return successfully_parsed_simulation_names;
}