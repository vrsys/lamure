#include <iostream>
#include <chrono>
#include <algorithm>
#include <boost/crc.hpp>
#include <boost/sort/spreadsort/spreadsort.hpp>
#include <fstream>
#include <memory>
#include <scm/core.h>
#include <scm/core/math.h>
#include <lamure/vt/pre/Preprocessor.h>

using namespace vt;

char *get_cmd_option(char **begin, char **end, const std::string &option) {
    char **it = std::find(begin, end, option);
    if (it != end && ++it != end)
        return *it;
    return nullptr;
}

bool cmd_option_exists(char **begin, char **end, const std::string &option) {
    return std::find(begin, end, option) != end;
}

int main(int argc, char *argv[]) {
    if (argc == 1 || !cmd_option_exists(argv, argv + argc, "-p"))
    {
        std::cout << "Usage: " << argv[0] << " <flags> -p <image> -c <config>" << std::endl << std::endl;
        return -1;
    }

    if (get_cmd_option(argv, argv + argc, "-p") != nullptr && get_cmd_option(argv, argv + argc, "-c") != nullptr)
    {
        std::string file_raster = std::string(get_cmd_option(argv, argv + argc, "-p"));
        std::string file_config = std::string(get_cmd_option(argv, argv + argc, "-c"));

        Preprocessor preprocessor(file_config.c_str());
        preprocessor.prepare_single_raster(file_raster.c_str());

        preprocessor.prepare_mipmap();
    }

    if (get_cmd_option(argv, argv + argc, "-c") != nullptr)
    {
        std::string file_config = std::string(get_cmd_option(argv, argv + argc, "-c"));

        Preprocessor preprocessor(file_config.c_str());
        preprocessor.prepare_mipmap();
    }

    return 0;
}