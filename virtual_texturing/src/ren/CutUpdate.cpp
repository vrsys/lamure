#include <lamure/vt/ren/CutUpdate.h>

CutUpdate::CutUpdate() : _dispatch_lock()
{
    _cut = std::set<uint32_t>();
    _should_stop.store(false);
    _new_feedback.store(false);
    _feedback_buffer = nullptr;
}
void CutUpdate::start() {
    _worker = std::thread(&CutUpdate::run, this);
}

void CutUpdate::run()
{
    while(!_should_stop.load())
    {
        //if(_new_feedback.load()) {
            std::unique_lock<std::mutex> lk(_dispatch_lock, std::defer_lock);
            dispatch();
            //_new_feedback.store(false);
        //}
        _cv.wait(lk);
        //std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
void CutUpdate::feedback(uint32_t *buf)
{
    //if(!_new_feedback.load()) {
        std::unique_lock<std::mutex> lk(_dispatch_lock);
        this->_feedback_buffer = buf;
        //_new_feedback.store(true);
        lk.unlock();
        _cv.notify_one();
    //}
}
void CutUpdate::dispatch()
{
    // TODO
    if(_feedback_buffer != nullptr) {
        uint32_t dummy = this->_feedback_buffer[0];
        dummy += 1;
        this->_feedback_buffer[0] = dummy;
    }
}
void CutUpdate::stop()
{
    //_should_stop.store(true);
    _worker.join();
}

CutUpdate::~CutUpdate() {
    std::cout << "burn it down" << std::endl;
}
