//
// Created by sebastian on 13.11.17.
//

#include "../include/lamure/vt/QueuedProcessor.h"

QueuedProcessor::QueuedProcessor() {
    _running = false;
    _thread = nullptr;
    _buffer = nullptr;
    _currentlyProcessing = 0;
}

void QueuedProcessor::request(Request *request){
    request->setParent(&_requests);
    _newRequest.notify_one();
}

void QueuedProcessor::start(){
    if(_thread != nullptr){
        return;
    }

    _running = true;
    _thread = new thread(&QueuedProcessor::run, this);
}

void QueuedProcessor::run(){
    beforeStart();
    unique_lock<mutex> lock(_newRequestLock);

    while(_running){
        Request *request = _requests.popNext();

        if(lock.owns_lock()){
            lock.unlock();
        }

        if(request != nullptr){
            ++_currentlyProcessing;
            process(request);
            --_currentlyProcessing;
            continue;
        }

        _newRequest.wait_for(lock, chrono::milliseconds(200));
    }

    beforeStop();
}

void QueuedProcessor::writeTo(PagedBuffer *buffer){
    _buffer = buffer;
}

void QueuedProcessor::stop(){
    _running = false;
}

size_t QueuedProcessor::pendingCount(){
    return _requests.getLength();
}

bool QueuedProcessor::currentlyProcessing(){
    return _currentlyProcessing > 0;
}