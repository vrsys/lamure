#ifndef VT_OBSERVABLE_H
#define VT_OBSERVABLE_H

#include <vector>
#include <cstdint>
#include <map>
#include <set>
#include <lamure/vt/Observer.h>

namespace vt {

    class Observable {
    protected:
        map<event_type, set<Observer*>> _events;
    public:
        Observable() = default;

        void observe(event_type event, Observer *observer);

        void unobserve(event_type event, Observer *observer);

        virtual void inform(event_type event);
    };

}


#endif //TILE_PROVIDER_OBSERVABLE_H
