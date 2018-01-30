//
// Created by sebastian on 24.01.18.
//

#ifndef TILE_PROVIDER_ATLASFILE_H
#define TILE_PROVIDER_ATLASFILE_H


#include <cstdint>
#include <fstream>
#include <cstring>
#include <lamure/vt/pre/Bitmap.h>
#include <lamure/vt/pre/QuadTree.h>
#include <lamure/vt/pre/GenericIndex.h>

namespace vt{
    namespace pre{
        class AtlasFile {
        public:
            enum LAYOUT{
                RAW = 1,
                PACKED
            };

            static constexpr size_t HEADER_SIZE = 47;

        private:
            std::ifstream _file;

            uint64_t _imageWidth;
            uint64_t _imageHeight;
            uint64_t _tileWidth;
            uint64_t _tileHeight;
            uint64_t _padding;

            Bitmap::PIXEL_FORMAT _pxFormat;
            LAYOUT _format;

            uint64_t _innerTileWidth;
            uint64_t _innerTileHeight;
            uint64_t _imageTileWidth;
            uint64_t _imageTileHeight;
            uint64_t _tilePxSize;
            uint64_t _tileByteSize;
            size_t _pxSize;

            uint32_t _treeDepth;
            uint64_t _filledTileCount;
            uint64_t _totalTileCount;

            GenericIndex *_index;

            uint64_t _getLE(uint8_t *data);
            Bitmap::PIXEL_FORMAT _getPixelFormat(uint8_t *data);
            LAYOUT _getFormat(uint8_t *data);

        public:
            AtlasFile(const char *fileName);
            ~AtlasFile();

            uint64_t getFilledTiles();
            uint64_t getTotalTiles();
            uint32_t getDepth();
            uint64_t getImageWidth();
            uint64_t getImageHeight();
            uint64_t getTileWidth();
            uint64_t getTileHeight();
            uint64_t getInnerTileWidth();
            uint64_t getInnerTileHeight();
            uint64_t getTileByteSize();
            uint64_t getPadding();
            Bitmap::PIXEL_FORMAT getPixelFormat();
            LAYOUT getFormat();

            bool getTile(uint64_t id, uint8_t *out);
            void extractLevel(uint32_t level, const char *fileName);

            uint64_t _getOffset(uint64_t id);
        };
    }
}

#endif //TILE_PROVIDER_ATLASFILE_H
