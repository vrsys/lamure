//
// Created by tihi6213 on 3/7/18.
//

#ifndef LAMURE_DOUBLEBUFFER_H
#define LAMURE_DOUBLEBUFFER_H

#include <lamure/vt/common.h>
namespace vt
{
template <typename T>
class DoubleBuffer
{
  public:
    explicit DoubleBuffer(T& front, T& back)
    {
        _front = front;
        _back = back;
        _new_data = false;
    }
    ~DoubleBuffer() {}

    T &get_front() { return _front; }
    T &get_back() { return _back; }

    void start_writing() { _back_lock.lock(); }
    void stop_writing()
    {
        if(_front_lock.try_lock())
        {
            deliver();
            _front_lock.unlock();

            _new_data = false;
        }
        else
        {
            _new_data = true;
        }

        _back_lock.unlock();
    }
    void start_reading()
    {
        _front_lock.lock();

        if(_back_lock.try_lock())
        {
            if(_new_data)
            {
                deliver();
                _new_data = false;
            }

            _back_lock.unlock();
        }
    }
    void stop_reading() { _front_lock.unlock(); }

  protected:
    T _front, _back;

    virtual void deliver();

  private:
    std::mutex _front_lock, _back_lock;
    bool _new_data;
};
}

#endif // LAMURE_DOUBLEBUFFER_H
