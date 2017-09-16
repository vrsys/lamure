## Lamure provenance library

This library provides convenience classes and methods for operations on provenance data.

### Structure of the library

Library encloses its classes in `prov::` namespace.

Common type definitions / forwarding is done in `common.h`.

#### Subpackages

| Subpackage | Functionality |
|---|---|
| `pro/data` | Data accessing and primitives: sparse data caching, dense data caching & streaming. |
| `pro/partitioning` | Spatial partitioning and data bounding / aggregation. |
| `pro/3dparty` | Holds all external dependencies. |

#### External dependencies

| Name | File | Reason for usage | Source page |
|---|---|---|---|
| easyexif | exif.h; exif.cpp | ISO-compliant C++ EXIF parsing | [GitHub](https://github.com/mayanklahiri/easyexif) |
| pdqsort | pdqsort.h | Sorting variant | [GitHub](https://github.com/orlp/pdqsort) |
| tinyply | tinyply.h; tinyply.cpp | `*.ply` format parsing | [GitHub](https://github.com/ddiakopoulos/tinyply) |


### Quick start

#### Sparse caching

    /*
    * Preprocess your sparse point cloude data with @pypro python package.
    * Make sure to have sparse.prov and sparse.prov.meta files in the same directory.
    */

    // Provide string path to the dense.prov file
    string name_file_sparse = ...

    // Initialize binary input streams
    prov::ifstream in_sparse(name_file_sparse, std::ios::in | std::ios::binary);
    prov::ifstream in_sparse_meta(name_file_sparse + ".meta", std::ios::in | std::ios::binary);

    // Initialize sparse cache (lightweight operation)
    prov::SparseCache cache_sparse(in_sparse, in_sparse_meta);

    // Load sparse point cloud data to the dense cache (RAM-heavy operation, takes some time)
    cache_sparse.cache();

    // Close input streams
    in_sparse.close();
    in_sparse_meta.close();

    // Access sparse point cloud data
    cache_sparse->...

#### Dense caching

    /*
    * Preprocess your dense point cloude data with @pypro python package.
    * Make sure to have dense.prov and dense.prov.meta files in the same directory.
    */

    // Provide string path to the dense.prov file
    string name_file_dense = ...

    // Initialize binary input streams
    prov::ifstream in_dense(name_file_dense, std::ios::in | std::ios::binary);
    prov::ifstream in_dense_meta(name_file_dense + ".meta", std::ios::in | std::ios::binary);

    // Initialize dense cache (lightweight operation)
    prov::DenseCache cache_dense(in_dense, in_dense_meta);

    // Load dense point cloud data to the dense cache (RAM-heavy operation, takes some time)
    cache_sparse.cache();

    // Close input streams
    in_dense.close();
    in_dense_meta.close();

    // Access dense point cloud data
    cache_dense->...

#### Sparse octree creation from dense cache

    /*
    * Preprocess your dense point cloude data with @pypro python package.
    * Make sure to have dense.prov and dense.prov.meta files in the same directory.
    * Initialize a @DenseCache and load dense point cloud data.
    */

    // Initialize a sparse octree builder
    prov::SparseOctree::Builder builder;

    // Define octree source to be the initialized dense cache
    builder.from(cache_dense);

    // Choose sorting (optional)
    builder.with_sort(prov::SparseOctree::PDQ_SORT);

    // Choose maximum depth (optional)
    builder.with_max_depth(10);

    // Choose minimal allowed number of points per node (optional)
    builder.with_min_per_node(8);

    // Build the octree (RAM-heavy operation, takes some time)
    prov::SparseOctree sparse_octree = builder.build();

    // Perform lookups
    sparse_octree.lookup_node_at_position(...);

    // Serialize octree to file (lightweight operation)
    prov::SparseOctree::save_tree(sparse_octree, "...");

#### Sparse octree recovery from file

    /*
    * Preprocess your dense point cloude data with @pypro python package.
    * Make sure to have dense.prov and dense.prov.meta files in the same directory.
    * Initialize a @DenseCache and load dense point cloud data.
    * Create a sparse octree and serialize it to a file.
    */

    // Load octree from file (lightweight operation)
    prov::SparseOctree sparse_octree = prov::SparseOctree::load_tree("...");

    // Perform lookups
    sparse_octree.lookup_node_at_position(...);

#### Dense streaming

    /*
    * Preprocess your dense point cloude data with @pypro python package.
    */

    // Provide string path to the dense.prov file
    string name_file_dense = ...

    // Initialize binary input streams
    prov::ifstream in_dense(name_file_dense, std::ios::in | std::ios::binary);

    // Initialize dense stream (lightweight operation)
    prov::DenseStream stream_dense = prov::DenseStream(in_dense);

    // Access dense point at implicitly defined index
    stream_dense.access_at_implicit(...);

    // Access dense points at implicitly defined index range
    std::vector<prov::DensePoint> vector = stream_dense.access_at_implicit_range(..., ...);

    in_dense.close();