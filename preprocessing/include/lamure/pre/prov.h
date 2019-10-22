// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_PROV_H_
#define PRE_PROV_H_

#include <lamure/pre/platform.h>
#include <lamure/types.h>

#include <fstream>
#include <memory>
#include <vector>
#include <sstream>

namespace lamure
{
namespace pre
{
static const uint32_t num_prov_values_ = 4;

class PREPROCESSING_DLL prov_data
{
  public:

    prov_data() 
    :
    mean_absolute_deviation_(0.f),
    standard_deviation_(0.f),
    coefficient_of_variation_(0.f) {
        for (uint32_t i = 0; i < num_prov_values_; ++i) {
            values_[i] = 0.f;
        }
    };


    float mean_absolute_deviation_;
    float standard_deviation_;
    float coefficient_of_variation_;
    float values_[num_prov_values_];

    static void write_json(std::string const& file_name) {
    	std::ofstream out_stream;
        out_stream.open(file_name, std::ios::out);
        std::string filestr;
        std::stringstream ss(filestr);

        ss << "[";
        for (uint32_t i = 0; i < num_prov_values_+3; ++i) {
          ss << "{\n";
		  ss << "\t\"type\": \"float\",\n";
		  ss << "\t\"visualization\": \"color\"\n";
		  ss << "}";
		  if (i != (num_prov_values_+3)-1) {
            ss << ",";
		  }
        }
        ss << "]";

        out_stream << ss.rdbuf();
        out_stream.close();
    }

};

using prov_vector = std::vector<prov_data>;
using shared_prov = std::shared_ptr<prov_data>;
using shared_prov_vector = std::shared_ptr<prov_vector>;

}
} // namespace lamure

#endif // PRE_PROV_H_
