//
// Created by sebastian on 20.11.17.
//

#ifndef TILE_PROVIDER_TILEREQUEST_H
#define TILE_PROVIDER_TILEREQUEST_H

#include <lamure/vt/ooc/Observable.h>
#include <lamure/vt/ooc/PriorityQueue.h>
#include <lamure/vt/ooc/TileAtlas.h>

namespace vt
{
typedef uint64_t id_type;

template <typename priority_type>
class AbstractBufferSlot;

template <typename priority_type>
class TileRequest : public PriorityQueueContent<priority_type>, public Observable
{
  public:
    enum STATE
    {
        EMPTY = 1,
        QUEUED_FOR_LOADING,
        LOADING,
        QUEUED_FOR_DECOMPRESSING,
        DECOMPRESSING,
        READY
    };

    enum EVENT
    {
        DELETED = 1
    };

  protected:
    id_type _id;
    STATE _state;
    AbstractBufferSlot<TileRequest<priority_type> *> *_slot;
    streamoff _offset;
    streamoff _length;
    chrono::high_resolution_clock::time_point _createdAt;

    static uint16_t _instanceCount;

  public:
    TileRequest(id_type id, streamoff offset, streamoff length) : PriorityQueueContent<priority_type>()
    {
        _id = id;
        _state = STATE::EMPTY;
        _slot = nullptr;
        _offset = offset;
        _length = length;
        ++_instanceCount;
        _createdAt = chrono::high_resolution_clock::now();
    }

    ~TileRequest()
    {
        --_instanceCount;
        inform(EVENT::DELETED);
    }

    static uint16_t getInsatnceCount() { return _instanceCount; }

    STATE getState() { return _state; }

    void setState(STATE state) { _state = state; }

    id_type getId() { return _id; }

    AbstractBufferSlot<TileRequest<priority_type> *> *getSlot() { return _slot; }

    void setSlot(AbstractBufferSlot<TileRequest<priority_type> *> *slot) { _slot = slot; }

    streamoff getOffset() { return _offset; }

    streamoff getLength() { return _length; }

    bool isEmpty() { return _state == STATE::EMPTY; }

    chrono::high_resolution_clock::time_point getCreatedAt() { return _createdAt; }
};

template <typename priority_type>
uint16_t TileRequest<priority_type>::_instanceCount = 0;
}

#endif // TILE_PROVIDER_TILEREQUEST_H
