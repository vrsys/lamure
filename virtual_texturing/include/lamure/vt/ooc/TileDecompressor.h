//
// Created by sebastian on 21.11.17.
//

#ifndef TILE_PROVIDER_TILEDECOMPRESSOR_H
#define TILE_PROVIDER_TILEDECOMPRESSOR_H

class TileBuffer;

#include <lamure/vt/ooc/TileBuffer.h>
#include <lamure/vt/ooc/TileCache.h>
#include <lamure/vt/ooc/TileRequest.h>
#include <thread>
#include <vector>

using namespace std;

namespace vt
{
template <typename priority_type>
class TileBuffer;

template <typename priority_type>
class TileRequest;

template <typename priority_type>
class TileDecompressor
{
  protected:
    alignas(CACHELINE_SIZE) TileBuffer<priority_type> *_inBuffer;
    alignas(CACHELINE_SIZE) TileCache *_outBuffer;
    thread *_thread;
    alignas(CACHELINE_SIZE) atomic<bool> _running;
    alignas(CACHELINE_SIZE) vector<thread *> _threads;

  public:
    TileDecompressor()
    {
        _inBuffer = nullptr;
        _outBuffer = nullptr;
        _thread = nullptr;
        _running = false;
    }

    ~TileDecompressor()
    {
        for(auto thread : _threads)
        {
            delete thread;
        }
    }

    void setInBuffer(TileBuffer<priority_type> *buffer) { _inBuffer = buffer; }

    void setOutBuffer(TileCache *buffer) { _outBuffer = buffer; }

    bool process(TileRequest<priority_type> *req)
    {
        auto inSlot = req->getSlot();
        auto outSlot = _outBuffer->getFreeSlot(chrono::milliseconds(100));

        if(outSlot == nullptr)
        {
            return false;
        }

        auto inPtr = inSlot->getPointer();
        auto outPtr = outSlot->getPointer();

        for(size_t i = 0; i < inSlot->getSize(); ++i)
        {
            outPtr[i] = inPtr[i];
        }

        req->setState(TileRequest<priority_type>::STATE::READY);
        outSlot->getAssocData()->setId(req->getId());
        outSlot->getAssocData()->setEmpty(false);

        delete req;

        _outBuffer->slotReady(outSlot);

        auto emptyReq = new TileRequest<priority_type>(0, 0, 0);
        emptyReq->setSlot(inSlot);
        inSlot->setAssocData(emptyReq);
        _inBuffer->slotFree(inSlot);

        return true;
    }

    void run()
    {
        AbstractBufferSlot<TileRequest<priority_type> *> *slot = nullptr;

        while(_running)
        {
            if(slot == nullptr)
            {
                slot = _inBuffer->getReadySlot(chrono::milliseconds(100));
            }

            if(slot != nullptr && slot->getAssocData() == nullptr)
            {
                cout << "A persisting Threading Bug just occured! :'O" << endl;
                slot = nullptr;
                continue;
            }

            if(slot == nullptr || !process(slot->getAssocData()))
            {
                continue;
            }

            slot = nullptr;
        }

        // cout << "FileReader terminated ..." << endl;
    }

    void start(size_t threadCount)
    {
        _running = true;

        for(size_t i = 0; i < threadCount; ++i)
        {
            _threads.push_back(new thread(&TileDecompressor::run, this));
        }

        // cout << "FileReader started ..." << endl;
    }

    void stop()
    {
        _running = false;
        // cout << "FileReader stopped ..." << endl;
        for(auto thread : _threads)
        {
            thread->join();
        }
    }
};
}

#endif // TILE_PROVIDER_TILEDECOMPRESSOR_H
