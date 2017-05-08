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

char *get_cmd_option (char **begin, char **end, const std::string &option)
{
    char **it = std::find (begin, end, option);
    if (it != end && ++it != end)
        return *it;
    return 0;
}

bool cmd_option_exists (char **begin, char **end, const std::string &option)
{
    return std::find (begin, end, option) != end;
}

int main (int argc, char *argv[])
{
    if (argc == 1 ||
        cmd_option_exists (argv, argv + argc, "-h") ||
        !cmd_option_exists (argv, argv + argc, "-f"))
        {

            std::cout << "Usage: " << argv[0] << "<flags> -f <input_file>.nvm" << std::endl <<
                      "INFO: nvm_explorer " << std::endl <<
                      "\t-f: selects .nvm input file" << std::endl <<
                      "\t    (-f flag is required) " << std::endl <<
                      std::endl;
            return 0;
        }

    std::string name_file_nvm = std::string (get_cmd_option (argv, argv + argc, "-f"));

    std::string ext = name_file_nvm.substr (name_file_nvm.size () - 3);
    if (ext.compare ("nvm") != 0)
        {
            std::cout << "please specify a .nvm file as input" << std::endl;
            return 0;
        }

    boost::filesystem::path path_project (name_file_nvm);
    path_project.parent_path ();

    ifstream in (name_file_nvm);
    vector<Camera> vec_camera;
    vector<Point> vec_point;
    vector<Image> vec_image;

    utils::read_nvm (in, vec_camera, vec_point, vec_image);

    //  std::cout << "cameras: " << vec_camera.size() << std::endl;
    //  std::cout << "points: " << vec_point.size() << std::endl;
    // for(std::vector<Point>::iterator it = vec_point.begin(); it != vec_point.end(); ++it) {
    //     if((*it).get_measurements().size() != 0)
    //         std::cout << (*it).get_measurements().size() << std::endl;
    // }
    //  std::cout << "vec_image: " << vec_image.size() << std::endl;

    // Scene scene(vec_camera, vec_point, vec_image);
    // scene.start_rendering();

    return 0;
}