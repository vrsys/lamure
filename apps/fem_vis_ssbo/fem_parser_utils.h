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



  uint64_t get_max_num_timesteps_in_collection() {
    uint64_t max_num_timesteps = 0;
    for(auto const& simulation : data) {
      max_num_timesteps = std::max(max_num_timesteps, simulation.second.series.size());
    }
    return max_num_timesteps; 
  }

  //i.e. How much size do we need to allocate in the SSBO to store - any - complete simulation series
  uint64_t get_max_num_elements_per_simulation() {
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

  char* get_data_ptr_to_attribute() {

    return data["Temperatur"].serialize_time_series();
  }

};


#endif //FEM_VIS_SSBO_PARSER_UTILS_H_