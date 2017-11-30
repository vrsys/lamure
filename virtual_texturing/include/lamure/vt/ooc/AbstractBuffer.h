//
// Created by sebastian on 21.11.17.
//

#ifndef TILE_PROVIDER_ABSTRACTBUFFER_H
#define TILE_PROVIDER_ABSTRACTBUFFER_H

#include <cstddef>
#include <mutex>
#include <atomic>

using namespace std;

namespace seb {

    template<typename T>
    class AbstractBuffer;

    template<typename assoc_data_type>
    class AbstractBufferSlot {
    public:
        enum STATE {
            FREE = 1,
            WRITING,
            READY,
            READING
        };

    protected:
        alignas(CACHELINE_SIZE) atomic<STATE> _state;

        alignas(CACHELINE_SIZE) uint8_t *_ptr;
        size_t _size;

        AbstractBuffer<assoc_data_type> *_buffer;

        atomic<assoc_data_type> _assocData;

        size_t _setAsscoData;

        AbstractBufferSlot() {
            _state.store(STATE::FREE);
            _ptr = nullptr;
            _size = 0;
            _buffer = nullptr;
            _setAsscoData = 0;
        }

        ~AbstractBufferSlot() = default;

        explicit AbstractBufferSlot(AbstractBuffer<assoc_data_type> &buffer) : AbstractBufferSlot() {
            _buffer = &buffer;
        }

        virtual void _setData(uint8_t *ptr, size_t size) {
            _ptr = ptr;
            _size = size;
        }

        virtual void setBuffer(AbstractBuffer<assoc_data_type> &buffer){
            _buffer = &buffer;
        }

        friend class AbstractBuffer<assoc_data_type>;

    public:
        virtual bool _setState(STATE state){
            bool stateSet = false;
            STATE exchangeState;

            switch(state){
                case STATE::FREE:
                    exchangeState = _state.exchange(STATE::FREE);
                    stateSet = true;
                    break;
                case STATE::WRITING:
                    exchangeState = STATE::FREE;
                    stateSet = _state.compare_exchange_weak(exchangeState, STATE::WRITING);
                    break;
                case STATE::READY:
                    exchangeState = STATE::WRITING;
                    stateSet = _state.compare_exchange_weak(exchangeState, STATE::READY);

                    if(!stateSet){
                        exchangeState = STATE::READING;
                        stateSet = _state.compare_exchange_weak(exchangeState, STATE::READY);
                    }

                    break;
                case STATE::READING:
                    if(_state.load() == STATE::READING){
                        exchangeState = STATE::READING;
                        stateSet = true;
                        break;
                    }

                    exchangeState = STATE::READY;
                    stateSet = _state.compare_exchange_weak(exchangeState, STATE::READING);

                    if(!stateSet){
                        exchangeState = STATE::FREE;
                        stateSet = _state.compare_exchange_weak(exchangeState, STATE::READING);
                    }

                    break;
            }

            if(stateSet){
                _buffer->slotStateChange(this, exchangeState, state);
            }

            return stateSet;
        }

        virtual AbstractBuffer<assoc_data_type> &getBuffer(){
            return *_buffer;
        }

        virtual STATE getState(){
            return _state.load();
        }

        virtual uint8_t *getPointer(){
            return _ptr;
        }

        size_t getSize(){
            return _size;
        }

        void setAssocData(assoc_data_type assocData){
            ++_setAsscoData;
            _assocData.store(assocData);
        }

        assoc_data_type getAssocData(){
            return _assocData.load();
        }

        size_t getSetCount(){
            return _setAsscoData;
        }

    };

    template<typename assoc_data_type>
    class AbstractBuffer {
    public:
        virtual void slotStateChange(AbstractBufferSlot <assoc_data_type> *slot, typename AbstractBufferSlot<assoc_data_type>::STATE oldState, typename AbstractBufferSlot<assoc_data_type>::STATE newState) = 0;

    protected:
        size_t _slotSize;
        size_t _slotCount;

        uint8_t *_data;
        AbstractBufferSlot<assoc_data_type> *_slots;

    public:
        AbstractBuffer(size_t slotSize, size_t slotCount){
            _slotSize = slotSize;
            _slotCount = slotCount;
            _data = new uint8_t[slotSize * slotCount];
            _slots = new AbstractBufferSlot<assoc_data_type>[slotCount];

            for(size_t i = 0; i < slotCount; ++i){
                auto slot = &_slots[i];

                slot->setBuffer(*this);
                slot->_setData(&_data[i * slotSize], slotSize);
            }


        }

        ~AbstractBuffer(){
            delete[] _data;
            delete[] _slots;
        }

        virtual AbstractBufferSlot<assoc_data_type> *getFreeSlot(chrono::milliseconds maxTime) = 0;

        virtual AbstractBufferSlot<assoc_data_type> *getReadySlot(chrono::milliseconds maxTime) = 0;

        virtual bool slotReading(AbstractBufferSlot<assoc_data_type> *slot){
            return slot->_setState(AbstractBufferSlot<assoc_data_type>::STATE::READING);
        }

        virtual bool slotReady(AbstractBufferSlot<assoc_data_type> *slot){
            return slot->_setState(AbstractBufferSlot<assoc_data_type>::STATE::READY);
        }

        virtual bool slotFree(AbstractBufferSlot<assoc_data_type> *slot){
            return slot->_setState(AbstractBufferSlot<assoc_data_type>::STATE::FREE);
        }

    };
}

#endif //TILE_PROVIDER_ABSTRACTBUFFER_H
