
#ifndef __RAYMON_SHAN_PARSER_H
#define __RAYMON_SHAN_PARSER_H

#include <stdio.h>
#include <string.h>
#include "raymoncommon.hpp"

typedef class InterfaceParser {
public:
  virtual unsigned long Request(pEpollBuffer buffer) = 0;
  virtual unsigned long Idle (void) = 0;
} InterfaceParser, *pInterfaceParser;

typedef class SampleParser {
public:
  virtual unsigned long Request(pEpollBuffer buffer) {
    unsigned long now;

    for (now = buffer->bufferBegin; now < buffer->bufferEnd; now++) {
      printf("%02x ", buffer->bufferPointer[now] & 0xff);
      if (buffer->bufferPointer[now] == '\n') {
      // command complate, set signal to run other thread/process
	buffer->bufferPointer[now + 1] = 0;
	printf("\nIn Sample Complate: %s\n", buffer->bufferPointer);
	//	buffer->bufferBegin = buffer->bufferEnd = 0;

	if (!strncmp(buffer->bufferPointer, "close", 5)) return REQUEST_CLOSE;    // close socket from server side
	else return REQUEST_COMPLETE;
      }
    }
  // command is incomplate, shold read from socket again
    buffer->bufferPointer[now + 1] = 0;
    printf("\nIn Sample Incomplate: %s\n", buffer->bufferPointer);
  //buffer->bufferBegin = buffer->bufferEnd;
    return REQUEST_CONTINUE;
  }
  virtual unsigned long Idle (void) {
    printf(".");
    return 0;
  }
} SampleParser, *pSampleParser;

#endif  // __RAYMON_SHAN_PARSR_H
