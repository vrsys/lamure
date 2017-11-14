//
// Created by sebastian on 13.11.17.
//

#include <iostream>
#include "lamure/vt/ooc/PagedBuffer.h"
#include "lamure/vt/ooc/QueuedProcessor.h"

namespace vt {

    PagedBuffer::PagedBuffer(size_t pageSize, size_t pageNum) {
        _processor = nullptr;
        _pageSize = pageSize;
        _pageNum = pageNum;
        _state = new atomic<STATE>[pageNum];
        _freePages = new size_t[pageNum];
        _buf = new uint8_t[pageSize * pageNum];

        for (size_t i = 0; i < pageNum; ++i) {
            _freePages[i] = i;
        }
    }

    size_t PagedBuffer::_popFree() {
        _freeLock.lock();

        size_t result = _freePages[0];

        for (size_t i = 0; i < (_pageNum - 1) && _freePages[i] != _pageNum; ++i) {
            _freePages[i] = _freePages[i + 1];
        }

        _freePages[_pageNum - 1] = _pageNum;

        _freeLock.unlock();

        return result;
    }

    size_t PagedBuffer::startWriting() {
        size_t result = _popFree();
        unique_lock<mutex> lock(_newFreeLock);

        while (result == _pageNum) {
            result = _popFree();

            _newFree.wait_for(lock, chrono::milliseconds(200));
        }

        _state[result] = STATE::WRITING;

        return result;
    }

    void PagedBuffer::makeReadable(size_t slotId, Request *req) {
        switch (_state[slotId]) {
            case STATE::WRITING:
                _state[slotId] = STATE::READABLE;

                if (_processor != nullptr) {
                    _processor->request(req);
                }

                break;
            case STATE::READING:
                _state[slotId] = STATE::READABLE;
                break;
            default:
                throw 1;
        }

#ifdef DEBUG
        //cout << req << " " << this_thread::get_id() << " Buffered offset " << req->getOffset() << " length " << req->getLength() << endl;
#endif
    }

    void PagedBuffer::startReading(size_t slotId) {
        if (_state[slotId] == STATE::READABLE) {
            _state[slotId] = STATE::READING;
            return;
        }

        throw 1;
    }

    bool PagedBuffer::_free(size_t slotId) {
        if (_state[slotId] == STATE::READABLE) {
            _state[slotId] = STATE::FREE;

            return true;
        }

        return false;
    }

    void PagedBuffer::free(size_t slotId) {
        if (_free(slotId)) {
            _freeLock.lock();

            for (size_t i = 0; i < _pageNum; ++i) {
                if (_freePages[i] == _pageNum) {
                    _freePages[i] = slotId;
                    break;
                }
            }

            _freeLock.unlock();

            _newFree.notify_one();

            return;
        }

        throw 1;
    }

    void PagedBuffer::inform(QueuedProcessor *processor) {
        _processor = processor;
    }

    uint8_t *PagedBuffer::getPointer(size_t slotId) {
        return (uint8_t *) ((size_t) _buf + slotId * _pageSize);
    }

}