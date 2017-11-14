//
// Created by sebastian on 13.11.17.
//

#ifndef TILE_PROVIDER_REQUESTQUEUE_H
#define TILE_PROVIDER_REQUESTQUEUE_H

#include <map>
#include "definitions.h"
#include "Request.h"

using namespace std;

class RequestQueue {
protected:
    mutex _requestsLock;

    multimap<priority_type, Request*, greater<priority_type>> _requests;
public:
    void insert(Request *request);
    void remove(Request *request);
    Request *getNext();
    Request *popNext();
    size_t getLength();
};


#endif //TILE_PROVIDER_REQUESTQUEUE_H
