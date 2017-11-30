//
// Created by sebastian on 13.11.17.
//

#include <cstring>
#include <iostream>
#include "lamure/vt/ooc/Decompressor.h"

namespace vt {

    void Decompressor::request(Request *request) {
        request->setState(Request::STATE::QUEUED_FOR_DECOMPRESS);

        QueuedProcessor::request(request);
    }

    void Decompressor::beforeStart() {

    }

    void Decompressor::process(Request *request) {
        request->setState(Request::STATE::DECOMPRESSING);

        //this_thread::sleep_for(chrono::milliseconds(50));

        if (_buffer != nullptr) {
            size_t newSlotId = _buffer->startWriting();
            uint8_t *out = _buffer->getPointer(newSlotId);

            PagedBuffer *buf = request->getBuffer();
            size_t oldSlotId = request->getSlotId();
            uint8_t *ptr = buf->getPointer(oldSlotId);

            request->setBuffer(_buffer);
            request->setSlotId(newSlotId);

            buf->startReading(oldSlotId);

            memcpy(out, ptr, request->getLength());

            buf->makeReadable(oldSlotId, request);
            buf->free(oldSlotId);

            _buffer->makeReadable(newSlotId, request);
        }

#ifdef DEBUG
        cout << request << " " << this_thread::get_id() << " Compressed offset " << request->getOffset() << " length " << request->getLength() << endl;
#endif
    }

    void Decompressor::beforeStop() {

    }

}