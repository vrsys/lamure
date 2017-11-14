//
// Created by sebastian on 13.11.17.
//

#ifndef TILE_PROVIDER_REQUEST_H
#define TILE_PROVIDER_REQUEST_H

#include <mutex>
#include <atomic>
#include "lamure/vt/common.h"
#include "PagedBuffer.h"

using namespace std;

namespace vt {

class RequestQueue;

class Request {
public:
    enum STATE{
        UNINITIALISED = 1,
        QUEUED_FOR_LOAD,
        LOADING,
        QUEUED_FOR_DECOMPRESS,
        DECOMPRESSING,
        READY,
        ABORTED
    };
protected:
    mutex _parentLock;
    mutex _priorityLock;

    RequestQueue *_parent;
    priority_type _priority;
    id_type _id;
    uint64_t _offset;
    uint64_t _len;
    PagedBuffer *_buffer;
    size_t _slotId;

    atomic<STATE> _state;
public:
    Request();
    Request(Request &req);

    void setParent(RequestQueue *parent);
    void setPriority(priority_type priority);
    void setState(STATE state);
    uint64_t getOffset();
    void setOffset(uint64_t offset);

    uint64_t getLength();
    void setLength(uint64_t length);

    void setBuffer(PagedBuffer *buffer);
    void setSlotId(size_t slotId);

    PagedBuffer *getBuffer();
    size_t getSlotId();

    RequestQueue *getParent();
    priority_type getPriority();

    id_type getId();
    void setId(id_type id);
};

}


#endif //TILE_PROVIDER_REQUEST_H
