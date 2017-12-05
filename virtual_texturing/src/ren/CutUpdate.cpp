#include <lamure/vt/ren/CutUpdate.h>

CutUpdate::CutUpdate() { _cut = std::set<uint32_t>(); }
void CutUpdate::start() { _worker = std::thread(&CutUpdate::run, this); _worker.detach();}
void CutUpdate::run()
{
    std::unique_lock<std::mutex> lk(_dispatch_lock);
    while(!_should_stop)
    {
        dispatch();
        _cv.wait(lk);
    }
}
void CutUpdate::feedback()
{
    // TODO
    {
        std::unique_lock<std::mutex> lk(_dispatch_lock);
        std::cout << "feedback()" << std::endl;
    }
    _cv.notify_one();
}
void CutUpdate::dispatch()
{
    // TODO
    std::unique_lock<std::mutex> lk(_dispatch_lock);
    std::cout << "dispatch()" << std::endl;
}
void CutUpdate::stop()
{
    std::unique_lock<std::mutex> lk(_dispatch_lock);
    _should_stop = true;
}
