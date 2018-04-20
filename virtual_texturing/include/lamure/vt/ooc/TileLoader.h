//
// Created by sebastian on 10.03.18.
//

#ifndef VT_OOC_HEAPPROCESSOR_H
#define VT_OOC_HEAPPROCESSOR_H

#include <lamure/vt/ooc/HeapProcessor.h>

namespace vt {
    namespace ooc {
        class TileLoader : public HeapProcessor{
        public:
            TileLoader();

            void beforeStart() override;

            void process(TileRequest *req) override;

            void beforeStop() override;
        };
    }
}


#endif //TILE_PROVIDER_TILELOADER_H
