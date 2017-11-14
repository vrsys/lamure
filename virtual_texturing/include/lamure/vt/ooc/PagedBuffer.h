//
// Created by sebastian on 13.11.17.
//

#ifndef TILE_PROVIDER_PAGEDBUFFER_H
#define TILE_PROVIDER_PAGEDBUFFER_H

#include <cstdint>
#include <cstddef>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace vt {

    class Request;

    using namespace std;

    class QueuedProcessor;

    class PagedBuffer {
    public:
        enum STATE {
            FREE = 1,
            WRITING,
            READABLE,
            READING
        };
    protected:
        mutex _newFreeLock;
        condition_variable _newFree;

        mutex _freeLock;

        size_t _pageSize;
        size_t _pageNum;
        uint8_t *_buf;
        atomic<STATE> *_state;
        size_t *_freePages;

        size_t _popFree();

        QueuedProcessor *_processor;

        bool _free(size_t slotId);

    public:
        PagedBuffer(size_t pageSize, size_t pageNum);

        virtual size_t startWriting();

        virtual void makeReadable(size_t slotId, Request *req);

        void startReading(size_t slotId);

        void free(size_t slotId);

        uint8_t *getPointer(size_t slotId);

        void inform(QueuedProcessor *processor);
    };
}


#endif //TILE_PROVIDER_PAGEDBUFFER_H
