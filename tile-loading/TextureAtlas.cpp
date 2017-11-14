//
// Created by sebastian on 13.11.17.
//

#include "TextureAtlas.h"
#include "definitions.h"
#include "Decompressor.h"

TextureAtlas::TextureAtlas(string fileName, size_t pageByteSize, COMPRESSION compression){
    _fileName = fileName;
    _pageByteSize = pageByteSize;
    _compression = compression;
    _buffer = nullptr;
    _decompressor = nullptr;

    _provider = new PageProvider(fileName);
    _cache = new PageCache(pageByteSize, 128);

    QueuedProcessor *proc = _provider;

    if(compression == COMPRESSION::DUMMY){
        _buffer = new PagedBuffer(pageByteSize, 4);
        _decompressor = new Decompressor();

        proc->writeTo(_buffer);
        _buffer->inform(_decompressor);

        proc = _decompressor;
    }

    proc->writeTo(_cache);
}

void TextureAtlas::start(){
    _provider->start();

    if(_decompressor != nullptr){
        _decompressor->start();
    }
}

uint8_t *TextureAtlas::getPageById(id_type id){
    size_t slotId = _cache->getPageById(id);

    if(slotId == 128){
        Request *req = new Request();

        req->setId(id);
        req->setOffset(id * _pageByteSize);
        req->setLength(_pageByteSize);

        _provider->request(req);

        return nullptr;
    }

    return _cache->getPointer(slotId);
}