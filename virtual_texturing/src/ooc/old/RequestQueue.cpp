//
// Created by sebastian on 13.11.17.
//

#include <iostream>
#include "lamure/vt/ooc/RequestQueue.h"

namespace vt {

    void RequestQueue::insert(Request *request) {
        lock_guard<mutex> lock(_requestsLock);

        _requests.insert(pair<priority_type, Request *>(request->getPriority(), request));
    }

    void RequestQueue::remove(Request *request) {
        _requestsLock.lock();

        for (auto iter = _requests.begin(); iter != _requests.end(); ++iter) {
            if (iter->second == request) {
                _requests.erase(iter);
                break;
            }
        }

        _requestsLock.unlock();
    }

    Request *RequestQueue::getNext() {
        lock_guard<mutex> lock(_requestsLock);

        auto iter = _requests.rbegin();

        return iter == _requests.rend() ? nullptr : iter->second;
    }

    Request *RequestQueue::popNext() {
        lock_guard<mutex> lock(_requestsLock);

        auto iter = _requests.begin();
        Request *result = nullptr;

        if (iter != _requests.end()) {
            result = iter->second;
            _requests.erase(iter);
        }

        return result;
    }

    size_t RequestQueue::getLength() {
        return _requests.size();
    }

}