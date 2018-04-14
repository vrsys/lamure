//
// Created by sebastian on 15.11.17.
//

#include <sstream>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <lamure/vt/pre/Preprocessor.h>
#include <lamure/vt/pre/AtlasFile.h>
#include <lamure/vt/ooc/TileProvider.h>
#include <lamure/vt/pre/DeltaECalculator.h>
#include <thread>

using namespace vt::pre;

Bitmap::PIXEL_FORMAT parsePixelFormat(const char *formatStr){
    if(std::strcmp(formatStr, "r") == 0){
        return Bitmap::PIXEL_FORMAT::R8;
    }else if(std::strcmp(formatStr, "rgb") == 0){
        return Bitmap::PIXEL_FORMAT::RGB8;
    }else if(std::strcmp(formatStr, "rgba") == 0){
        return Bitmap::PIXEL_FORMAT::RGBA8;
    }else{
        throw std::runtime_error("Invalid pixel format given.");
    }
}

AtlasFile::LAYOUT parseFileFormat(const char *formatStr){
    if(std::strcmp(formatStr, "raw") == 0){
        return AtlasFile::LAYOUT::RAW;
    }else if(std::strcmp(formatStr, "packed") == 0){
        return AtlasFile::LAYOUT::PACKED;
    }else{
        throw std::runtime_error("Invalid file format given.");
    }
}

const char *printLayout(AtlasFile::LAYOUT layout){
    switch(layout){
        case AtlasFile::LAYOUT::RAW:
            return "raw";
        case AtlasFile::LAYOUT::PACKED:
            return "packed";
    }
}

const char *printPxFormat(Bitmap::PIXEL_FORMAT pxFormat){
    switch(pxFormat){
        case Bitmap::PIXEL_FORMAT::R8:
            return "R8";
        case Bitmap::PIXEL_FORMAT::RGB8:
            return "RGB8";
        case Bitmap::PIXEL_FORMAT::RGBA8:
            return "RGBA8";
    }
}

int process(const int argc, const char **argv){
    if(argc != 11){
        std::cout << "Wrong count of parameters." << std::endl;
        std::cout << "Expected parameters:" << std::endl;
        std::cout << "\t<image file> <image pixel format (r, rgb, rgba)>" << std::endl;
        std::cout << "\t<image width> <image height>" << std::endl;
        std::cout << "\t<tile width> <tile height> <padding>" << std::endl;
        std::cout << "\t<out file (without extension)> <out file format (raw, packed)> <out pixel format (r, rgb, rgba)>" << std::endl;
        std::cout << "\t<max memory usage>" << std::endl;

        return 1;
    }

    Bitmap::PIXEL_FORMAT inPixelFormat;
    Bitmap::PIXEL_FORMAT outPixelFormat;
    AtlasFile::LAYOUT outFileFormat;

    try {
        inPixelFormat = parsePixelFormat(argv[1]);
    }catch(std::runtime_error error){
        std::cout << "Invalid input pixel format given: \"" << argv[1] << "\"." << std::endl;

        return 1;
    }

    try {
        outPixelFormat = parsePixelFormat(argv[9]);
    }catch(std::runtime_error &error){
        std::cout << "Invalid output pixel format given: \"" << argv[9] << "\"." << std::endl;

        return 1;
    }

    try {
        outFileFormat = parseFileFormat(argv[8]);
    }catch(std::runtime_error &error){
        std::cout << "Invalid output file format given: \"" << argv[8] << "\"." << std::endl;

        return 1;
    }

    std::stringstream stream;

    uint64_t imageWidth;
    uint64_t imageHeight;
    size_t tileWidth;
    size_t tileHeight;
    size_t padding;
    size_t maxMemory;

    stream.write(argv[2], std::strlen(argv[2]));

    if (!(stream >> imageWidth)) {
        std::cout << imageWidth << std::endl;
        std::cerr << "Invalid image width \"" << argv[2] << "\"." << std::endl;

        return 1;
    }

    stream.clear();
    stream.write(argv[3], std::strlen(argv[3]));

    if (!(stream >> imageHeight)) {
        std::cerr << "Invalid image height \"" << argv[3] << "\"." << std::endl;

        return 1;
    }

    stream.clear();
    stream.write(argv[4], std::strlen(argv[4]));

    if (!(stream >> tileWidth)) {
        std::cerr << "Invalid tile width \"" << argv[4] << "\"." << std::endl;

        return 1;
    }

    stream.clear();
    stream.write(argv[5], std::strlen(argv[5]));

    if (!(stream >> tileHeight)) {
        std::cerr << "Invalid tile height \"" << argv[5] << "\"." << std::endl;

        return 1;
    }

    stream.clear();
    stream.write(argv[6], std::strlen(argv[6]));

    if (!(stream >> padding)) {
        std::cerr << "Invalid padding \"" << argv[6] << "\"." << std::endl;

        return 1;
    }

    stream.clear();
    stream.write(argv[10], std::strlen(argv[10]));

    if (!(stream >> maxMemory)) {
        std::cerr << "Invalid maximum memory size \"" << argv[10] << "\"." << std::endl;

        return 1;
    }

    Preprocessor pre(argv[0], inPixelFormat, imageWidth, imageHeight);

    pre.setOutput(argv[7], outPixelFormat, outFileFormat, tileWidth, tileHeight, padding);
    pre.run(maxMemory);

    return 0;
}

