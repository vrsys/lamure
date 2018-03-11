//
// Created by sebastian on 13.11.17.
//

#ifndef VT_OOC_HEAP_PROCESSOR_H
#define VT_OOC_HEAP_PROCESSOR_H


#include <thread>
#include <atomic>
#include <condition_variable>
#include <lamure/vt/ooc/TileCache.h>
#include <lamure/vt/PriorityHeap.h>
#include <lamure/vt/ooc/TileRequest.h>

namespace vt {
    namespace ooc {
        class HeapProcessor {
        protected:
            PriorityHeap<uint32_t> _requests;

            std::atomic<bool> _running;
            std::atomic<size_t> _currentlyProcessing;
            std::thread *_thread;

            TileCache *_cache;
        public:
            HeapProcessor();

            void request(TileRequest *request);

            void start();

            void run();

            virtual void beforeStart() = 0;

            virtual void process(TileRequest *req) = 0;

            virtual void beforeStop() = 0;

            void writeTo(TileCache *cache);

            void stop();

            size_t pendingCount();

            bool currentlyProcessing();
        };
    }
}

#endif //TILE_PROVIDER_HEAP_PROCESSOR_H
