#include "lamure/pro/common.h"
#include "lamure/pro/data/DenseData.h"
#include "lamure/pro/data/SparseData.h"
#include <algorithm>
#include <fstream>
#include <iostream>

using namespace std;

char *get_cmd_option(char **begin, char **end, const string &option)
{
    char **it = find(begin, end, option);
    if(it != end && ++it != end)
        return *it;
    return 0;
}

bool cmd_option_exists(char **begin, char **end, const string &option) { return find(begin, end, option) != end; }
bool check_file_extensions(string name_file, const char *pext)
{
    string ext(pext);
    if(name_file.substr(name_file.size() - ext.size()).compare(ext) != 0)
    {
        cout << "Please specify " + ext + " file as input" << endl;
        return true;
    }
    return false;
}

int main(int argc, char *argv[])
{
    if(argc == 1 || !cmd_option_exists(argv, argv + argc, "-s") || !cmd_option_exists(argv, argv + argc, "-d"))
    {
        cout << "Usage: " << argv[0] << "<flags> -s <project>.sparse.pro  -d <project>.dense.pro" << endl << endl;
        return -1;
    }

    string name_file_sparse = string(get_cmd_option(argv, argv + argc, "-s"));
    string name_file_dense = string(get_cmd_option(argv, argv + argc, "-d"));

    if(check_file_extensions(name_file_sparse, "sparse.pro") && check_file_extensions(name_file_dense, "dense.pro"))
    {
        return -1;
    }

    prov::ifstream in_sparse(name_file_sparse);
    prov::ifstream in_dense(name_file_dense);

    prov::SparseData sparseData;
    prov::DenseData denseData;

    if(in_sparse.is_open())
    {
        in_sparse >> sparseData;
        in_sparse.close();
    }

    if(in_dense.is_open())
    {
        in_dense >> denseData;
        in_dense.close();
    }

    return 0;
}