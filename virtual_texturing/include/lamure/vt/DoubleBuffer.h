//
// Created by towe2387 on 12/10/17.
//

#ifndef LAMURE_DOUBLEBUFFER_H
#define LAMURE_DOUBLEBUFFER_H

#include <cstddef>
#include <cstdint>
#include <atomic>
#include <mutex>

class DoubleBuffer{
protected:
    size_t _size;

    std::mutex _frontLock;
    uint8_t *_front;

    std::mutex _backLock;
    uint8_t *_back;
    bool _newData;

    void _swap(){
        auto ptr = _back;
        _back = _front;
        _front = ptr;

        memcpy(_back, _front, _size);
    }

public:
    DoubleBuffer(size_t size){
        _size = size;
        _front = new uint8_t[size];
        _back = new uint8_t[size];
        _newData = false;
    }

    ~DoubleBuffer(){
        delete[] _front;
        delete[] _back;
    }

    uint8_t *startWriting(){
        _backLock.lock();

        return _back;
    }

    void stopWriting(){
        if(_frontLock.try_lock()){
            _swap();
            _frontLock.unlock();

            _newData = false;
        }else{
            _newData = true;
        }

        _backLock.unlock();
    }

    uint8_t *startReading(){
        _frontLock.lock();

        if(_backLock.try_lock()){
            if(_newData){
                _swap();
                _newData = false;
            }

            _backLock.unlock();
        }

        return _front;
    }

    void stopReading(){
        _frontLock.unlock();
    }

    size_t getSize(){
        return _size;
    }

};

#endif //LAMURE_DOUBLEBUFFER_H
