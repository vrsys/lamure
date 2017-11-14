//
// Created by sebastian on 13.11.17.
//

#include <iostream>
#include "PageProvider.h"

PageProvider::PageProvider(string filePath) {
    _filePath = filePath;
}

void PageProvider::request(Request *request){
    request->setState(Request::STATE::QUEUED_FOR_LOAD);

    QueuedProcessor::request(request);
}

void PageProvider::beforeStart() {
    _file.open(_filePath);
}

void PageProvider::process(Request *request){
    request->setState(Request::STATE::LOADING);

    if(_buffer != nullptr){
        size_t slotId = _buffer->startWriting();
        uint8_t *out = _buffer->getPointer(slotId);

        request->setBuffer(_buffer);
        request->setSlotId(slotId);

        _file.seekg(request->getOffset());
        _file.read((char*)out, request->getLength());
        _file.clear();

        _buffer->makeReadable(slotId, request);
    }

#ifdef DEBUG
    cout << request << " " << this_thread::get_id() << " Loaded offset " << request->getOffset() << " length " << request->getLength() << endl;
#endif
}

void PageProvider::beforeStop(){
    _file.close();
}