int info(const int argc, const char **argv){
    if(argc != 1){
        std::cout << "Wrong count of parameters." << std::endl;
        std::cout << "Expected parameters:" << std::endl;
        std::cout << "\t<processed image>" << std::endl;

        return 1;
    }

    AtlasFile *atlas = nullptr;

    try {
        atlas = new AtlasFile(argv[0]);
    }catch(std::runtime_error &error){
        std::cout << "Could not open file \"" << argv[0] << "\"." << std::endl;

        return 1;
    }

    std::cout << "Information for file \"" << argv[0] << "\":" << std::endl;
    std::cout << "\torig. dim. : " << atlas->getImageWidth() << " px x " << atlas->getImageHeight() << " px" << std::endl;
    std::cout << "\ttile. dim. : " << atlas->getTileWidth() << " px x " << atlas->getTileHeight() << " px" << std::endl;
    std::cout << "\tpadding    : " << atlas->getPadding() << " px" << std::endl;
    std::cout << "\tlayout     : " << printLayout(atlas->getFormat()) << std::endl;
    std::cout << "\tpx format  : " << printPxFormat(atlas->getPixelFormat()) << std::endl;
    std::cout << "\tlevels     : " << atlas->getDepth() << std::endl;
    std::cout << "\ttiles      : " << atlas->getFilledTiles() << " / " << atlas->getTotalTiles() << std::endl << std::endl;
    std::cout << "\toffset index at " << atlas->getOffsetIndexOffset() << std::endl;
    std::cout << "\tcielab index at " << atlas->getCielabIndexOffset() << std::endl;
    std::cout << "\tpayload at " << atlas->getPayloadOffset() << std::endl;
    std::cout << std::endl;

    delete atlas;

    return 0;
}

int extract(const int argc, const char **argv){
    if(argc != 3){
        std::cout << "Wrong count of parameters." << std::endl;
        std::cout << "Expected parameters:" << std::endl;
        std::cout << "\t<processed image>" << std::endl;
        std::cout << "\t<level>" << std::endl;
        std::cout << "\t<out file>" << std::endl;

        return 1;
    }

    AtlasFile *atlas;

    try {
        atlas = new AtlasFile(argv[0]);
    }catch(std::runtime_error &error){
        std::cout << "Could not open file \"" << argv[0] << "\"." << std::endl;

        return 1;
    }

    std::stringstream stream;

    uint32_t level;

    stream.write(argv[1], std::strlen(argv[1]));

    if (!(stream >> level) || level >= atlas->getDepth()) {
        std::cerr << "Invalid level \"" << argv[1] << "\"." << std::endl;

        return 1;
    }

    atlas->extractLevel(level, argv[2]);

    std::cout << "Extracted level " << level << " to \"" << argv[2] << "\"." << std::endl;
    std::cout << "Dimensions  : " << (atlas->getInnerTileWidth() * QuadTree::getWidthOfLevel(level)) << " px x " << (atlas->getInnerTileHeight() * QuadTree::getWidthOfLevel(level)) << " px" << std::endl;
    std::cout << "Pixel Format: " << printPxFormat(atlas->getPixelFormat()) << std::endl;

    delete atlas;

    return 0;
}

int extract_raw(const int argc, const char **argv){
    if(argc != 2){
        std::cout << "Wrong count of parameters." << std::endl;
        std::cout << "Expected parameters:" << std::endl;
        std::cout << "\t<processed image>" << std::endl;
        std::cout << "\t<level>" << std::endl;
        std::cout << "\t<out file>" << std::endl;

        return 1;
    }

    AtlasFile *atlas = nullptr;
    uint8_t *buffer = nullptr;

    try {
        atlas = new AtlasFile(argv[0]);
        std::ofstream dataFile(argv[1], std::ios_base::binary);

        auto totalTiles = atlas->getTotalTiles();

        std::cout << totalTiles << std::endl;

        buffer = new uint8_t[totalTiles * atlas->getTileByteSize()];

        for(size_t i = 0; i < totalTiles; ++i){
            atlas->getTile(i, &buffer[i * atlas->getTileByteSize()]);
        }

        dataFile.write((char*)buffer, totalTiles * atlas->getTileByteSize());
    }catch(std::runtime_error &error){
        std::cout << "Could not open file \"" << argv[0] << "\"." << std::endl;

        return 1;
    }

    delete atlas;
    delete buffer;

    return 0;
}

