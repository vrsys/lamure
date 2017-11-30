#ifndef TILE_PROVIDER_TILEBUFFER_H
#define TILE_PROVIDER_TILEBUFFER_H

#include <map>
#include "lamure/vt/ooc/AbstractBuffer.h"
#include "lamure/vt/ooc/TileRequest.h"

using namespace std;

namespace seb{

    template<typename priority_type>
    class TileBuffer : public AbstractBuffer<TileRequest<priority_type>*>{
    protected:
        typedef TileRequest<priority_type> request_type;
        typedef AbstractBufferSlot<request_type*> slot_type;

        alignas(CACHELINE_SIZE) PriorityQueue<priority_type> _free;
        alignas(CACHELINE_SIZE) PriorityQueue<priority_type> _ready;

        alignas(CACHELINE_SIZE) mutex _idsLock;
        alignas(CACHELINE_SIZE) map<id_type, slot_type*> _ids;

    public:
        TileBuffer(size_t slotSize, size_t slotCount) : AbstractBuffer<request_type*>(slotSize, slotCount) {
            for(size_t i = 0; i < this->_slotCount; ++i){
                auto slot = &this->_slots[i];
                request_type *emptyReq = new request_type(0, -1, -1);
                emptyReq->setSlot(slot);
                auto content = (PriorityQueueContent<priority_type>*)emptyReq;
                slot->setAssocData(emptyReq);

                _free.push(content);
            }
        }

        virtual void slotStateChange(slot_type *slot,
                                     typename slot_type::STATE oldState,
                                     typename slot_type::STATE newState) {

        }

        virtual slot_type *getFreeSlot(chrono::milliseconds maxTime) {
            PriorityQueueContent<priority_type> *req;

            if(!_free.pop(req, maxTime)){
                return nullptr;
            }

            auto slot = ((request_type*)req)->getSlot();
            removeTileId(slot->getAssocData()->getId(), slot);
            slot->_setState(slot_type::STATE::WRITING);
            delete req;

            return slot;
        }

        virtual slot_type *getReadySlot(chrono::milliseconds maxTime){
            PriorityQueueContent<priority_type> *req;

            if(!_ready.pop(req, maxTime)){
                return nullptr;
            }

            return ((TileRequest<priority_type>*)req)->getSlot();
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

            if(slot->_setState(slot_type::STATE::WRITING)){
                delete req;
                slot->setAssocData(nullptr);

                return slot;
            }

            return nullptr;
        }

        virtual bool slotReady(slot_type *slot){
            if(AbstractBuffer<request_type*>::slotReady(slot)){
                auto req = slot->getAssocData();
                auto content = (PriorityQueueContent<priority_type>*)req;

                insertTileId(req->getId(), slot);

                _ready.push(content);

                return true;
            }

            return false;
        }

        virtual bool slotFree(slot_type *slot){
            if(AbstractBuffer<request_type*>::slotFree(slot)){
                PriorityQueueContent<priority_type> *req = slot->getAssocData();
                _free.push(req);
                return true;
            }

            return false;
        }
    };

}

#endif
