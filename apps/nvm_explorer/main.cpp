#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <lamure/ren/model_database.h>
#include "utils.h"
// #include "scene.h"

#define VERBOSE
#define DEFAULT_PRECISION 15

using namespace utils;

char *get_cmd_option(char **begin, char **end, const std::string &option) {
    char **it = std::find(begin, end, option);
    if (it != end && ++it != end)
        return *it;
    return 0;
}

bool cmd_option_exists(char **begin, char **end, const std::string &option) {
    return std::find(begin, end, option) != end;
}

int main(int argc, char *argv[]) {
    if (argc == 1 ||
        cmd_option_exists(argv, argv + argc, "-h") ||
        !cmd_option_exists(argv, argv + argc, "-f")) {

        std::cout << "Usage: " << argv[0] << "<flags> -f <input_file>.nvm" << std::endl <<
                  "INFO: nvm_explorer " << std::endl <<
                  "\t-f: selects .nvm input file" << std::endl <<
                  "\t    (-f flag is required) " << std::endl <<
                  std::endl;
        return 0;
    }

    std::string nvm_filename = std::string(get_cmd_option(argv, argv + argc, "-f"));

    std::string ext = nvm_filename.substr(nvm_filename.size() - 3);
    if (ext.compare("nvm") != 0) {
        std::cout << "please specify a .nvm file as input" << std::endl;
        return 0;
    }

    ifstream in(nvm_filename);
    vector<camera>        camera_vec;    //camera (input/ouput)
    vector<point>        point_vec;     //3D point(input/output)
    vector<image>        images;   //measurment/projection vector

    utils::read_nvm(in, camera_vec, point_vec, images);
    std::cout << "cameras: " << camera_vec.size() << std::endl;
    std::cout << "points: " << point_vec.size() << std::endl;
    // for(std::vector<point>::iterator it = point_vec.begin(); it != point_vec.end(); ++it) {
    //     if((*it).get_measurements().size() != 0)
    //         std::cout << (*it).get_measurements().size() << std::endl;
    // }
    std::cout << "images: " << images.size() << std::endl;

    // Scene scene(camera_vec, point_vec, images);
    // scene.start_rendering();

    return 0;
}



