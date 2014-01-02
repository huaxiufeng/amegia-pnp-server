// Copyright (c) 2012-2013 Hua Xiufeng <huaxiufeng@kaisquare.com.cn>
// Copyright (c) 2013 KAI Square Software Limited
// All rights reserved
//
// Author: Hua Xiufeng

#ifndef BUFFER_QUEUE_H
#define BUFFER_QUEUE_H

#include "gloghelper.h"
#include <pthread.h>

template<class _TYPE>
class BufferQueue
{
public:
  BufferQueue(int num)
  {
    _size = 0;
    _max_size = num;
    if (_max_size <= 0) _max_size = 4096;
    _data = new _TYPE[_max_size];
    pthread_mutex_init(&_lock, NULL);
  }

  BufferQueue(const BufferQueue& bq)
  {
    _size = bq._size;
    _max_size = bq._max_size;
    _data = new _TYPE[_max_size];
    memcpy(_data, bq._data, _max_size);
    pthread_mutex_init(&_lock, NULL);
  }

  BufferQueue& operator= (const BufferQueue& bq)
  {
    if (this != &bq) {
      _size = bq._size;
      _max_size = bq._max_size;
      if (_data) delete []_data;
      _data = new _TYPE[_max_size];
      memcpy(_data, bq._data, _max_size);
    }
    return *this;
  }

  virtual ~BufferQueue(void)
  {
    delete []_data;
    pthread_mutex_destroy(&_lock);
  }

  const _TYPE* top()
  {
    return _data;
  }

  void pop(_TYPE *dst, int &len)
  {
    pthread_mutex_lock(&_lock);
    if (_size >= len && len > 0) {
      if (dst) {
        memcpy(dst, _data, len);
      }
      memmove(_data, _data+len, _size-len);
      _size -= len;
    }
    else {
      len = 0;
    }
    pthread_mutex_unlock(&_lock);
  }

  void push(const _TYPE *src, int len)
  {
    pthread_mutex_lock(&_lock);
    if (_size+len > _max_size) {
      _max_size *= 2;
      _TYPE *_new_data = new _TYPE[_max_size];
      memcpy(_new_data, _data, _size);
      delete []_data;
      _data = _new_data;
    }
    memcpy(_data+_size, src, len);
    _size += len;
    pthread_mutex_unlock(&_lock);
  }

  int size()
  {
    return _size;
  }

  int capacity()
  {
    return _max_size;
  }

  bool empty()
  {
    return _size == 0;
  }

  void clear() {
    _size = 0;
  }

protected:

  volatile int _size;     // used queue length
  volatile int _max_size; // max queue length
  _TYPE *_data;           // data buffer array
  pthread_mutex_t _lock;  // operation lock
};

#endif
