//
// Created by sebastian on 23.11.17.
//

#include <iostream>
#include <lamure/vt/ooc/Observable.h>

namespace vt
{
void Observable::observe(event_type event, Observer *observer)
{
    auto eventIter = _events.find(event);

    if(eventIter == _events.end())
    {
        eventIter = _events.insert(pair<event_type, set<Observer *>>(event, set<Observer *>())).first;
    }

    auto observerSet = eventIter->second;

    auto observerIter = observerSet.find(observer);

    if(observerIter != observerSet.end())
    {
        return;
    }

    eventIter->second.insert(observer);
}

void Observable::unobserve(event_type event, Observer *observer)
{
    auto eventIter = _events.find(event);

    if(eventIter == _events.end())
    {
        return;
    }

    auto observerSet = eventIter->second;

    auto observerIter = observerSet.find(observer);

    if(observerIter == observerSet.end())
    {
        return;
    }

    //(*observerIter) = nullptr;
    observerSet.erase(observer);
}

void Observable::inform(event_type event)
{
    auto iter = _events.find(event);

    if(iter == _events.end())
    {
        return;
    }

    auto observers = iter->second;
    // Observer arr[observers.size()];
    // size_t ctr = 0;

    for(auto observer = observers.begin(); observer != observers.end(); ++observer)
    {
        // auto nxt = next(observer);

        // arr[ctr] = (*observer);
        (*observer)->inform(event, this);
        // observer = nxt;
        //++ctr;
    }
}
}