int delta(const int argc, const char **argv){
    if(argc != 2){
        std::cout << "Wrong count of parameters." << std::endl;
        std::cout << "Expected parameters:" << std::endl;
        std::cout << "\t<processed image>" << std::endl;
        std::cout << "\t<max memory usage>" << std::endl;

        return 1;
    }

    DeltaECalculator *calculator;

    try {
        calculator = new DeltaECalculator(argv[0]);
    }catch(std::runtime_error &error){
        std::cout << "Could not open file \"" << argv[0] << "\"." << std::endl;

        return 1;
    }

    std::stringstream stream;
    size_t maxMemory;
    stream.write(argv[1], std::strlen(argv[1]));

    if (!(stream >> maxMemory)) {
        std::cerr << "Invalid maximum memory size \"" << argv[1] << "\"." << std::endl;

        return 1;
    }

    calculator->calculate(maxMemory);

    delete calculator;

    return 0;
}

uint64_t createRandomImage(const char *fileName, uint64_t width, uint64_t height, Bitmap::PIXEL_FORMAT pxFormat, size_t maxBufferSize) {
    std::random_device random;
    std::default_random_engine randomEng(random());
    std::uniform_int_distribution<uint8_t> randomGen(0, 255);

    std::ofstream outFile(fileName, std::ios::trunc);
    size_t pxByteSize = Bitmap::pixelSize(pxFormat);
    size_t bufferSize = (maxBufferSize / pxByteSize) * pxByteSize;

    uint8_t *buffer = new uint8_t[bufferSize];
    uint64_t absOffset = 0;

    for(size_t y = 0; y < height; ++y){
        for(size_t x = 0; x < width; ++x){
            for(size_t b = 0; b < pxByteSize; ++b){
                buffer[absOffset % bufferSize] = randomGen(randomEng);

                if((++absOffset % bufferSize) == 0){
                    outFile.write((char*)buffer, bufferSize);
                }
            }
        }
    }

    outFile.write((char*)buffer, absOffset % bufferSize);
    outFile.close();

    return absOffset;
}

void benchmarkPreprocessing(const char *folderName, size_t tileWidth, size_t tileHeight, size_t padding, size_t levels, Bitmap::PIXEL_FORMAT pxFormat, size_t maxMemorySize){
    size_t innerTileWidth = tileWidth - 2 * padding;
    size_t innerTileHeight = tileHeight - 2 * padding;

    std::string benchmarkFileName = std::string(folderName) + "/benchmark_process.csv";
    std::ofstream benchmarkFile(benchmarkFileName, std::ios::trunc);

    benchmarkFile << "img_width;img_height;img_size_in_byte;tile_width;tile_height;tile_padding;levels;atlas_file_size_in_byte;max_mem_usage;process_time_in_ms;delta_time_in_ms;" << std::endl;

    for(size_t level = 0; level < levels; ++level){
        size_t imgTileWidth = (1 << level);
        uint64_t imgWidth = imgTileWidth * innerTileWidth;
        uint64_t imgHeight = imgTileWidth * innerTileHeight;
        std::string imgFileName = std::string(folderName) + "/img/l" + std::to_string(level + 1) + "_w" + std::to_string(imgWidth) + "_h" + std::to_string(imgHeight) + ".data";

        uint64_t imgFileSize = createRandomImage(imgFileName.c_str(), imgWidth, imgHeight, pxFormat, maxMemorySize);

        Preprocessor pre(imgFileName, pxFormat, imgWidth, imgHeight);

        std::string atlasFileName = std::string(folderName) + "/atlas/l" + std::to_string(level + 1) + "_w" + std::to_string(imgWidth) + "_h" + std::to_string(imgHeight);

        pre.setOutput(atlasFileName, pxFormat, AtlasFile::LAYOUT::PACKED, tileWidth, tileHeight, padding);

        auto start = std::chrono::system_clock::now();
        pre.run(maxMemorySize);
        auto processDuration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start);

        std::ifstream atlasFile(atlasFileName + ".atlas", std::ios::ate);
        uint64_t atlasFileSize = atlasFile.tellg();
        atlasFile.close();

        DeltaECalculator delta((atlasFileName + ".atlas").c_str());

        start = std::chrono::system_clock::now();
        delta.calculate(maxMemorySize);
        auto deltaDuration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start);

        std::cout << processDuration.count() << std::endl;
        benchmarkFile << imgWidth << ";" << imgHeight << ";" << imgFileSize << ";" << tileWidth << ";" << tileHeight << ";" << padding << ";" << (level + 1) << ";" << atlasFileSize << ";" << maxMemorySize << ";" << processDuration.count() << ";" << deltaDuration.count() << ";" << std::endl;
    }

    benchmarkFile.close();
}

