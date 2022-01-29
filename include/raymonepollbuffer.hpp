
#ifndef __RAYMON_SHAN_EPOLL_BUFFER_H
#define __RAYMON_SHAN_EPOLL_BUFFER_H

#include <stdlib.h>
#include "raymoncommon.hpp"
#include "raymonlog.hpp"
#include "raymonlockbuffer.hpp"


typedef class InterfaceBuffer {
public:
  virtual pEpollBuffer Get() = 0;
  virtual void Put(pEpollBuffer buffer) = 0;
} InterfaceBuffer, *pInterfaceBuffer;


typedef class SampleBuffer : public InterfaceBuffer {
public:
  virtual pEpollBuffer Get() {
    const unsigned long FIX_BUFFER_LENGTH = 1024 * 1000;
    pEpollBuffer buffer = (pEpollBuffer)malloc(FIX_BUFFER_LENGTH + sizeof(EpollBuffer));
    buffer->bufferPointer = (char*)(buffer + 1);
    buffer->bufferSize = FIX_BUFFER_LENGTH;
    buffer->bufferBegin = buffer->bufferEnd = 0;
    return buffer;
  }
  virtual void Put(pEpollBuffer buffer) {
    if (buffer) free(buffer);
  }
} SampleBuffer, *pSampleBuffer;


typedef class OwnBufferEpoll : public InterfaceBuffer{
private:
  LockBufferWithStruct<EpollBuffer> *ownBuffer;
  unsigned long bufferSize;

public:
  virtual pEpollBuffer Get(void) {
    pEpollBuffer buffer = ownBuffer->Get();
    if (buffer) {
      buffer->bufferPointer = (char*)(buffer + 1);
      buffer->bufferSize = OwnBufferEpoll::bufferSize;
      buffer->bufferBegin = buffer->bufferEnd = 0;
    }
    return buffer;
  }
  virtual void Put(pEpollBuffer buffer) {
    if (buffer) ownBuffer->Put(buffer);
  }
  OwnBufferEpoll(unsigned long connectnum, unsigned long buffersize) {
    bufferSize = buffersize;
    ownBuffer = new LockBufferWithStruct<EpollBuffer>(connectnum, buffersize);
    if (!ownBuffer) {
      //      _LOG_CRIT("SYSTEM BUFFER ALLOC ERROR");
      exit(1);
    }
  }
  ~OwnBufferEpoll() {
    delete ownBuffer;
  }
  unsigned long FreeNumber() {
    return ownBuffer->FreeNumber();
  }
  
} OwnBufferEpoll, *pOwnBufferEpoll;



#endif  // __RAYMON_SHAN_EPOLL_BUFFER_H
