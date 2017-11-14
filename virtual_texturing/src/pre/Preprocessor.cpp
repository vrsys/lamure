#include <lamure/vt/pre/Preprocessor.h>

namespace vt {

Preprocessor::Preprocessor(const char *path_config) {
    config = new CSimpleIniA(true, false, false);

    if (config->LoadFile(path_config) < 0)
    {
        throw std::runtime_error("Configuration file parsing error");
    }

    boost::filesystem::path path_texture(config->GetValue(Config::TEXTURE_MANAGEMENT, Config::FILE_PPM, Config::UNDEF));
    boost::filesystem::path dir_texture = path_texture.parent_path();
    Magick::InitializeMagick(dir_texture.c_str());

    if (atoi(config->GetValue(Config::DEBUG, Config::VERBOSE, Config::UNDEF)) == 1)
    {
        std::cout << std::endl;
        std::cout << "Preprocessor initialized" << std::endl;
        std::cout << "Operating on config file: " << path_texture.filename() << std::endl;
        std::cout << "Working directory: " << dir_texture.c_str() << std::endl;
        std::cout << std::endl;
    }
}

bool Preprocessor::prepare_single_raster(const char *name_raster) {
    if (atoi(config->GetValue(Config::DEBUG, Config::VERBOSE, Config::UNDEF)) == 1)
    {
        std::cout << std::endl;
        std::cout << "Attempting to convert image to bitmap in-core..." << std::endl;
    }

    Config::FORMAT_TEXTURE format_texture = Config::which_texture_format(config->GetValue(Config::TEXTURE_MANAGEMENT, Config::TEXTURE_FORMAT, Config::UNDEF));

    Magick::Image image;
    try
    {
        image.read(name_raster);
        Magick::Geometry geometry = image.size();
        size_t side = (size_t) std::pow(2, std::floor(std::log(std::min(geometry.width(), geometry.height())) / std::log(2)));
        geometry.aspect(false);
        geometry.width(side);
        geometry.height(side);
        image.crop(geometry);

        // TODO: convert image to relevant depth & colorspace
        switch (format_texture)
        {
        case Config::FORMAT_TEXTURE::RGBA8:
        case Config::FORMAT_TEXTURE::RGB6:
        case Config::FORMAT_TEXTURE::R2:

            image.depth(8);
            break;
        }

        image.write(config->GetValue(Config::TEXTURE_MANAGEMENT, Config::FILE_PPM, Config::UNDEF));
    }
    catch (Magick::Exception &error_)
    {
        std::cout << "Caught exception: " << error_.what() << std::endl;
        return false;
    };

    if (atoi(config->GetValue(Config::DEBUG, Config::VERBOSE, Config::UNDEF)) == 1)
    {
        std::cout << "Attempt successful" << std::endl;
        std::cout << std::endl;
    }

    return true;
}

bool Preprocessor::prepare_mipmap() {
    auto tile_size = (size_t) atoi(config->GetValue(Config::TEXTURE_MANAGEMENT, Config::TILE_SIZE, Config::UNDEF));
    if ((tile_size & (tile_size - 1)) != 0)
    {
        throw std::runtime_error("Tile size is not a power of 2");
    }

    uint32_t count_threads = 0;
    count_threads = atoi(config->GetValue(Config::TEXTURE_MANAGEMENT, Config::OPT_RUN_IN_PARALLEL, Config::UNDEF)) != 1 ? 1 : thread::hardware_concurrency();

    size_t dim_x = 0, dim_y = 0;

    std::ifstream input;
    input.open(config->GetValue(Config::TEXTURE_MANAGEMENT, Config::FILE_PPM, Config::UNDEF), std::ifstream::in | std::ifstream::binary);
    read_ppm_header(input, dim_x, dim_y);
    input.close();

    bool verbose = atoi(config->GetValue(Config::DEBUG, Config::VERBOSE, Config::UNDEF)) == 1;

    uint32_t tree_depth = QuadTree::calculate_depth(dim_x, tile_size);
    size_t first_node_leaf = QuadTree::get_first_node_id_of_depth(tree_depth);
    size_t last_node_leaf = QuadTree::get_first_node_id_of_depth(tree_depth) + QuadTree::get_length_of_depth(tree_depth) - 1;

    if (verbose)
    {
        std::cout << std::endl;
        std::cout << "Bitmap dimensions: " << dim_x << " x " << dim_y << std::endl;
        std::cout << "Tile size: " << tile_size << " x " << tile_size << std::endl;
        std::cout << "Tree depth: " << tree_depth << std::endl;
        std::cout << std::endl;

        std::cout << std::endl;
        std::cout << "Creating " << QuadTree::get_length_of_depth(tree_depth) << " leaf level tiles in " << count_threads << " threads" << std::endl;
    }

    size_t nodes_per_thread = (last_node_leaf - first_node_leaf + 1) / count_threads;

    std::vector<std::thread> thread_pool;

    // TODO: implement in-core tile row optimization
    for (uint32_t _thread_id = 0; _thread_id < count_threads; _thread_id++)
    {
        size_t node_start = first_node_leaf + _thread_id * nodes_per_thread;
        size_t node_end = (_thread_id != count_threads - 1) ? first_node_leaf + (_thread_id + 1) * nodes_per_thread : last_node_leaf;
        thread_pool.emplace_back(
            [=]()
            { write_tile_range_at_depth(_thread_id, tree_depth, node_start, node_end); });
    }
    for (auto &_thread : thread_pool)
    {
        _thread.join();
    }

    thread_pool.clear();

    if (verbose)
    {
        std::cout << "Leaf level ready" << std::endl;
        std::cout << std::endl;
    }

    for (uint32_t _depth = tree_depth - 1; _depth != static_cast<unsigned>(-1); _depth--)
    {
        if (verbose)
        {
            std::cout << std::endl;
            std::cout << "Stitching " << QuadTree::get_length_of_depth(_depth) << " tiles of depth " << _depth << std::endl;
        }

        first_node_leaf = QuadTree::get_first_node_id_of_depth(_depth);
        last_node_leaf = QuadTree::get_first_node_id_of_depth(_depth) + QuadTree::get_length_of_depth(_depth) - 1;

        nodes_per_thread = (last_node_leaf - first_node_leaf + 1) / count_threads;

        if (nodes_per_thread > 0)
        {
            for (uint32_t _thread_id = 0; _thread_id < count_threads; _thread_id++)
            {
                size_t node_start = first_node_leaf + _thread_id * nodes_per_thread;
                size_t node_end = (_thread_id != count_threads - 1) ? first_node_leaf + (_thread_id + 1) * nodes_per_thread : last_node_leaf;
                thread_pool.emplace_back([=]()
                                         { stitch_tile_range(_thread_id, node_start, node_end); });
            }
            for (auto &_thread : thread_pool)
            {
                _thread.join();
            }
            thread_pool.clear();
        }
        else
        {
            stitch_tile_range(0, first_node_leaf, last_node_leaf);
        }
    }

    if (verbose)
    {
        std::cout << "Hierarchy ready" << std::endl;
        std::cout << std::endl;

        std::cout << std::endl;
        std::cout << "Writing mipmap synchronously" << std::endl;
    }

    const char *file_mipmap = config->GetValue(Config::TEXTURE_MANAGEMENT, Config::FILE_MIPMAP, Config::UNDEF);
    std::ofstream output;
    output.open(file_mipmap, std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);

    auto *buf_tile = new char[tile_size * tile_size * 3];

    for (uint32_t _depth = 0; _depth <= tree_depth; _depth++)
    {
        first_node_leaf = QuadTree::get_first_node_id_of_depth(_depth);
        last_node_leaf = QuadTree::get_first_node_id_of_depth(_depth) + QuadTree::get_length_of_depth(_depth) - 1;

        for (size_t _id = first_node_leaf; _id <= last_node_leaf; _id++)
        {
            std::ifstream input_tile;
            input_tile.open("id_" + std::to_string(_id) + ".ppm", std::ifstream::in | std::ifstream::binary);

            read_ppm_header(input_tile, dim_x, dim_y);
            input_tile.read(&buf_tile[0], tile_size * tile_size * 3);

            input_tile.close();

            output.write(&buf_tile[0], tile_size * tile_size * 3);

            if (atoi(config->GetValue(Config::DEBUG, Config::KEEP_INTERMEDIATE_DATA, Config::UNDEF)) != 1)
            {
                std::remove(("id_" + std::to_string(_id) + ".ppm").c_str());
            }
        }
    }

    output.close();

    if (verbose)
    {
        std::cout << "Mipmap done" << std::endl;
        std::cout << std::endl;
    }

    return false;
}

void Preprocessor::read_ppm_header(std::ifstream &_ifs, size_t &_dim_x, size_t &_dim_y) {
    auto *buf_file_version = new char[3];
    _ifs.getline(&buf_file_version[0], 3, '\x0A');

    if (strcmp(buf_file_version, "P6") != 0)
    {
        throw std::runtime_error("PPM file format not equal to P6");
    }

    auto *buf_dim_x = new char[8];
    auto *buf_dim_y = new char[8];
    auto *buf_color_depth = new char[8];

    _ifs.getline(&buf_dim_x[0], 8, '\x20');
    _ifs.getline(&buf_dim_y[0], 8, '\x0A');
    _ifs.getline(&buf_color_depth[0], 8, '\x0A');

    _dim_x = (size_t) atol(buf_dim_x);
    _dim_y = (size_t) atol(buf_dim_y);
    auto color_depth = (short) atoi(buf_color_depth);

    if (color_depth != 255)
    {
        throw std::runtime_error("PPM color depth not equal to 255");
    }

    if (_dim_x != _dim_y || (_dim_x & (_dim_x - 1)) != 0)
    {
        throw std::runtime_error("PPM dimensions are not equal or not a power of 2");
    }
}

// TODO: implement in-core tile row optimization
void Preprocessor::write_tile_range_at_depth(uint32_t _thread_id, uint32_t _depth, size_t _node_start, size_t _node_end) {
    std::ifstream _ifs;
    _ifs.open(config->GetValue(Config::TEXTURE_MANAGEMENT, Config::FILE_PPM, Config::UNDEF),
              std::ifstream::in | std::ifstream::binary);

    size_t dim_x = 0, dim_y = 0;

    read_ppm_header(_ifs, dim_x, dim_y);

    streamoff _offset_header = _ifs.tellg();

    auto _tile_size = (size_t) atoi(config->GetValue(Config::TEXTURE_MANAGEMENT, Config::TILE_SIZE, Config::UNDEF));
    if ((_tile_size & (_tile_size - 1)) != 0)
    {
        throw std::runtime_error("Tile size is not a power of 2");
    }

    auto *buf_tile = new char[_tile_size * _tile_size * 3];
    std::string buf_header =
        "P6\x0A" + std::to_string(_tile_size) + "\x20" + std::to_string(_tile_size) + "\x0A" + "255" + "\x0A";

    size_t _node_first = QuadTree::get_first_node_id_of_depth(_depth);
    size_t _tiles_per_row = QuadTree::get_tiles_per_row(_depth);

    size_t _range = _node_end - _node_start;

    bool verbose = atoi(config->GetValue(Config::DEBUG, Config::VERBOSE, Config::UNDEF)) == 1;

    auto start = std::chrono::high_resolution_clock::now();
    double average = 0.0;

    for (size_t _id = _node_start; _id <= _node_end; _id++)
    {
        if (verbose)
        {
            start = std::chrono::high_resolution_clock::now();
        }

        uint_fast64_t skip_cols, skip_rows;
        morton2D_64_decode((uint_fast64_t) (_id - _node_first), skip_cols, skip_rows);

        _ifs.seekg(_offset_header);
        _ifs.ignore(skip_rows * _tiles_per_row * _tile_size * _tile_size * 3);

        for (uint32_t _row = 0; _row < _tile_size; _row++)
        {
            _ifs.ignore(skip_cols * _tile_size * 3);
            _ifs.read(&buf_tile[_row * _tile_size * 3], _tile_size * 3);
            _ifs.ignore((_tiles_per_row - skip_cols - 1) * _tile_size * 3);
        }

        std::ofstream output;
        output.open("id_" + std::to_string(_id) + ".ppm", std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
        output.write(&buf_header.c_str()[0], buf_header.size());
        output.write(&buf_tile[0], _tile_size * _tile_size * 3);
        output.close();

        if (verbose)
        {
            if (_id - _node_start != 0)
            {
                average += (std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start).count() - average) / (_id - _node_start);
            }
            else
            {
                average = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start).count();
            }
        }

        if (verbose)
        {
            if ((_id - _node_start) % (_range / 10 + 1) == 0)
            {
                std::cout << "Thread #" << _thread_id << " progress: " << (_id - _node_start) * 100 / _range << " %" << std::endl;
            }
        }
    }