void benchmarkOutOfCore(const char *folderName, const char *atlasFileName, size_t maxMemSize) {
    std::string benchmarkFileName = std::string(folderName) + "/benchmark_ooc.csv";
    std::ofstream benchmarkFile(benchmarkFileName, std::ios::trunc);

    vt::ooc::TileProvider provider;

    auto atlas = provider.addResource(atlasFileName);
    uint64_t tileCount = atlas->getTotalTiles();

    std::random_device random;
    std::default_random_engine randomEng(random());
    std::uniform_int_distribution<uint64_t> randomGen(0, tileCount - 1);

    provider.start(maxMemSize);

    uint64_t
            tileId1 = randomGen(randomEng),
            tileId2 = randomGen(randomEng),
            tileId3 = randomGen(randomEng);

    size_t loadedCount = 0;
    size_t loadCount = SIZE_MAX;

    benchmarkFile << "img_width;img_height;tile_width;tile_height;tile_padding;levels;max_mem_usage;throughput_per_minute;throughput_per_second;milliseconds_per_tile;" << std::endl;
    auto start = std::chrono::system_clock::now();

    while ((std::chrono::system_clock::now() - start) < std::chrono::milliseconds(60000)) {
        if (provider.getTile(atlas, tileId1, loadCount--) != nullptr) {
            ++loadedCount;
            provider.ungetTile(atlas, tileId1);
            tileId1 = randomGen(randomEng);
        }

        if (provider.getTile(atlas, tileId2, loadCount--) != nullptr) {
            ++loadedCount;
            provider.ungetTile(atlas, tileId2);
            tileId2 = randomGen(randomEng);
        }

        if (provider.getTile(atlas, tileId3, loadCount--) != nullptr) {
            ++loadedCount;
            provider.ungetTile(atlas, tileId3);
            tileId3 = randomGen(randomEng);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    benchmarkFile << atlas->getImageWidth() << ";" << atlas->getImageHeight() << ";" << atlas->getTileWidth() << ";" << atlas->getTileHeight() << ";" << atlas->getPadding() << ";" << atlas->getDepth() << ";" << maxMemSize << ";" << loadedCount << ";" << ((float)loadedCount / 60.0) << ";" << (60000.0 / loadedCount) << ";";

    std::cout << loadedCount << std::endl;

    provider.stop();
    benchmarkFile.close();
}

int main(const int argc, const char **argv){
    if(argc >= 2){
        if(std::strcmp(argv[1], "process") == 0){
            return process(argc - 2, (const char**)((size_t)argv + 2 * sizeof(char*)));
        }else if(std::strcmp(argv[1], "delta") == 0){
            return delta(argc - 2, (const char**)((size_t)argv + 2 * sizeof(char*)));
        }else if(std::strcmp(argv[1], "info") == 0){
            return info(argc - 2, (const char**)((size_t)argv + 2 * sizeof(char*)));
        }else if(std::strcmp(argv[1], "extract") == 0){
            return extract(argc - 2, (const char**)((size_t)argv + 2 * sizeof(char*)));
        }else if(std::strcmp(argv[1], "extract_raw") == 0){
            return extract_raw(argc - 2, (const char**)((size_t)argv + 2 * sizeof(char*)));
        }else if(std::strcmp(argv[1], "benchmark_preprocess") == 0){
            benchmarkPreprocessing("/mnt/terabytes_of_textures/benchmark", 256, 256, 1, 10, Bitmap::PIXEL_FORMAT::RGB8, 1000000000);
            return 0;
        }else if(std::strcmp(argv[1], "benchmark_ooc") == 0){
            benchmarkOutOfCore("/mnt/terabytes_of_textures/benchmark", "/mnt/terabytes_of_textures/benchmark/atlas/l7_w16256_h16256.atlas", 2000000000);
            return 0;
        }
    }

    std::cout << "Expected instruction." << std::endl;
    std::cout << "Available:" << std::endl;
    std::cout << "\tprocess - to preprocess an image" << std::endl;
    std::cout << "\tdelta - to calculate delta e values on image" << std::endl;
    std::cout << "\tinfo - to read meta information of preprocessed image" << std::endl;
    std::cout << "\textract - to extract a certain level of detail from preprocessed image" << std::endl;
    std::cout << std::endl;

    return 1;
};