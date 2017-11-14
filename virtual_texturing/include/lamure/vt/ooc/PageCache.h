//
// Created by sebastian on 13.11.17.
//

#ifndef TILE_PROVIDER_PAGECACHE_H
#define TILE_PROVIDER_PAGECACHE_H


#include <map>
#include "PagedBuffer.h"
#include "lamure/vt/common.h"

namespace vt {

    class PageCache : public PagedBuffer {
    private:
        clock_t *_lastUsed;
        id_type *_ids;
    public:
        PageCache(size_t pageSize, size_t pageNum);

        size_t startWriting();

        void makeReadable(size_t slotId, Request *req);

        size_t getPageById(id_type id);
    };
}


#endif //TILE_PROVIDER_PAGECACHE_H
