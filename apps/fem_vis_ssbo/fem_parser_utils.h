#ifndef FEM_VIS_SSBO_PARSER_UTILS_H_
#define FEM_VIS_SSBO_PARSER_UTILS_H_

#include <algorithm>
#include <cstdint>
#include <cstring> //memcpy
#include <exception>
#include <iostream>
#include <map>
#include <vector>



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

// all 9 attributes that comply with the current data
// allows back- and forth-casting between enum classes and int
enum class FEM_attrib {
  U_XYZ  , // verschiebung der Punkte, z positiv nach unten gerichtet   ; 3 attribute
  SIG_XX , // Normalspannung; Kraft bezogen auf eine Fläche in lokale x-Richtung (Richtung eines Stabes); 1 attribut
  TAU_XY , //Schubspannung entlang Flaechen xy 
  TAU_XZ , //Schubspannung entlang Flaechen xz
  TAU_ABS, //Betrag addierter Schubspannungsvektoren xy und xz
  SIG_V  , //Vergleichsspannung
  EPS_X,    // Dehnung

  NUM_FEM_ATTRIBS // total number of vectors (needs to stay for convenient iteration over attributes)
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

  std::map<FEM_attrib, float> global_min_val; //global min value for each attribute of the entire attribute
  std::map<FEM_attrib, float> global_max_val; //global max value for each attribute of the entire attribute

  std::vector<float> serialized_time_series;


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

  char* get_data_ptr_to_simulation_data(std::string const& simulation_name);

};


void parse_file_to_fem(std::string const& attribute_name, std::string const& sorted_fem_time_series_files, fem_attribute_collection& fem_collection);

void parse_directory_to_fem(std::string const& simulation_name, // e.g. "Temperatur"
                            std::vector<std::string> const& sorted_fem_time_series_files,
                            fem_attribute_collection& fem_collection);

std::vector<std::string> parse_fem_collection(std::string const& fem_mapping_file_path, fem_attribute_collection& fem_collection);

#endif //FEM_VIS_SSBO_PARSER_UTILS_H_