#include <chrono>
#include <lamure/prov/common.h>
#include <lamure/prov/data/DenseCache.h>
#include <lamure/prov/data/DenseStream.h>
#include <lamure/prov/data/SparseCache.h>
#include <lamure/prov/partitioning/SparseOctree.h>

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
    if(argc == 1 || !cmd_option_exists(argv, argv + argc, "-d"))
    {
        cout << "Usage: " << argv[0] << " -d <project>.dense.prov" << endl;
        return -1;
    }

    string name_file_dense = string(get_cmd_option(argv, argv + argc, "-d"));

    if(check_file_extensions(name_file_dense, ".prov"))
    {
        throw std::runtime_error("File format is incompatible");
    }

    std::ifstream in_dense(name_file_dense, std::ios::in | std::ios::binary);
    std::ifstream in_dense_meta(name_file_dense + ".meta", std::ios::in | std::ios::binary);

    lamure::prov::DenseCache cache_dense(in_dense, in_dense_meta);
    
    if(in_dense.is_open())
    {
        auto start = std::chrono::high_resolution_clock::now();
        cache_dense.cache();
        auto end = std::chrono::high_resolution_clock::now();
        printf("Caching dense data took: %f ms\n", std::chrono::duration<double, std::milli>(end - start));
        in_dense.close();
    }

    auto start = std::chrono::high_resolution_clock::now();
    lamure::prov::SparseOctree::Builder builder(cache_dense);

    builder.with_sort(lamure::prov::SparseOctree::PDQ_SORT);
    builder.with_max_depth(10);
    builder.with_min_per_node(8);
    builder.with_cubic_nodes(true);

    lamure::prov::SparseOctree sparse_octree = builder.build();
    auto end = std::chrono::high_resolution_clock::now();
    printf("\nSparse octree creation took: %f ms\n", std::chrono::duration<double, std::milli>(end - start));

    start = std::chrono::high_resolution_clock::now();
    lamure::prov::SparseOctree::save_tree(sparse_octree, "tree.prov");
    end = std::chrono::high_resolution_clock::now();
    printf("\nSparse octree save took: %f ms\n", std::chrono::duration<double, std::milli>(end - start));

    start = std::chrono::high_resolution_clock::now();
    lamure::prov::SparseOctree recovered_sparse_octree = lamure::prov::SparseOctree::load_tree("tree.prov");
    end = std::chrono::high_resolution_clock::now();
    printf("\nSparse octree load took: %f ms\n", std::chrono::duration<double, std::milli>(end - start));

    start = std::chrono::high_resolution_clock::now();
    recovered_sparse_octree.debug_information_loss(cache_dense, cache_dense.get_points().size());
    end = std::chrono::high_resolution_clock::now();
    printf("\nSparse octree information loss debug took: %f ms\n", std::chrono::duration<double, std::milli>(end - start));
    printf("\nTime elapsed per lookup: %f ms\n", std::chrono::duration<double, std::milli>(end - start) / cache_dense.get_points().size());

    start = std::chrono::high_resolution_clock::now();
    recovered_sparse_octree.debug_randomized_lookup(1000000);
    end = std::chrono::high_resolution_clock::now();
    printf("\nSparse octree randomized lookup debug took: %f ms\n", std::chrono::duration<double, std::milli>(end - start));
    printf("\nTime elapsed per lookup: %f ms\n", std::chrono::duration<double, std::milli>(end - start) / 1000000);

    return 0;
}
