//
// Created by sebastian on 13.11.17.
//

#ifndef TILE_PROVIDER_QUEUEDPROCESSOR_H
#define TILE_PROVIDER_QUEUEDPROCESSOR_H


#include <thread>
#include <atomic>
#include <condition_variable>
#include "RequestQueue.h"
#include "PagedBuffer.h"

namespace vt {

    class QueuedProcessor {
    protected:
        mutex _newRequestLock;
        condition_variable _newRequest;

        RequestQueue _requests;
        atomic<bool> _running;
        atomic<size_t> _currentlyProcessing;
        thread *_thread;

        PagedBuffer *_buffer;
    public:
        QueuedProcessor();

        void request(Request *request);

        void start();

        void run();

        virtual void beforeStart() = 0;

        virtual void process(Request *request) = 0;

        virtual void beforeStop() = 0;

        void writeTo(PagedBuffer *buffer);

        void stop();

        size_t pendingCount();

        bool currentlyProcessing();
    };

}


#endif //TILE_PROVIDER_QUEUEDPROCESSOR_H
