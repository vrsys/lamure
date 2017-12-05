#include <lamure/vt/ren/CutUpdate.h>

CutUpdate::CutUpdate()
{
    _cut = std::set<uint32_t>();
    _should_stop.store(false);
}
void CutUpdate::start() { _worker = std::thread(&CutUpdate::run, this); }
void CutUpdate::run()
{
    std::unique_lock<std::mutex> lk(_dispatch_lock);
    std::cout << "1: " << std::this_thread::get_id() << std::endl;

    while(!_should_stop.load())
    {
        dispatch();
        _cv.wait(lk);
    }
}
void CutUpdate::feedback()
{
    std::cout << "2: " << std::this_thread::get_id() << std::endl;
    // TODO
    std::lock_guard<std::mutex> lk(_dispatch_lock);
    std::cout << "feedback()" << std::endl;
    _cv.notify_one();
}
void CutUpdate::dispatch()
{
    // TODO
    std::cout << "dispatch()" << std::endl;
}
void CutUpdate::stop()
{
    std::unique_lock<std::mutex> lk(_dispatch_lock);
    _should_stop.store(true);
    _worker.join();
}
