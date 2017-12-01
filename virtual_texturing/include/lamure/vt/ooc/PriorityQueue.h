//
// Created by sebastian on 20.11.17.
//

#ifndef TILE_PROVIDER_PRIORITYQUEUE_H
#define TILE_PROVIDER_PRIORITYQUEUE_H

#include "lamure/vt/ooc/AbstractQueue.h"
#include "lamure/vt/ooc/TileRequest.h"

using namespace std;

namespace seb{

template<typename priority_type>
class PriorityQueueEntry;

template<typename priority_type>
class PriorityQueueContent{
protected:
    atomic<priority_type> _priority;
    atomic<PriorityQueueEntry<priority_type>*> _entry;

    friend class PriorityQueueEntry<priority_type>;

public:
    PriorityQueueContent(){
        _priority.store(0);
        _entry.store(nullptr);
    }

    virtual ~PriorityQueueContent(){
        remove();
    }

    virtual void remove(){
        auto entry = _entry.load();

        if(entry != nullptr) {
            entry->remove();
        }
    }

    virtual void setPriority(priority_type priority){
        _priority.store(priority);

        auto entry = _entry.load();

        if(entry != nullptr){
            entry->reinsert();
        }
    }

    virtual priority_type getPriority(){
        return _priority.load();
    }

    virtual PriorityQueueEntry<priority_type> *getEntry(){
        return _entry.load();
    }
};

template<typename priority_type>
class PriorityQueue;

template<typename priority_type>
class PriorityQueueEntry : public AbstractQueueEntry<PriorityQueueContent<priority_type>*>{
protected:
    mutex _deleteLock;
public:
    explicit PriorityQueueEntry(PriorityQueueContent<priority_type> *content, PriorityQueue<priority_type> &queue);

    ~PriorityQueueEntry() {
        //lock_guard<mutex> lock(_deleteLock);
        this->_content.load()->_entry.store(nullptr);
    }

    virtual priority_type getPriority(){
        return this->_content.load()->getPriority();
    }

    virtual void reinsert(){
        /*unique_lock<mutex> lock(_deleteLock, try_to_lock);

        if(!lock.owns_lock()){
            return;
        }*/

        ((PriorityQueue<priority_type>*)this->_queue.load())->reinsert(*this);
    }
};

    template<typename priority_type>
    PriorityQueueEntry<priority_type>::PriorityQueueEntry(PriorityQueueContent<priority_type> *content,
                                           PriorityQueue<priority_type> &queue) :
            AbstractQueueEntry<PriorityQueueContent<priority_type>*>(content, &queue){
        content->_entry.store(this);
    }

    template<typename priority_type>
class PriorityQueue : public AbstractQueue<PriorityQueueContent<priority_type>*>{
protected:
    virtual void _insertUnsafe(PriorityQueueEntry<priority_type> &entry){
        auto next = (PriorityQueueEntry<priority_type>*)this->_first.load();
        PriorityQueueEntry<priority_type> *prev = nullptr;

        while(next != nullptr){
            if(next->getPriority() >= entry.getPriority()){
                break;
            }

            prev = next;
            next = (PriorityQueueEntry<priority_type>*)next->getNext();
        }

        entry.setPrev(prev);
        entry.setNext(next);

        if(prev == nullptr){
            this->_first.store(&entry);
        }else{
            prev->setNext(&entry);
        }

        if(next == nullptr){
            this->_last.store(&entry);
        }else{
            next->setPrev(&entry);
        }
    }

public:
    virtual void reinsert(PriorityQueueEntry<priority_type> &entry){
        lock_guard<mutex> lock(this->_lock);

        try {
            this->_extractUnsafe(entry);
            this->_insertUnsafe(entry);
        }catch(exception &e){
            cout << e.what() << endl;
        }
    }

    virtual void push(PriorityQueueContent<priority_type> *&content){
        auto entry = new PriorityQueueEntry<priority_type>(content, *this);

        lock_guard<mutex> lock(this->_lock);

        this->_insertUnsafe(*entry);
        this->_incrementSize(1);

    }

    virtual bool pop(PriorityQueueContent<priority_type> *&content, const chrono::milliseconds maxTime){
        unique_lock<mutex> lock(this->_lock);

        this->_newEntry.wait_for(lock, maxTime, [this]{
            return this->_first.load() != nullptr;
        });

        auto entry = (PriorityQueueEntry<priority_type>*)this->_popBackUnsafe();

        if(entry == nullptr){
            return false;
        }

        content = entry->getContent();
        delete entry;

        this->_incrementSize(-1);

        return true;
    }

    virtual bool popLeast(PriorityQueueContent<priority_type> *&content, const chrono::milliseconds maxTime){
        unique_lock<mutex> lock(this->_lock);

        this->_newEntry.wait_for(lock, maxTime, [this]{
            return this->_last.load() != nullptr;
        });

        auto entry = (PriorityQueueEntry<priority_type>*)this->_popFrontUnsafe();

        if(entry == nullptr){
            return false;
        }

        content = entry->getContent();
        delete entry;

        this->_incrementSize(-1);

        return true;
    }

    bool contains(PriorityQueueContent<priority_type> *content){
        lock_guard<mutex> lock(this->_lock);

        for(auto entry = this->_first.load(); entry != nullptr; entry = entry->getNext()){
            if(entry->getContent() == content){
                return true;
            }
        }

        return false;
    }
};

}

#endif //TILE_PROVIDER_PRIORITYQUEUE_H
