//
// Created by sebastian on 23.11.17.
//

#ifndef TILE_PROVIDER_OBSERVABLE_H
#define TILE_PROVIDER_OBSERVABLE_H

#include <lamure/vt/common.h>
#include <lamure/vt/ooc/Observer.h>

using namespace std;

namespace vt
{
class Observable
{
  protected:
    map<event_type, set<Observer *>> _events;

  public:
    Observable() = default;

    void observe(event_type event, Observer *observer);

    void unobserve(event_type event, Observer *observer);

    virtual void inform(event_type event);
};
}

#endif // TILE_PROVIDER_OBSERVABLE_H
