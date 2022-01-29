
#ifndef __RAYMON_SHAN_LOCK_BUFFER_H
#define __RAYMON_SHAN_LOCK_BUFFER_H

#include <malloc.h>

#ifdef  __RAYMON_SHAN_TEST
#include <stdio.h>
#endif  //__RAYMON_SHAN_TEST

#include "raymoncommon.hpp"
#include "raymonspin.hpp"

#ifndef __RAYMON_SHAN_BUFFER_DEFINE
#define __RAYMON_SHAN_BUFFER_DEFINE
typedef struct BufferList{
  struct BufferList *freeNext;
  void *pad;
} BufferList, *pBufferList;
#endif  // __RAYMON_SHAN_BUFFER_DEFINE


typedef class LockBuffer{
private:
  SpinLocker BUFFERLOCK;
  
  void *freeAddress;                            // for malloc and free
  volatile pBufferList freeStart;
  unsigned long buffernumber;
  unsigned long buffersize;
  unsigned long realsize;

  SpinLong freeNumber;
  
public:
  LockBuffer(unsigned long number, unsigned long size) {
    buffernumber = number;
    buffersize = size;
    realsize = ((sizeof(BufferList) + size) / CACHE_LINE_SIZE + 1) * CACHE_LINE_SIZE;
    freeAddress = malloc(number * realsize + CACHE_LINE_SIZE);
    freeStart = (pBufferList)freeAddress;
    freeNumber = number;

    char *nowbuffer = (char*)freeAddress;
    char *nextbuffer = nowbuffer + realsize;
    for (unsigned i = 0; i < number - 1; i++) {
      ((pBufferList)nowbuffer)->freeNext = (pBufferList)nextbuffer;
      nowbuffer = nextbuffer;
      nextbuffer += realsize;
    }
    ((pBufferList)nowbuffer)->freeNext = 0;
  };
  ~LockBuffer() {
    free(freeAddress);
  };

  void* Get(void) {
    pBufferList node;
    
    BUFFERLOCK ++;
    node = freeStart;
    if (node->freeNext) {
      freeStart = node->freeNext;
      BUFFERLOCK --;
      freeNumber --;
      return node + 1;
    } else {
      BUFFERLOCK --;
      return 0;
    }
  };

  void Put(void* buf) {
    pBufferList node;
    if (!buf) return;

    node = (pBufferList)buf - 1;
    freeNumber ++;    
    BUFFERLOCK ++;
    node->freeNext = freeStart;
    freeStart = node;
    BUFFERLOCK --;
  };

  unsigned long FreeNumber() {
    return freeNumber;
  }

#ifdef  __RAYMON_SHAN_TEST
  void Display(unsigned long maxvalue = 20) {
    pBufferList now = freeStart;
    unsigned i = 0;

    while (now) {
      printf("%p -> ", now);
      now = now->freeNext;
      if (i++ > maxvalue) break;
    }
    printf(" 0\nFree Number: %ld\n", FreeNumber());
} 
#endif  //__RAYMON_SHAN_TEST
  
} LockBuffer, *pLockBuffer;


template <typename _S>
class LockBufferWithStruct {
  pLockBuffer plockbuffer;

public:
  LockBufferWithStruct(unsigned long number, unsigned long addition) {
    plockbuffer = new LockBuffer(number, sizeof(_S) + addition);
  }
  ~LockBufferWithStruct() {
    delete plockbuffer;
  }
  _S* Get(void) {
    return (_S*)plockbuffer->Get();
  }
  void Put(_S *s) {
    plockbuffer->Put((void*) s);
  }
  unsigned long FreeNumber() {
    return plockbuffer->FreeNumber();
  }
};

#endif  // __RAYMON_SHAN_LOCK_BUFFER_H

