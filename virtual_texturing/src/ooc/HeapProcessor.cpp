#include <lamure/vt/ooc/HeapProcessor.h>

namespace vt {
    namespace ooc {
        HeapProcessor::HeapProcessor() : _requests(0, nullptr){
            _thread = nullptr;
            _cache = nullptr;
        }

        void HeapProcessor::request(TileRequest *request){
            _requests.push(request->getPriority(), request);
        }

        void HeapProcessor::start(){
            if(_thread != nullptr){
                throw std::runtime_error("HeapProcessor is already started.");
            }

            if(_cache == nullptr){
                throw std::runtime_error("Cache needs to be set.");
            }

            _running = true;
            _thread = new std::thread(&HeapProcessor::run, this);
        }

        void HeapProcessor::run(){
            beforeStart();

            while(_running){
                PriorityHeapContent<uint32_t> *content;

                if(!_requests.pop(content, std::chrono::milliseconds(200))){
                    continue;
                }

                auto req = (TileRequest*)content;

                ++_currentlyProcessing;
                process(req);
                --_currentlyProcessing;
            }

            beforeStop();
        }

        void HeapProcessor::writeTo(TileCache *cache){
            _cache = cache;
        }

        void HeapProcessor::stop(){
            _running = false;
            _thread->join();
        }

        size_t HeapProcessor::pendingCount(){
            return _requests.size();
        }

        bool HeapProcessor::currentlyProcessing(){
            return _currentlyProcessing > 0;
        }
    }
}