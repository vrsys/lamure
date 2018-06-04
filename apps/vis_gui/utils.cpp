//
// Created by moqe3167 on 30/05/18.
//

#include "utils.h"

namespace vis_utils
{
    char *get_cmd_option(char **begin, char **end, const std::string &option) {
        char **it = std::find(begin, end, option);
        if (it != end && ++it != end) {
            return *it;
        }
        return 0;
    }

    bool cmd_option_exists(char **begin, char **end, const std::string &option) {
        return std::find(begin, end, option) != end;
    }

    scm::math::mat4f load_matrix(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Unable to open transformation file: \""
                      << filename << "\"\n";
            return scm::math::mat4f::identity();
        }
        scm::math::mat4f mat = scm::math::mat4f::identity();
        std::string matrix_values_string;
        std::getline(file, matrix_values_string);
        std::stringstream sstr(matrix_values_string);
        for (int i = 0; i < 16; ++i)
            sstr >> mat[i];
        file.close();
        return scm::math::transpose(mat);
    }

    std::string const strip_whitespace(std::string const& in_string) {
        return boost::regex_replace(in_string, boost::regex("^ +| +$|( ) +"), "$1");

    }

//checks for prefix AND removes it (+ whitespace) if it is found;
//returns true, if prefix was found; else false
    bool parse_prefix(std::string &in_string, std::string const &prefix) {

        uint32_t num_prefix_characters = prefix.size();

        bool prefix_found
                = (!(in_string.size() < num_prefix_characters )
                   && strncmp(in_string.c_str(), prefix.c_str(), num_prefix_characters ) == 0);

        if( prefix_found ) {
            in_string = in_string.substr(num_prefix_characters);
            in_string = strip_whitespace(in_string);
        }

        return prefix_found;
    }

    void resolve_relative_path(std::string& base_path, std::string& relative_path) {

        while(parse_prefix(relative_path, "../")) {
            size_t slash_position = base_path.find_last_of("/", base_path.size()-2);
            base_path = base_path.substr(0, slash_position);
        }
    }
}