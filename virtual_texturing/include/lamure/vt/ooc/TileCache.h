//
// Created by sebastian on 21.11.17.
//

#ifndef TILE_PROVIDER_TILECACHE_H
#define TILE_PROVIDER_TILECACHE_H

#include <map>
#include <iostream>
#include <iomanip>
#include "lamure/vt/ooc/AbstractBuffer.h"
#include "lamure/vt/ooc/TileRequest.h"

using namespace std;

namespace seb{

    typedef uint64_t id_type;
    typedef uint64_t time_type;

    class TileCacheContent : public PriorityQueueContent<time_type>{
    protected:
        id_type _id;
        AbstractBufferSlot<TileCacheContent*> *_slot;
        bool _empty;
    public:
        TileCacheContent() : PriorityQueueContent<time_type>() {
            _id = 0;
            _slot = nullptr;
            _empty = true;
        }

        void setId(id_type id){
            _id = id;
        }

        id_type getId(){
            return _id;
        }

        void setSlot(AbstractBufferSlot<TileCacheContent*> *slot){
            _slot = slot;
        }

        AbstractBufferSlot<TileCacheContent*> *getSlot(){
            return _slot;
        }

        void setEmpty(bool empty = true){
            _empty = empty;
        }

        bool isEmpty(){
            return _empty;
        }

        static TileCacheContent *empty(){
            return new TileCacheContent();
        }
    };

    class TileCache : public AbstractBuffer<TileCacheContent*>{
    protected:
        typedef time_type priority_type;
        typedef AbstractBufferSlot<TileCacheContent*> slot_type;

        alignas(CACHELINE_SIZE) PriorityQueue<priority_type> _free;

        alignas(CACHELINE_SIZE) mutex _newTileLock;
        alignas(CACHELINE_SIZE) condition_variable _newTile;

        alignas(CACHELINE_SIZE) mutex _idsLock;
        alignas(CACHELINE_SIZE) map<id_type, slot_type*> _ids;

    public:
        explicit TileCache(size_t slotSize, size_t slotCount) : AbstractBuffer<TileCacheContent*>(slotSize, slotCount) {
            for(size_t i = 0; i < this->_slotCount; ++i){
                auto slot = &this->_slots[i];
                auto content = TileCacheContent::empty();
                content->setSlot(slot);
                slot->setAssocData(content);

                auto temp = (PriorityQueueContent<priority_type>*)content;

                _free.push(temp);
            }
        }

        virtual void slotStateChange(slot_type *slot,
                                     typename slot_type::STATE oldState,
                                     typename slot_type::STATE newState) {

        }

        slot_type *getFreeSlot(chrono::milliseconds maxTime = chrono::milliseconds::zero()) {
            PriorityQueueContent<priority_type> *tile;

            if(!_free.popLeast(tile, maxTime)){
                return nullptr;
            }

            auto slot = ((TileCacheContent*)tile)->getSlot();

            if(!((TileCacheContent*)tile)->isEmpty()) {
                removeTileId(slot->getAssocData()->getId(), slot);
            }

            slot->_setState(slot_type::STATE::WRITING);
            ((TileCacheContent*)tile)->setEmpty();

            return slot;
        }

        virtual slot_type *getReadySlot(chrono::milliseconds maxTime){
            /*PriorityQueueContent<priority_type> *req;

            if(!_ready.pop(req, maxTime)){
                return nullptr;
            }

            return ((TileRequest<priority_type>*)req)->getSlot();*/

            return nullptr;
        }

        virtual void insertTileId(id_type id, slot_type *slot){
            lock_guard<mutex> lock(_idsLock);
            _ids.insert(pair<id_type, slot_type*>(id, slot));
        }

        virtual void removeTileId(id_type id, slot_type *slot){
            lock_guard<mutex> lock(_idsLock);
            typename map<id_type, slot_type*>::iterator iter = _ids.find(id);

            if(iter == _ids.end()){
                return;
            }

            if(iter->second == slot){
                _ids.erase(iter);
            }
        }

        virtual slot_type *getSlotByTileId(id_type id){
            lock_guard<mutex> lock(_idsLock);
            typename map<id_type, slot_type*>::iterator iter = _ids.find(id);

            if(iter == _ids.end()){
                return nullptr;
            }

            slot_type *slot = iter->second;
            auto req = slot->getAssocData();

            if(slot->_setState(slot_type::STATE::READING)){
                req->remove();
                return slot;
            }

            return nullptr;
        }

        virtual bool slotReady(slot_type *slot){
            if(AbstractBuffer<TileCacheContent*>::slotFree(slot)){
                auto tile = slot->getAssocData();
                auto content = (PriorityQueueContent<priority_type>*)tile;

                tile->setPriority(chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now().time_since_epoch()).count());

                //cout << tile->getId() << endl;

                insertTileId(tile->getId(), slot);
                _free.push(content);
                _newTile.notify_all();

                return true;
            }

            return false;
        }

        virtual bool slotFree(slot_type *slot){
            if(AbstractBuffer<TileCacheContent*>::slotFree(slot)){
                PriorityQueueContent<priority_type> *tile = slot->getAssocData();
                _free.push(tile);
                return true;
            }

            return false;
        }

        uint8_t *get(id_type id){
            auto slot = getSlotByTileId(id);

            if(slot == nullptr){
                return nullptr;
            }

            return slot->getPointer();
        }

        void unget(id_type id){
            lock_guard<mutex> lock(_idsLock);
            map<id_type, AbstractBufferSlot<TileCacheContent*>*>::iterator iter = _ids.find(id);

            if(iter == _ids.end()){
                return;
            }

            auto slot = iter->second;
            slotFree(slot);
        }

        void print(){
            cout << "TileCache" << endl;
            ios::fmtflags f( cout.flags() );
            for(size_t i = 0; i < this->_slotCount; ++i){
                auto slot = &this->_slots[i];
                cout << "[" << setw(sizeof(id_type) * 2) << slot->getAssocData()->getId() << "|" << (_free.contains(slot->getAssocData()) ? " " : "X") << "] ";

                if(i % 4 == 3){
                    cout << endl;
                }
            }
            cout.flags( f );
        }
    };

}

#endif //TILE_PROVIDER_TILECACHE_H