    _ifs.close();

    if (verbose)
    {
        std::cout << "Thread #" << _thread_id << " done" << std::endl;
        std::cout << "Average tile creation time: " << average << " msec" << std::endl;
    }
}

void Preprocessor::stitch_tile_range(uint32_t _thread_id, size_t _node_start, size_t _node_end) {
    auto _tile_size = (size_t) atoi(config->GetValue(Config::TEXTURE_MANAGEMENT, Config::TILE_SIZE, Config::UNDEF));
    if ((_tile_size & (_tile_size - 1)) != 0)
    {
        throw std::runtime_error("Tile size is not a power of 2");
    }

    bool verbose = atoi(config->GetValue(Config::DEBUG, Config::VERBOSE, Config::UNDEF)) == 1;

    size_t _range = _node_end - _node_start + 1;

    auto start = std::chrono::high_resolution_clock::now();
    double average = 0.0;

    std::array<Magick::Image, 4> children_imgs;

    for (size_t i = 0; i < 4; i++)
    {
        Magick::Image _child;
        children_imgs[i] = _child;
    }

    for (size_t _id = _node_start; _id <= _node_end; _id++)
    {
        if (verbose)
        {
            start = std::chrono::high_resolution_clock::now();
        }

        for (size_t i = 0; i < 4; i++)
        {
            children_imgs[i].read("id_" + std::to_string(QuadTree::get_child_id(_id, i)) + ".ppm");
        }

        write_stitched_tile(_id, _tile_size, children_imgs);

        if (verbose)
        {
            if (_id - _node_start != 0)
            {
                average += (std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start).count() - average) / (_id - _node_start);
            }
            else
            {
                average = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start).count();
            }
        }

        if (verbose)
        {
            if ((_id - _node_start) % (_range / 10 + 1) == 0)
            {
                std::cout << "Thread #" << _thread_id << " progress: " << (_id - _node_start) * 100 / _range << " %" << std::endl;
            }
        }
    }

    if (verbose)
    {
        std::cout << "Thread #" << _thread_id << " done" << std::endl;
        std::cout << "Average tile stitch time: " << average << " msec" << std::endl;
    }
}

void Preprocessor::write_stitched_tile(size_t _id, size_t _tile_size, std::array<Magick::Image, 4> &_child_imgs) {
    Magick::Montage montage_settings;
    montage_settings.tile(Magick::Geometry(2, 2));
    montage_settings.geometry(Magick::Geometry(_tile_size, _tile_size));

    std::list<Magick::Image> montage_list;
    Magick::montageImages(&montage_list, _child_imgs.begin(), _child_imgs.end(), montage_settings);
    Magick::writeImages(montage_list.begin(), montage_list.end(), "id_" + std::to_string(_id) + ".ppm");

    Magick::Image image;
    image.read("id_" + std::to_string(_id) + ".ppm");
    Magick::Geometry geometry = image.size();
    geometry.width(_tile_size);
    geometry.height(_tile_size);
    image.scale(geometry);
    image.depth(8);
    image.write("id_" + std::to_string(_id) + ".ppm");
}

}