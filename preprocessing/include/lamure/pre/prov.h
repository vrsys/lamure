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
class PREPROCESSING_DLL prov
{
  public:
    prov() 
    :
    mean_absolute_deviation_(0.f),
    standard_deviation_(0.f),
    coefficient_of_variation_(0.f),
    value_3_(0.f),
    value_4_(0.f),
    value_5_(0.f),
    value_6_(0.f) {};


  	float mean_absolute_deviation_;
    float standard_deviation_;
    float coefficient_of_variation_;
    float value_3_;
    float value_4_;
    float value_5_;
    float value_6_;

    static void write_json(std::string const& file_name) {
    	std::ofstream out_stream;
        out_stream.open(file_name, std::ios::out);
        std::string filestr;
        std::stringstream ss(filestr);

        int32_t num_values = 7;

        ss << "[";
        for (int32_t i = 0; i < num_values; ++i) {
          ss << "{\n";
		  ss << "\t\"type\": \"float\",\n";
		  ss << "\t\"visualization\": \"color\"\n";
		  ss << "}";
		  if (i != num_values-1) {
            ss << ",";
		  }
        }
        ss << "]";

        out_stream << ss.rdbuf();
        out_stream.close();
    }

};

using prov_vector = std::vector<prov>;
using shared_prov = std::shared_ptr<prov>;
using shared_prov_vector = std::shared_ptr<prov_vector>;

}
} // namespace lamure

#endif // PRE_PROV_H_
