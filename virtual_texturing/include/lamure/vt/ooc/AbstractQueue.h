//
// Created by sebastian on 20.11.17.
//

#ifndef TILE_PROVIDER_ABSTRACTQUEUE_H
#define TILE_PROVIDER_ABSTRACTQUEUE_H

#include <cstddef>
#include <atomic>
#include <mutex>
#include <condition_variable>

using namespace std;

namespace seb {

    template<typename content_type>
    class AbstractQueue;

    template<typename content_type>
    class AbstractQueueEntry {
    protected:
        atomic<AbstractQueueEntry<content_type> *> _prev;
        atomic<AbstractQueueEntry<content_type> *> _next;

        atomic<AbstractQueue<content_type> *> _queue;
        atomic<content_type> _content;

        friend class AbstractQueue<content_type>;

    public:
        explicit AbstractQueueEntry(content_type content, AbstractQueue<content_type> *queue) {
            _content.store(content);
            _queue.store(queue);
        }

        virtual ~AbstractQueueEntry() {

        }

        virtual void setPrev(AbstractQueueEntry<content_type> *prev) {
            _prev.store(prev);
        }

        virtual void setNext(AbstractQueueEntry<content_type> *next) {
            _next.store(next);
        }

        virtual AbstractQueueEntry<content_type> *getPrev() {
            return _prev.load();
        }

        virtual AbstractQueueEntry<content_type> *getNext() {
            return _next.load();
        }

        virtual content_type getContent() {
            return _content.load();
        }

        virtual void remove() {
            _queue.load()->remove(*this);
        }

        virtual AbstractQueue<content_type> *getQueue(){
            return _queue.load();
        }
    };

    template<typename content_type>
    class AbstractQueue {
    protected:
        alignas(CACHELINE_SIZE) condition_variable _newEntry;

        alignas(CACHELINE_SIZE) mutex _lock;
        atomic<size_t> _size;
        atomic<AbstractQueueEntry<content_type> *> _first;
        atomic<AbstractQueueEntry<content_type> *> _last;

        virtual void _extractUnsafe(AbstractQueueEntry<content_type> &entry) {
            auto prev = entry.getPrev();
            auto next = entry.getNext();

            // remove
            if (prev == nullptr) {
                _first.store(next);
            } else {
                prev->setNext(next);
            }

            if (next == nullptr) {
                _last.store(prev);
            } else {
                next->setPrev(prev);
            }
        }

        virtual AbstractQueueEntry<content_type> *_popFrontUnsafe() {
            auto first = _first.load();

            if (first != nullptr) {
                _extractUnsafe(*first);
            }

            return first;
        }

        virtual AbstractQueueEntry<content_type> *_popBackUnsafe() {
            auto last = _last.load();

            if (last != nullptr) {
                _extractUnsafe(*last);
            }

            return last;
        }

        void _incrementSize(int8_t add){
            _size.store(_size.load() + add);
        }

    public:
        AbstractQueue() {
            _size.store(0);
            _first.store(nullptr);
            _last.store(nullptr);
        }

        virtual ~AbstractQueue() {
            auto entry = _first.load();

            while (entry != nullptr) {
                auto next = entry->getNext();
                delete entry;
                entry = next;
            }
        }

        virtual void push(content_type &content) = 0;

        virtual bool pop(content_type &content, const chrono::milliseconds maxTime) = 0;

        size_t getSize() {
            return _size.load();
        }

        virtual void remove(AbstractQueueEntry<content_type> &entry) {
            lock_guard<mutex> lock(_lock);
            _extractUnsafe(entry);
            delete &entry;
            _incrementSize(-1);
        }
    };
}

#endif //TILE_PROVIDER_ABSTRACTQUEUE_H
