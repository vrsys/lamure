//
// Created by sebastian on 23.11.17.
//

#ifndef TILE_PROVIDER_OBSERVER_H
#define TILE_PROVIDER_OBSERVER_H

#include <cstdint>

namespace vt
{
typedef uint32_t event_type;

class Observable;

class Observer
{
  protected:
  public:
    Observer() = default;
    virtual void inform(event_type event, Observable *observable) = 0;
};
}

#endif // TILE_PROVIDER_OBSERVER_H
