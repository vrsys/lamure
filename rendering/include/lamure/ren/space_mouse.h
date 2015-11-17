// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef REN_SPACE_MOUSE_H_
#define REN_SPACE_MOUSE_H_

#include <lamure/ren/platform.h>

#include <thread>
#include <memory>

namespace lamure {
namespace ren
{

/* Usage example:
 *
 *  SpaceMouse sm;
 *  sm.set_event_callback([](uint16_t code, float value){ std::cout << code << " " << value << std::endl; });
 *  sm.Run();
 *
 * where the 'code' attribute in callback function:
 * 0 - X
 * 1 - Y
 * 2 - Z
 * 3 - RX
 * 4 - RY
 * 5 - RZ
 * 'value' attribute range approx [-500..500]
 */

class RENDERING_DLL SpaceMouse
{
public:
    typedef std::function<void(uint16_t, float)> EventFunction;

                        SpaceMouse();
    virtual             ~SpaceMouse();
                        SpaceMouse(const SpaceMouse&) = delete;
                        SpaceMouse& operator=(const SpaceMouse&) = delete;

    void                Run();
    void                set_event_callback(const EventFunction& callback) 
                            { event_callback_ = callback; }

private:
    void                Stop();
    void                ReadLoop();

    std::thread         thread_; 

    struct impl_t;
    std::unique_ptr<impl_t> impl_;

    bool                shutdown_flag_;
    EventFunction       event_callback_;
};


}}

#endif // REN_SPACE_MOUSE_H_

