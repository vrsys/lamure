//
// Created by sebastian on 13.11.17.
//

#ifndef TILE_PROVIDER_PAGEPROVIDER_H
#define TILE_PROVIDER_PAGEPROVIDER_H

#include <fstream>
#include <string>
#include <deque>
#include <cstdint>
#include <map>
#include "QueuedProcessor.h"

using namespace std;

namespace vt {

    class PageProvider : public QueuedProcessor {
    private:
        string _filePath;
        ifstream _file;
    public:
        PageProvider(string filePath);

        void request(Request *request);

        void beforeStart();

        void process(Request *request);

        void beforeStop();
    };

}


#endif //TILE_PROVIDER_PAGEPROVIDER_H
