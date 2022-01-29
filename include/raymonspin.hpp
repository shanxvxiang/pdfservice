
#ifndef __RAYMON_SHAN_SPIN_H
#define __RAYMON_SHAN_SPIN_H

#include "raymoncommon.hpp"

// every locker should in different cache line.
typedef class SpinLocker {
private:
  const static unsigned long _M_FREE = 0;
  const static unsigned long _M_BUSY = 1;
  
  volatile unsigned long locker;

public:
  SpinLocker(unsigned long l = _M_FREE) {
    locker = l;
  }
  void Lock(void) {
    while (!__sync_bool_compare_and_swap(&locker, _M_FREE, _M_BUSY));
  }
  void Unlock(void) {
    locker = _M_FREE;
  }
  void operator ++(int) {
    Lock();
  }
  void operator --(int) {
    Unlock();
  }
} __attribute__ ((aligned(CACHE_LINE_SIZE))) SpinLocker;


// same reason for avoid memory barrier
typedef class SpinLong {
 private:
  volatile unsigned long value;
  
 public:
  SpinLong(unsigned long l = 0) {
    value = l;
  }
  
  operator unsigned long() {
    return value;
  }
  void operator = (unsigned long v) {
    value = v;
  }
  unsigned long operator ++(int) {
    return __sync_fetch_and_add(&value, 1);
  }
  unsigned long operator --(int) {
    return __sync_fetch_and_sub(&value, 1);
  }  
    
  unsigned long operator +=(unsigned long v) {
    return __sync_fetch_and_add(&value, v);
  }
  unsigned long operator -=(unsigned long v) {
    return __sync_fetch_and_sub(&value, v);
  }
  unsigned long operator &=(unsigned long v) {
    return __sync_fetch_and_and(&value, v);
  }
  unsigned long operator |=(unsigned long v) {
    return __sync_fetch_and_or(&value, v);
  }
  unsigned long operator ^=(unsigned long v) {
    return __sync_fetch_and_xor(&value, v);
  }
  void cas(unsigned long oldvalue, unsigned long newvalue) {
    while (!__sync_bool_compare_and_swap(&value, oldvalue, newvalue));
    return;
  }
} __attribute__ ((aligned(CACHE_LINE_SIZE))) SpinLong;


#endif  // __RAYMON_SHAN_SPIN_H
