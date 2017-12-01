//
// Created by sebastian on 13.11.17.
//

#include "lamure/vt/ooc/PageCache.h"
#include "lamure/vt/ooc/QueuedProcessor.h"

namespace vt {

    PageCache::PageCache(size_t pageSize, size_t pageNum) : PagedBuffer(pageSize, pageNum) {
        _lastUsed = new clock_t[pageNum];
        _ids = new id_type[pageNum];
    }

    size_t PageCache::startWriting() {
        size_t result = _popFree();

        if (result == _pageNum) {
            clock_t lastUsed = clock() + 1000;

            for (size_t i = 0; i < _pageNum; ++i) {
                if (_lastUsed[i] < lastUsed) {
                    lastUsed = _lastUsed[i];
                    result = i;
                }
            }

            if (!_free(result)) {
                throw 1;
            }
        }

        _state[result] = STATE::WRITING;

        return result;
    }

    void PageCache::makeReadable(size_t slotId, Request *req) {
        switch (_state[slotId]) {
            case STATE::WRITING:
                _state[slotId] = STATE::READABLE;
                _ids[slotId] = req->getId();
                _lastUsed[slotId] = clock();

                if (_processor != nullptr) {
                    _processor->request(req);
                }

                break;
            case STATE::READING:
                _state[slotId] = STATE::READABLE;
                _ids[slotId] = req->getId();
                _lastUsed[slotId] = clock();
                break;
            default:
                throw 1;
        }

        delete req;

#ifdef DEBUG
        //cout << req << " " << this_thread::get_id() << " Buffered offset " << req->getOffset() << " length " << req->getLength() << endl;
#endif
    }

    size_t PageCache::getPageById(id_type id) {
        for (size_t i = 0; i < _pageNum; ++i) {
            if ((_state[i] == STATE::READABLE || _state[i] == STATE::READING) && _ids[i] == id) {
                _lastUsed[i] = clock();
                return i;
            }
        }

        return _pageNum;
    }

}