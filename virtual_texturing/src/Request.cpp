//
// Created by sebastian on 13.11.17.
//

#include "../include/lamure/vt/Request.h"
#include "../include/lamure/vt/RequestQueue.h"

Request::Request() {
    _parent = nullptr;
    _priority = 0;
}

Request::Request(Request &req) : Request() {
    _priority = req._priority;
}

void Request::setState(STATE state){
    _state = state;
}

void Request::setParent(RequestQueue *parent) {
    _parentLock.lock();

    if(_parent != parent) {
        if (_parent != nullptr) {
            _parent->remove(this);
        }

        _parent = parent;

        if(_parent != nullptr) {
            _parent->insert(this);
        }
    }

    _parentLock.unlock();
}

void Request::setPriority(priority_type priority){
    _parentLock.lock();
    _priorityLock.lock();

    if(priority != _priority && _parent != nullptr){
        _parent->remove(this);

        _priority = priority;

        _parent->insert(this);
    }

    _priorityLock.unlock();
    _parentLock.unlock();
}

RequestQueue *Request::getParent(){
    lock_guard<mutex> lock(_parentLock);

    return _parent;
}

priority_type Request::getPriority(){
    lock_guard<mutex> lock(_priorityLock);

    return _priority;
}

void Request::setOffset(uint64_t offset){
    _offset = offset;
}

uint64_t Request::getOffset(){
    return _offset;
}

uint64_t Request::getLength(){
    return _len;
}

void Request::setLength(uint64_t length){
    _len = length;
}

void Request::setBuffer(PagedBuffer *buffer){
    _buffer = buffer;
}

void Request::setSlotId(size_t slotId){
    _slotId = slotId;
}

PagedBuffer *Request::getBuffer(){
    return _buffer;
}

size_t Request::getSlotId(){
    return _slotId;
}

id_type Request::getId(){
    return _id;
}

void Request::setId(id_type id){
    _id = id;
}