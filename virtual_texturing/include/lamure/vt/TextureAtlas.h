//
// Created by sebastian on 13.11.17.
//

#ifndef TILE_PROVIDER_TEXTUREATLAS_H
#define TILE_PROVIDER_TEXTUREATLAS_H


#include <cstddef>
#include <string>
#include "PageProvider.h"
#include "PageCache.h"
#include "Decompressor.h"

using namespace std;

class TextureAtlas {
public:
    enum COMPRESSION{
        NONE = 1,
        DUMMY
    };
private:
    string _fileName;
    size_t _pageByteSize;
    COMPRESSION _compression;
    PageProvider *_provider;
    PagedBuffer *_buffer;
    Decompressor *_decompressor;
    PageCache *_cache;
public:
    TextureAtlas(string fileName, size_t pageByteSize, COMPRESSION compression);
    void start();
    uint8_t *getPageById(id_type id);
};


#endif //TILE_PROVIDER_TEXTUREATLAS_H
