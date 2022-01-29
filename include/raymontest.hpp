
#ifndef __RAYMON_SHAN_TEST_H
#define __RAYMON_SHAN_TEST_H

#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

#include "raymoncommon.hpp"
#include "raymonlog.hpp"

typedef class TestTools {
private:
  timespec timelog;

public:
  void StartTimeCount() {
    clock_gettime(CLOCK_MONOTONIC, &timelog);
  };
  unsigned long EndTimeCount() {
    timespec end = timelog;
    clock_gettime(CLOCK_MONOTONIC, &timelog);
    return (timelog.tv_sec - end.tv_sec) * 1000000 + (timelog.tv_nsec - end.tv_nsec) / 1000;
  };
} TestTools, *pTestTools;


// following https://blog.csdn.net/djinglan/article/details/8726871
typedef class SignalHandle {
public:
  static void SignalParser(int signo) {
    void *signbuffer[30] = {0};
    size_t signbuffersize, i;
    char **stackstrings = NULL;

    printf("In process sign");
    signbuffersize = backtrace(signbuffer, 30);
    fprintf(stdout, "Signal %d handle, Obtained %zd stack frames\n", signo, signbuffersize);
    stackstrings = backtrace_symbols(signbuffer, signbuffersize);
    for (i = 0; i < signbuffersize; i++) {
      fprintf(stdout, "%s\n", stackstrings[i]);
    }
    free(stackstrings);
    exit(0);
  }

  SignalHandle() {
    if (signal(SIGSEGV, SignalParser) == SIG_ERR) {
      fprintf(stdout, "Error in Handle Signal SIGSEGV");
      exit(1);
    }
    if (signal(SIGILL, SignalParser) == SIG_ERR) {
      fprintf(stdout, "Error in Handle Signal SIGILL");
      exit(1);
    }
    if (signal(SIGTERM, SignalParser) == SIG_ERR) {
      fprintf(stdout, "Error in Handle Signal SIGTERM");
      exit(1);
    }
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {  // avoid closed by remote RST
      fprintf(stdout, "Error in Handle Signal SIGPIPE");
      exit(1);
    }

  }
} SignalHandle, *pSignalHandle;


unsigned long GetMeminfo() {
  struct sysinfo info;
  sysinfo(&info);
  //  printf("pdfium: sysinfo freeman: %ld\n", info.freeram);
  return info.freeram;
}

long WriteFinish(int socket, char *buffer, long size) {
  long writen = 0, once = 0;
  do {
    once = write(socket, buffer + writen, size - writen);
    if (once < 0 && errno != EAGAIN) {
      return -1;
    }
    if (once > 0) writen += once;
    if (writen == size) return 0;
    sleep(0);
  } while (1);
}

char *LoadFileToBuffer(char *filename, char **filebuffer, long *filesize) {
  struct stat statbuffer;
  int filehandle, fileread;
  
  if (stat(filename, &statbuffer)) return ERROR_IN_OPENFILE;
  *filesize = statbuffer.st_size;

  filehandle = open(filename, O_RDONLY);
  if (filehandle == -1) return ERROR_IN_OPENFILE;
  *filebuffer = (char*)malloc(*filesize);
  if (!(*filebuffer)) return ERROR_IN_MALLOC;
  fileread = read(filehandle, *filebuffer, *filesize);
  if (fileread != *filesize) {
    if (*filebuffer) free(*filebuffer);
    *filebuffer = 0;
    return ERROR_IN_READFILE;
  }
  return NULL;  
}


#endif  // __RAYMON_SHAN_TEST_H
