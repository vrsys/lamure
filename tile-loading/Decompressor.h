//
// Created by sebastian on 13.11.17.
//

#ifndef TILE_PROVIDER_DECOMPRESSOR_H
#define TILE_PROVIDER_DECOMPRESSOR_H


#include "QueuedProcessor.h"

class Decompressor : public QueuedProcessor{
private:
public:
    void request(Request *request);

    void beforeStart();

    void process(Request *request);

    void beforeStop();
};


#endif //TILE_PROVIDER_DECOMPRESSOR_H
