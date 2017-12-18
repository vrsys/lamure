//
// Created by sebastian on 23.11.17.
//

#ifndef TILE_PROVIDER_TILEATLAS_H
#define TILE_PROVIDER_TILEATLAS_H

#include <cstdint>
#include <lamure/vt/ooc/Observer.h>
#include <lamure/vt/ooc/TileBuffer.h>
#include <lamure/vt/ooc/TileDecompressor.h>
#include <lamure/vt/ooc/TileLoader.h>
#include <lamure/vt/ooc/TileRequest.h>
#include <map>
#include <string>

using namespace std;

namespace vt
{
typedef uint64_t id_type;

template <typename priority_type>
class TileRequest;

template <typename priority_type>
class TileLoader;

template <typename priority_type>
class TileAtlas : public Observer
{
  protected:
    mutex _requestsLock;
    map<id_type, TileRequest<priority_type> *> _requests;
    TileLoader<priority_type> _loader;
    TileBuffer<priority_type> _buffer;
    TileDecompressor<priority_type> _decompressor;
    TileCache _cache;
    ifstream _fileStream;
    size_t _tileSize;

    mutex _emptyLock;
    condition_variable cond;
    chrono::high_resolution_clock::time_point _createdAt;

  public:
    explicit TileAtlas(string &fileName, size_t tileSize) : _buffer(tileSize, 64), _cache(tileSize, 128), _fileStream(fileName)
    {
        _tileSize = tileSize;
        _loader.setFileStream(&_fileStream);
        _loader.setBuffer(&_buffer);

        _decompressor.setInBuffer(&_buffer);
        _decompressor.setOutBuffer(&_cache);

        _loader.start();
        _decompressor.start(1);

        _createdAt = chrono::high_resolution_clock::now();
    }

    ~TileAtlas()
    {
        _loader.stop();
        _decompressor.stop();
    }

    bool alreadyRequested(id_type id){
        auto ptr = _cache.get(id);

        if(ptr != nullptr)
        {
            return true;
        }

        lock_guard<mutex> lock(_requestsLock);

        return _requests.find(id) != _requests.end();
    }

    uint8_t *get(id_type id, priority_type priority)
    {
        auto ptr = _cache.get(id);

        if(ptr != nullptr)
        {
            return ptr;
        }

        unique_lock<mutex> lock(_requestsLock);

        if(_requests.find(id) != _requests.end())
        {
            return nullptr;
        }

        lock.unlock();

        auto req = new TileRequest<priority_type>(id, id * _tileSize, _tileSize);

        insert(req);

        _loader.read(req);

        return nullptr;
    }

    void unget(id_type id) { _cache.unget(id); }

    void insert(TileRequest<priority_type> *req)
    {
        lock_guard<mutex> lock(_requestsLock);

        req->observe(TileRequest<priority_type>::EVENT::DELETED, this);
        _requests.insert(pair<id_type, TileRequest<priority_type> *>(req->getId(), req));
    }

    void remove(TileRequest<priority_type> *req)
    {
        lock_guard<mutex> lock(_requestsLock);

        auto iter = _requests.find(req->getId());

        if(iter != _requests.end())
        {
            _requests.erase(iter);
        }

        if(_requests.empty())
        {
            cond.notify_one();
        }
    }

    void inform(event_type event, Observable *observable)
    {
        auto req = (TileRequest<priority_type> *)observable;
        // cout << req->getId() << " deleted after " << chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now() - req->getCreatedAt()).count() << " microsecs" << endl;
        remove(req);
    }

    void wait()
    {
        unique_lock<mutex> lock(_emptyLock);

        cond.wait(lock, [this] { return _requests.empty(); });

        cout << "finished in " << chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - _createdAt).count() << " ms" << endl;
    }
};
}

#endif // TILE_PROVIDER_TILEATLAS_H
