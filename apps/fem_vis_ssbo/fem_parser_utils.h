#ifndef FEM_VIS_SSBO_PARSER_UTILS_H_
#define FEM_VIS_SSBO_PARSER_UTILS_H_

#include <algorithm>
#include <cstdint>
#include <cstring> //memcpy
#include <exception>
#include <iostream>
#include <map>
#include <vector>

#include <scm/core.h>
#include <scm/core/math.h>

class FEM_path_not_a_directory_exception: public std::exception
{
  virtual const char* what() const throw()
  {
    return "One of the provided FEM paths in the mapping file was not a directory!";
  }
};

class time_series_already_parsed_exception: public std::exception
{
  virtual const char* what() const throw()
  {
    return "Time series for attribute was already parsed!";
  }
};

class simulation_not_parsed_exception: public std::exception
{
  virtual const char* what() const throw()
  {
    return "The simulation you wanted to get data from was not loaded! Is the name correct and the attribute registered in the mapping-file?";
  }
};

class no_FEM_simulation_parsed_exception: public std::exception
{
  virtual const char* what() const throw()
  {
    return "No FEM simulation was parsed successfully, such that there is no valid name to provide get_data_ptr_to_simulation_data(...) with.";
  }
};

class unequal_number_of_FEM_vertices_in_time_series: public std::exception
{
  virtual const char* what() const throw()
  {
    return "Different time steps in the same time series had different numbers of FEM result lines! This is forbidden.";
  }
};

class attribute_not_present_in_series: public std::exception
{
  virtual const char* what() const throw()
  {
    return "The requested attribute was not defined in the parsed FEM series.";
  }
};

// all 9 attributes that comply with the current data
// allows back- and forth-casting between enum classes and int
enum class FEM_attrib {
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

  NUM_FEM_ATTRIBS = 10, // total number of vectors (needs to stay for convenient iteration over attributes)
};

// contains all simulated attributes per simulation step
struct fem_attributes_per_simulation_step {
  std::map<FEM_attrib, std::vector<float> > data;
  
  // for now we do not store the min and max value of the U-vector
  std::map<FEM_attrib, float> local_min_val;  //min values for each attribute of the current timestep
  std::map<FEM_attrib, float> local_max_val;  //max values for each attribute of the current timestep

  void serialize(uint64_t byte_offset_timestep, std::vector<float>& target) const;
};

// contains all fem attributes for a temporal simulation
struct fem_attributes_per_time_series {
  std::vector<fem_attributes_per_simulation_step> series;

  int32_t num_vertices_in_fem_model = -1; //-1 indicates, that this time series has not been parsed yet
  int32_t num_timesteps_in_series = 0; // is 1 for static simulations, MAX_NUM_TIMESTEPS for dynamic one

  std::map<FEM_attrib, float> global_min_val; //global min value for each attribute of the entire attribute
  std::map<FEM_attrib, float> global_max_val; //global max value for each attribute of the entire attribute

  std::vector<float> serialized_time_series;

  float max_simulation_time_in_milliseconds = 0.0f;

  char* serialize_time_series();

};

// contains all time series (also individual attributes) of an entire collection defined in an fem_value_mapping_file
struct fem_attribute_collection {
  // key:   name of the parent folder of the attributes as std::string
  // value: time series data as as fem_attributes_per_time_series
  std::map<std::string, fem_attributes_per_time_series> data;



  uint64_t get_max_num_timesteps_in_collection() const;

  //i.e. How much size do we need to allocate in the SSBO to store - any - complete simulation series
  uint64_t get_max_num_elements_per_simulation() const;

  uint64_t get_num_vertices_per_simulation(std::string const& simulation_name) const;
  uint64_t get_num_timesteps_per_simulation(std::string const& simulation_name) const;

  uint64_t get_num_attributes_per_simulation(std::string const& simulation_name) const { return 10;}

  char* get_data_ptr_to_simulation_data(std::string const& simulation_name);

  // returns local minimum and maximum for desired attribute (->global for this time step only)
  //std::pair<float, float> get_local_extrema_for_attribute_in_timestep(FEM_attrib const& simulation_attrib, std::string const& simulation_name, int32_t time_step) const;
  // returns global minimum and maximum for desired attribute (->global for entire time series)
  std::pair<float, float> get_global_extrema_for_attribute_in_series(FEM_attrib const& simulation_attrib, std::string const& simulation_name) const;

};


void parse_file_to_fem(std::string const& attribute_name, std::string const& sorted_fem_time_series_files, 
                       fem_attribute_collection& fem_collection, scm::math::mat4f const& fem_to_pcl_transform);

void parse_directory_to_fem(std::string const& simulation_name, // e.g. "Temperatur"
                            std::vector<std::string> const& sorted_fem_time_series_files,
                            fem_attribute_collection& fem_collection, scm::math::mat4f const& fem_to_pcl_transform);

std::vector<std::string> parse_fem_collection(std::string const& fem_mapping_file_path, 
                                              fem_attribute_collection& fem_collection, scm::math::mat4f const& fem_to_pcl_transform);

#endif //FEM_VIS_SSBO_PARSER_UTILS_H_