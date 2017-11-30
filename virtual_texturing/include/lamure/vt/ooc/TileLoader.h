//
// Created by sebastian on 21.11.17.
//

#ifndef TILE_PROVIDER_TILELOADER_H
#define TILE_PROVIDER_TILELOADER_H

#include <fstream>
#include <thread>
#include "lamure/vt/ooc/TileBuffer.h"

using namespace std;

namespace seb {

    template<typename priority_type>
    class TileLoader {
    protected:
        alignas(CACHELINE_SIZE) ifstream *_fileStream;
        alignas(CACHELINE_SIZE) TileBuffer<priority_type> *_buffer;
        alignas(CACHELINE_SIZE) PriorityQueue<priority_type> _requests;
        alignas(CACHELINE_SIZE) thread *_thread;
        alignas(CACHELINE_SIZE) atomic<bool> _running;

    public:
        TileLoader() {
            _fileStream = nullptr;
            _buffer = nullptr;
            _thread = nullptr;
            _running = false;
        }

        void setFileStream(ifstream *fileStream) {
            _fileStream = fileStream;
        }

        ifstream *getFileStream() {
            return _fileStream;
        }

        void setBuffer(TileBuffer<priority_type> *buffer) {
            _buffer = buffer;
        }

        void read(TileRequest<priority_type> *req){
            auto content = (PriorityQueueContent<priority_type>*)req;

            _requests.push(content);
        }

        bool process(TileRequest<priority_type> *req) {
            auto slot = _buffer->getFreeSlot(chrono::milliseconds(100));

            if (slot == nullptr) {
                return false;
            }

            //cout << "L: process request " << req->getId() << endl;

            req->setState(TileRequest<priority_type>::STATE::LOADING);

            auto ptr = slot->getPointer();

            _fileStream->seekg(req->getOffset());
            _fileStream->read((char *) ptr, req->getLength());

            if(req == nullptr){
                cout << "irgghh" << endl;
            }

            slot->setAssocData(req);
            req->setSlot(slot);

            req->setState(TileRequest<priority_type>::STATE::QUEUED_FOR_DECOMPRESSING);
            _buffer->slotReady(slot);

            return true;
        }

        void run() {
            PriorityQueueContent<priority_type> *req = nullptr;

            while (_running) {
                if (req == nullptr && !_requests.pop(req, chrono::milliseconds(100))) {
                    continue;
                }

                if (!process((TileRequest<priority_type>*)req)) {
                    continue;
                }

                //delete req;
                req = nullptr;
            }

            //cout << "FileReader terminated ..." << endl;
        }

        void start() {
            _running = true;
            _thread = new thread(&TileLoader::run, this);

            //cout << "FileReader started ..." << endl;
        }

        void stop() {
            _running = false;
            //cout << "FileReader stopped ..." << endl;
            _thread->join();
        }
    };
}

#endif //TILE_PROVIDER_TILELOADER_H
