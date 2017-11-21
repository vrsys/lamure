#include <lamure/vt/pre/Preprocessor.h>

namespace vt
{

Preprocessor::Preprocessor(const char *path_config)
{
    config = new CSimpleIniA(true, false, false);

    if (config->LoadFile(path_config) < 0)
    {
        throw std::runtime_error("Configuration file parsing error");
    }

    Config::FORMAT_TEXTURE format_texture = Config::which_texture_format(config->GetValue(Config::TEXTURE_MANAGEMENT, Config::TEXTURE_FORMAT, Config::UNDEF));

    boost::filesystem::path path_texture;
    switch (format_texture)
    {
        case Config::FORMAT_TEXTURE::RGBA8:

            path_texture = boost::filesystem::path(std::string(config->GetValue(Config::TEXTURE_MANAGEMENT, Config::NAME_TEXTURE, Config::UNDEF)) + ".rgba");
            break;
        case Config::FORMAT_TEXTURE::RGB8:

            path_texture = boost::filesystem::path(std::string(config->GetValue(Config::TEXTURE_MANAGEMENT, Config::NAME_TEXTURE, Config::UNDEF)) + ".rgb");
            break;
        case Config::FORMAT_TEXTURE::R8:

            path_texture = boost::filesystem::path(std::string(config->GetValue(Config::TEXTURE_MANAGEMENT, Config::NAME_TEXTURE, Config::UNDEF)) + ".gray");
            break;
    }
    boost::filesystem::path dir_texture = path_texture.parent_path();
    Magick::InitializeMagick(dir_texture.c_str());

    if (atoi(config->GetValue(Config::DEBUG, Config::VERBOSE, Config::UNDEF)) == 1)
    {
        std::cout << std::endl;
        std::cout << "Preprocessor initialized" << std::endl;
        std::cout << "Operating on config file: " << path_config << std::endl;
        std::cout << "Working directory: " << dir_texture.c_str() << std::endl;
        std::cout << std::endl;
    }
}

bool Preprocessor::prepare_single_raster(const char *name_raster)
{
    if (atoi(config->GetValue(Config::DEBUG, Config::VERBOSE, Config::UNDEF)) == 1)
    {
        std::cout << std::endl;
        std::cout << "Attempting to convert full image in-core..." << std::endl;
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

        switch (format_texture)
        {
            case Config::FORMAT_TEXTURE::RGBA8:

                image.type(Magick::TrueColorMatteType);
                image.depth(8);
                image.write(std::string(config->GetValue(Config::TEXTURE_MANAGEMENT, Config::NAME_TEXTURE, Config::UNDEF)) + ".rgba");
                break;
            case Config::FORMAT_TEXTURE::RGB8:

                image.type(Magick::TrueColorType);
                image.depth(8);
                image.write(std::string(config->GetValue(Config::TEXTURE_MANAGEMENT, Config::NAME_TEXTURE, Config::UNDEF)) + ".rgb");
                break;
            case Config::FORMAT_TEXTURE::R8:

                image.type(Magick::GrayscaleType);
                image.depth(8);
                image.write(std::string(config->GetValue(Config::TEXTURE_MANAGEMENT, Config::NAME_TEXTURE, Config::UNDEF)) + ".gray");
                break;
        }
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

bool Preprocessor::prepare_mipmap()
{
    auto tile_size = (size_t) atoi(config->GetValue(Config::TEXTURE_MANAGEMENT, Config::TILE_SIZE, Config::UNDEF));
    if ((tile_size & (tile_size - 1)) != 0)
    {
        throw std::runtime_error("Tile size is not a power of 2");
    }

    uint32_t count_threads = 0;
    count_threads = atoi(config->GetValue(Config::TEXTURE_MANAGEMENT, Config::OPT_RUN_IN_PARALLEL, Config::UNDEF)) != 1 ? 1 : thread::hardware_concurrency();
    Config::FORMAT_TEXTURE format_texture = Config::which_texture_format(config->GetValue(Config::TEXTURE_MANAGEMENT, Config::TEXTURE_FORMAT, Config::UNDEF));

    size_t dim_x = 0, dim_y = 0;

    std::ifstream input;
    switch (format_texture)
    {
        case Config::FORMAT_TEXTURE::RGBA8:

            input.open(std::string(config->GetValue(Config::TEXTURE_MANAGEMENT, Config::NAME_TEXTURE, Config::UNDEF)) + ".rgba", std::ifstream::in | std::ifstream::binary);
            break;
        case Config::FORMAT_TEXTURE::RGB8:

            input.open(std::string(config->GetValue(Config::TEXTURE_MANAGEMENT, Config::NAME_TEXTURE, Config::UNDEF)) + ".rgb", std::ifstream::in | std::ifstream::binary);
            break;
        case Config::FORMAT_TEXTURE::R8:

            input.open(std::string(config->GetValue(Config::TEXTURE_MANAGEMENT, Config::NAME_TEXTURE, Config::UNDEF)) + ".gray", std::ifstream::in | std::ifstream::binary);
            break;
    }
    read_dimensions(input, dim_x, dim_y);
    input.close();

    bool verbose = atoi(config->GetValue(Config::DEBUG, Config::VERBOSE, Config::UNDEF)) == 1;

    uint32_t tree_depth = QuadTree::calculate_depth(dim_x, tile_size);

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

    std::vector<std::thread> thread_pool;

    for (uint32_t _thread_id = 0; _thread_id < count_threads; _thread_id++)
    {
        if (atoi(config->GetValue(Config::TEXTURE_MANAGEMENT, Config::OPT_TILE_ROW_IN_CORE, Config::UNDEF)) == 1)
        {
            thread_pool.emplace_back([=]()
                                     { extract_leaf_tile_rows(_thread_id); });
        }
        else
        {
            thread_pool.emplace_back([=]()
                                     { extract_leaf_tile_range(_thread_id); });
        }
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

        size_t first_node_depth = QuadTree::get_first_node_id_of_depth(_depth);
        size_t last_node_depth = QuadTree::get_first_node_id_of_depth(_depth) + QuadTree::get_length_of_depth(_depth) - 1;

        size_t nodes_per_thread = (last_node_depth - first_node_depth + 1) / count_threads;

        if (nodes_per_thread > 0)
        {
            for (uint32_t _thread_id = 0; _thread_id < count_threads; _thread_id++)
            {
                size_t node_start = first_node_depth + _thread_id * nodes_per_thread;
                size_t node_end = (_thread_id != count_threads - 1) ? first_node_depth + (_thread_id + 1) * nodes_per_thread : last_node_depth;
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
            stitch_tile_range(0, first_node_depth, last_node_depth);
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

    auto *buf_tile = new char[tile_size * tile_size * get_byte_stride(format_texture)];

    for (uint32_t _depth = 0; _depth <= tree_depth; _depth++)
    {
        size_t first_node_depth = QuadTree::get_first_node_id_of_depth(_depth);
        size_t last_node_depth = QuadTree::get_first_node_id_of_depth(_depth) + QuadTree::get_length_of_depth(_depth) - 1;

        for (size_t _id = first_node_depth; _id <= last_node_depth; _id++)
        {
            std::ifstream input_tile;
            switch (format_texture)
            {
                case Config::FORMAT_TEXTURE::RGBA8:

                    input_tile.open("id_" + std::to_string(_id) + ".rgba", std::ifstream::in | std::ifstream::binary);
                    break;
                case Config::FORMAT_TEXTURE::RGB8:

                    input_tile.open("id_" + std::to_string(_id) + ".rgb", std::ifstream::in | std::ifstream::binary);
                    break;
                case Config::FORMAT_TEXTURE::R8:

                    input_tile.open("id_" + std::to_string(_id) + ".gray", std::ifstream::in | std::ifstream::binary);
                    break;
            }

            read_dimensions(input_tile, dim_x, dim_y);
            input_tile.read(&buf_tile[0], tile_size * tile_size * get_byte_stride(format_texture));

            input_tile.close();

            output.write(&buf_tile[0], tile_size * tile_size * get_byte_stride(format_texture));

            if (atoi(config->GetValue(Config::DEBUG, Config::KEEP_INTERMEDIATE_DATA, Config::UNDEF)) != 1)
            {
                switch (format_texture)
                {
                    case Config::FORMAT_TEXTURE::RGBA8:

                        std::remove(("id_" + std::to_string(_id) + ".rgba").c_str());
                        break;
                    case Config::FORMAT_TEXTURE::RGB8:

                        std::remove(("id_" + std::to_string(_id) + ".rgb").c_str());
                        break;
                    case Config::FORMAT_TEXTURE::R8:

                        std::remove(("id_" + std::to_string(_id) + ".gray").c_str());
                        break;
                }
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

void Preprocessor::extract_leaf_tile_range(uint32_t _thread_id)
{
    Config::FORMAT_TEXTURE format_texture = Config::which_texture_format(config->GetValue(Config::TEXTURE_MANAGEMENT, Config::TEXTURE_FORMAT, Config::UNDEF));
    std::ifstream ifs;

    switch (format_texture)
    {
        case Config::FORMAT_TEXTURE::RGBA8:

            ifs.open(std::string(config->GetValue(Config::TEXTURE_MANAGEMENT, Config::NAME_TEXTURE, Config::UNDEF)) + ".rgba", std::ifstream::in | std::ifstream::binary);
            break;
        case Config::FORMAT_TEXTURE::RGB8:

            ifs.open(std::string(config->GetValue(Config::TEXTURE_MANAGEMENT, Config::NAME_TEXTURE, Config::UNDEF)) + ".rgb", std::ifstream::in | std::ifstream::binary);
            break;
        case Config::FORMAT_TEXTURE::R8:

            ifs.open(std::string(config->GetValue(Config::TEXTURE_MANAGEMENT, Config::NAME_TEXTURE, Config::UNDEF)) + ".gray", std::ifstream::in | std::ifstream::binary);
            break;
    }

    size_t dim_x = 0, dim_y = 0;

    read_dimensions(ifs, dim_x, dim_y);

    streamoff _offset_header = ifs.tellg();

    auto _tile_size = (size_t) atoi(config->GetValue(Config::TEXTURE_MANAGEMENT, Config::TILE_SIZE, Config::UNDEF));
    if ((_tile_size & (_tile_size - 1)) != 0)
    {
        throw std::runtime_error("Tile size is not a power of 2");
    }

    uint32_t _tree_depth = QuadTree::calculate_depth(dim_x, _tile_size);

    auto *buf_tile = new char[_tile_size * _tile_size * get_byte_stride(format_texture)];

    size_t _node_first_leaf = QuadTree::get_first_node_id_of_depth(_tree_depth);
    size_t _node_last_leaf = QuadTree::get_first_node_id_of_depth(_tree_depth) + QuadTree::get_length_of_depth(_tree_depth) - 1;
    size_t _tiles_per_row = QuadTree::get_tiles_per_row(_tree_depth);

    size_t _nodes_per_thread = (_node_last_leaf - _node_first_leaf + 1) / thread::hardware_concurrency();

    size_t _node_start = _node_first_leaf + _thread_id * _nodes_per_thread;
    size_t _node_end = (_thread_id != thread::hardware_concurrency() - 1) ? _node_first_leaf + (_thread_id + 1) * _nodes_per_thread : _node_last_leaf;

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
        morton2D_64_decode((uint_fast64_t) (_id - _node_first_leaf), skip_cols, skip_rows);

        ifs.seekg(_offset_header);
        ifs.ignore(skip_rows * _tiles_per_row * _tile_size * _tile_size * get_byte_stride(format_texture));

        for (uint32_t _row = 0; _row < _tile_size; _row++)
        {
            ifs.ignore(skip_cols * _tile_size * get_byte_stride(format_texture));
            ifs.read(&buf_tile[_row * _tile_size * get_byte_stride(format_texture)], _tile_size * get_byte_stride(format_texture));
            ifs.ignore((_tiles_per_row - skip_cols - 1) * _tile_size * get_byte_stride(format_texture));
        }

        std::ofstream output;

        switch (format_texture)
        {
            case Config::FORMAT_TEXTURE::RGBA8:

                output.open("id_" + std::to_string(_id) + ".rgba", std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
                break;
            case Config::FORMAT_TEXTURE::RGB8:

                output.open("id_" + std::to_string(_id) + ".rgb", std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
                break;
            case Config::FORMAT_TEXTURE::R8:

                output.open("id_" + std::to_string(_id) + ".gray", std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
                break;
        }

        output.write(&buf_tile[0], _tile_size * _tile_size * get_byte_stride(format_texture));
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

    ifs.close();

    if (verbose)
    {
        std::cout << "Thread #" << _thread_id << " done" << std::endl;
        std::cout << "Average tile creation time: " << average << " msec" << std::endl;
    }
}

void Preprocessor::extract_leaf_tile_rows(uint32_t _thread_id)
{
    Config::FORMAT_TEXTURE format_texture = Config::which_texture_format(config->GetValue(Config::TEXTURE_MANAGEMENT, Config::TEXTURE_FORMAT, Config::UNDEF));
    std::ifstream ifs;

    switch (format_texture)
    {
        case Config::FORMAT_TEXTURE::RGBA8:

            ifs.open(std::string(config->GetValue(Config::TEXTURE_MANAGEMENT, Config::NAME_TEXTURE, Config::UNDEF)) + ".rgba", std::ifstream::in | std::ifstream::binary);
            break;
        case Config::FORMAT_TEXTURE::RGB8:

            ifs.open(std::string(config->GetValue(Config::TEXTURE_MANAGEMENT, Config::NAME_TEXTURE, Config::UNDEF)) + ".rgb", std::ifstream::in | std::ifstream::binary);
            break;
        case Config::FORMAT_TEXTURE::R8:

            ifs.open(std::string(config->GetValue(Config::TEXTURE_MANAGEMENT, Config::NAME_TEXTURE, Config::UNDEF)) + ".gray", std::ifstream::in | std::ifstream::binary);
            break;
    }

    size_t dim_x = 0, dim_y = 0;

    read_dimensions(ifs, dim_x, dim_y);

    const auto tile_size = static_cast<const size_t>(atoi(config->GetValue(Config::TEXTURE_MANAGEMENT, Config::TILE_SIZE, Config::UNDEF)));
    if ((tile_size & (tile_size - 1)) != 0)
    {
        throw std::runtime_error("Tile size is not a power of 2");
    }

    const uint32_t tree_depth = QuadTree::calculate_depth(dim_x, tile_size);
    const size_t tiles_per_row = QuadTree::get_tiles_per_row(tree_depth);

    size_t rows_per_thread = tiles_per_row / thread::hardware_concurrency();

    auto **buf_tiles = new char *[tiles_per_row];
    for (int _tile_row = 0; _tile_row < tiles_per_row; _tile_row++)
    {
        buf_tiles[_tile_row] = new char[tile_size * tile_size * get_byte_stride(format_texture)];
    }

    size_t _tile_row = _thread_id * rows_per_thread;

    bool verbose = atoi(config->GetValue(Config::DEBUG, Config::VERBOSE, Config::UNDEF)) == 1;

    auto start = std::chrono::high_resolution_clock::now();
    double average = 0.0;

    if (_tile_row < tiles_per_row)
    {
        if (verbose)
        {
            start = std::chrono::high_resolution_clock::now();
        }

        ifs.ignore(_tile_row * tiles_per_row * tile_size * tile_size * get_byte_stride(format_texture));

        while (_tile_row < (_thread_id + 1) * rows_per_thread && _tile_row < tiles_per_row)
        {
            for (uint32_t _row = 0; _row < tile_size; _row++)
            {
                for (uint32_t _tile_col = 0; _tile_col < tiles_per_row; _tile_col++)
                {
                    ifs.read(&buf_tiles[_tile_col][_row * tile_size * get_byte_stride(format_texture)], tile_size * get_byte_stride(format_texture));
                }
            }

            for (uint32_t _tile_col = 0; _tile_col < tiles_per_row; _tile_col++)
            {
                uint_fast64_t _id = morton2D_64_encode(_tile_col, _tile_row) + QuadTree::get_first_node_id_of_depth(tree_depth);

                std::ofstream output;
                switch (format_texture)
                {
                    case Config::FORMAT_TEXTURE::RGBA8:

                        output.open("id_" + std::to_string(_id) + ".rgba", std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
                        break;
                    case Config::FORMAT_TEXTURE::RGB8:

                        output.open("id_" + std::to_string(_id) + ".rgb", std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
                        break;
                    case Config::FORMAT_TEXTURE::R8:

                        output.open("id_" + std::to_string(_id) + ".gray", std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
                        break;
                }
                output.write(&buf_tiles[_tile_col][0], tile_size * tile_size * get_byte_stride(format_texture));
                output.close();
            }

            if (verbose)
            {
                if ((_tile_row - _thread_id * rows_per_thread) % (rows_per_thread / 10 + 1) == 0)
                {
                    std::cout << "Thread #" << _thread_id << " progress: " << (_tile_row - _thread_id * rows_per_thread) * 100 / rows_per_thread << " %" << std::endl;
                }
            }

            _tile_row += 1;
        }

        if (verbose)
        {
            average = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start).count() / rows_per_thread / tiles_per_row;
        }
    }

    ifs.close();

    for (int _tile_row = 0; _tile_row < tiles_per_row; _tile_row++)
        delete[] buf_tiles[_tile_row];
    delete[] buf_tiles;

    if (verbose)
    {
        std::cout << "Thread #" << _thread_id << " done" << std::endl;
        std::cout << "Average tile creation time: " << average << " msec" << std::endl;
    }
}

void Preprocessor::stitch_tile_range(uint32_t _thread_id, size_t _node_start, size_t _node_end)
{
    auto tile_size = (size_t) atoi(config->GetValue(Config::TEXTURE_MANAGEMENT, Config::TILE_SIZE, Config::UNDEF));
    if ((tile_size & (tile_size - 1)) != 0)
    {
        throw std::runtime_error("Tile size is not a power of 2");
    }

    bool verbose = atoi(config->GetValue(Config::DEBUG, Config::VERBOSE, Config::UNDEF)) == 1;
    Config::FORMAT_TEXTURE format_texture = Config::which_texture_format(config->GetValue(Config::TEXTURE_MANAGEMENT, Config::TEXTURE_FORMAT, Config::UNDEF));

    size_t _range = _node_end - _node_start + 1;

    auto start = std::chrono::high_resolution_clock::now();
    double average = 0.0;

    char *_buf_concat = new char[2 * tile_size * 2 * tile_size * get_byte_stride(format_texture)];
    char *_buf_out = new char[tile_size * tile_size * get_byte_stride(format_texture)];

    for (size_t _id = _node_start; _id <= _node_end; _id++)
    {
        if (verbose)
        {
            start = std::chrono::high_resolution_clock::now();
        }

        std::ifstream _ifs_0;
        std::ifstream _ifs_1;
        std::ifstream _ifs_2;
        std::ifstream _ifs_3;

        switch (format_texture)
        {
            case Config::FORMAT_TEXTURE::RGBA8:

                _ifs_0.open("id_" + std::to_string(QuadTree::get_child_id(_id, 0)) + ".rgba");
                _ifs_1.open("id_" + std::to_string(QuadTree::get_child_id(_id, 1)) + ".rgba");
                _ifs_2.open("id_" + std::to_string(QuadTree::get_child_id(_id, 2)) + ".rgba");
                _ifs_3.open("id_" + std::to_string(QuadTree::get_child_id(_id, 3)) + ".rgba");
                break;
            case Config::FORMAT_TEXTURE::RGB8:

                _ifs_0.open("id_" + std::to_string(QuadTree::get_child_id(_id, 0)) + ".rgb");
                _ifs_1.open("id_" + std::to_string(QuadTree::get_child_id(_id, 1)) + ".rgb");
                _ifs_2.open("id_" + std::to_string(QuadTree::get_child_id(_id, 2)) + ".rgb");
                _ifs_3.open("id_" + std::to_string(QuadTree::get_child_id(_id, 3)) + ".rgb");
                break;
            case Config::FORMAT_TEXTURE::R8:

                _ifs_0.open("id_" + std::to_string(QuadTree::get_child_id(_id, 0)) + ".gray");
                _ifs_1.open("id_" + std::to_string(QuadTree::get_child_id(_id, 1)) + ".gray");
                _ifs_2.open("id_" + std::to_string(QuadTree::get_child_id(_id, 2)) + ".gray");
                _ifs_3.open("id_" + std::to_string(QuadTree::get_child_id(_id, 3)) + ".gray");
                break;
        }

        for (size_t _row = 0; _row < 2 * tile_size; _row++)
        {
            if (_row < tile_size)
            {
                _ifs_0.read(&_buf_concat[_row * 2 * tile_size * get_byte_stride(format_texture)], tile_size * get_byte_stride(format_texture));
                _ifs_1.read(&_buf_concat[(_row * 2 + 1) * tile_size * get_byte_stride(format_texture)], tile_size * get_byte_stride(format_texture));
            }
            else
            {
                _ifs_2.read(&_buf_concat[_row * 2 * tile_size * get_byte_stride(format_texture)], tile_size * get_byte_stride(format_texture));
                _ifs_3.read(&_buf_concat[(_row * 2 + 1) * tile_size * get_byte_stride(format_texture)], tile_size * get_byte_stride(format_texture));
            }
        }

        _ifs_0.close();
        _ifs_1.close();
        _ifs_2.close();
        _ifs_3.close();

        for (size_t _row = 0; _row < 2 * tile_size; _row+=2)
        {
            for (size_t _col = 0; _col < 2 * tile_size; _col+=2)
            {
                // TODO: implement a better downscaling algo
                switch (format_texture)
                {
                    case Config::FORMAT_TEXTURE::RGBA8:
                    {
                        char pixel_r = _buf_concat[(_row * 2 * tile_size + _col) * get_byte_stride(format_texture) + 0];
                        char pixel_g = _buf_concat[(_row * 2 * tile_size + _col) * get_byte_stride(format_texture) + 1];
                        char pixel_b = _buf_concat[(_row * 2 * tile_size + _col) * get_byte_stride(format_texture) + 2];
                        char pixel_a = _buf_concat[(_row * 2 * tile_size + _col) * get_byte_stride(format_texture) + 3];

                        _buf_out[(_row / 2 * tile_size + _col / 2) * get_byte_stride(format_texture) + 0] = pixel_r;
                        _buf_out[(_row / 2 * tile_size + _col / 2) * get_byte_stride(format_texture) + 1] = pixel_g;
                        _buf_out[(_row / 2 * tile_size + _col / 2) * get_byte_stride(format_texture) + 2] = pixel_b;
                        _buf_out[(_row / 2 * tile_size + _col / 2) * get_byte_stride(format_texture) + 3] = pixel_a;

                        break;
                    }
                    case Config::FORMAT_TEXTURE::RGB8:
                    {
                        char pixel_r = _buf_concat[(_row * 2 * tile_size + _col) * get_byte_stride(format_texture) + 0];
                        char pixel_g = _buf_concat[(_row * 2 * tile_size + _col) * get_byte_stride(format_texture) + 1];
                        char pixel_b = _buf_concat[(_row * 2 * tile_size + _col) * get_byte_stride(format_texture) + 2];

                        _buf_out[(_row / 2 * tile_size + _col / 2) * get_byte_stride(format_texture) + 0] = pixel_r;
                        _buf_out[(_row / 2 * tile_size + _col / 2) * get_byte_stride(format_texture) + 1] = pixel_g;
                        _buf_out[(_row / 2 * tile_size + _col / 2) * get_byte_stride(format_texture) + 2] = pixel_b;

                        break;
                    }
                    case Config::FORMAT_TEXTURE::R8:
                    {
                        char pixel_r = _buf_concat[(_row * 2 * tile_size + _col) * get_byte_stride(format_texture) + 0];

                        _buf_out[(_row / 2 * tile_size + _col / 2) * get_byte_stride(format_texture) + 0] = pixel_r;

                        break;
                    }
                }
            }
        }

        std::ofstream output;
        switch (format_texture)
        {
            case Config::FORMAT_TEXTURE::RGBA8:

                output.open("id_" + std::to_string(_id) + ".rgba", std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
                break;
            case Config::FORMAT_TEXTURE::RGB8:

                output.open("id_" + std::to_string(_id) + ".rgb", std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
                break;
            case Config::FORMAT_TEXTURE::R8:

                output.open("id_" + std::to_string(_id) + ".gray", std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
                break;
        }
        output.write(&_buf_out[0], tile_size * tile_size * get_byte_stride(format_texture));
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

    delete [] _buf_concat;
    delete [] _buf_out;

    if (verbose)
    {
        std::cout << "Thread #" << _thread_id << " done" << std::endl;
        std::cout << "Average tile stitch time: " << average << " msec" << std::endl;
    }
}

void Preprocessor::read_dimensions(std::ifstream &ifs, size_t &dim_x, size_t &dim_y)
{
    size_t fsize = (size_t) ifs.tellg();
    ifs.seekg(0, std::ios::end);
    fsize = (size_t) (ifs.tellg()) - fsize;

    Config::FORMAT_TEXTURE format_texture = Config::which_texture_format(config->GetValue(Config::TEXTURE_MANAGEMENT, Config::TEXTURE_FORMAT, Config::UNDEF));

    auto byte_stride = get_byte_stride(format_texture);

    dim_x = dim_y = (size_t) std::sqrt(fsize / byte_stride);

    ifs.seekg(0);
}

uint8_t Preprocessor::get_byte_stride(Config::FORMAT_TEXTURE _format_texture)
{
    uint8_t _byte_stride = 0;
    switch (_format_texture)
    {
        case Config::FORMAT_TEXTURE::RGBA8:_byte_stride = 4;
            break;
        case Config::FORMAT_TEXTURE::RGB8:_byte_stride = 3;
            break;
        case Config::FORMAT_TEXTURE::R8:_byte_stride = 1;
            break;
    }

    return _byte_stride;
}

}