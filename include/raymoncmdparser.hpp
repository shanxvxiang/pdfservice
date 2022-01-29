
#ifndef __RAYMON_SHAN_CMD_PARA_H
#define __RAYMON_SHAN_CMD_PARA_H

// #include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>

#include "raymoncommon.hpp"
#include "raymontest.hpp"


typedef class CmdParser {
private:
  OnePara paras[CMD_PARA_MAX_PARA_NUMBER];
  unsigned long parasNum;
  int argc;
  char **argv;

  char *fileargv[CMD_PARA_MAX_PARA_NUMBER];
  char *filebuffer;
  long filesize;

public:
  CmdParser() {
    parasNum = 0;
    filebuffer = 0;
  }
  ~CmdParser() {
    if (filebuffer) free(filebuffer);
  }
  
  char *SetCmdBuffer(int argc, char** argv) {
    this->argv = argv;
    this->argc = argc;
    return 0;
  }
  char *SetCmdBuffer(char *argv0, char *parafile) {
    char *result;
    int cmdstatus = 0;
    
    this->argv = fileargv;
    argv[0] = argv0;
    argc = 1;

    result = LoadFileToBuffer(parafile, &filebuffer, &filesize);
    if (result) return result;

    for (char *now = filebuffer; now < filebuffer + filesize; now++) {
      if (cmdstatus == 0) {       // after \n wait for para or '#' for comment
	if (*now == ' ' || *now == '\r' || *now == '\n') continue;
	if (*now == '#') {
	  cmdstatus = 1;
	  continue;
	}
	fileargv[argc] = now;
	argc++;
	cmdstatus = 2;
	// cmd is begin
      } else if (cmdstatus == 1) {  // is comment
	if (*now == '\n') cmdstatus = 0;  // newline, wait for para
	continue;
      } else if (cmdstatus == 2) {
	if (*now == ' ' || *now == '\r' || *now == '\n' ) {
	  *now = 0;
	  cmdstatus = 0;
	}
	if (*now == '#') {
	  *now = 0;
	  cmdstatus = 1;
	}
	continue;
      }
    }
    return NULL;
  }

  // return 0 for OK, return 1 for out of range
  int SetCmdPara(char type, const char *key, const char *desc, void *value, void *value1 = NULL) {
    if (parasNum >= CMD_PARA_MAX_PARA_NUMBER) return 1;

    pOnePara nowpara = &paras[parasNum];
    nowpara->paraType = type;
    nowpara->paraKey = key;
    nowpara->paraKeyLen = strlen(key);
    nowpara->paraDesc = desc;
    nowpara->paraValue.v_addr = value;
    nowpara->paraValue1.v_addr = value1;

    parasNum ++;
    return 0;
  }
  int SetCmdParaChar(const char *key, const char *desc, char **value) {
    return SetCmdPara('c', key, desc, value);
  }
  int SetCmdParaLong(const char *key, const char *desc, long *value) {
    return SetCmdPara('l', key, desc, value);
  }
  int SetCmdParaStrings(const char *key, const char *desc, char **value, unsigned long *num) {
    return SetCmdPara('s', key, desc, value, num);
  }

  char *GetCmdPara() {
    unsigned long len;

    for (long j = 1; j < argc; j++) {
      for (unsigned i = 0; i < parasNum; i++) {
	len = paras[i].paraKeyLen;
	if (!strncmp(paras[i].paraKey, argv[j], len)) {
	  if (paras[i].paraType == 'c') *(paras[i].paraValue.pp_addr) = argv[j] + len;
	  if (paras[i].paraType == 'l') *(paras[i].paraValue.l_addr) = atol(argv[j]+len);
	  if (paras[i].paraType == 's') {
	    if (*(paras[i].paraValue1.l_addr) > 0) {
	      (*(paras[i].paraValue1.l_addr))--;
	      paras[i].paraValue.pp_addr[*(paras[i].paraValue1.l_addr)] = argv[j] + len;
	    }
	  }
	  break;
	}
	if (i == parasNum - 1) return argv[j];
      }
    }
    return 0;
  }

  void DisplayCmdPara(void) {
    pOnePara nowpara = paras;

    printf("%s Compiled %s %s V0.5%s\n", argv[0], __DATE__, __TIME__, __MY__VERSION__);
    for (unsigned i = 0; i < parasNum; i++) {
      printf("%s", nowpara->paraKey);
      for (int j = strlen(nowpara->paraKey); j < CMD_PARA_MAX_KEY_LENGTH; j++) printf(" ");
      printf("%s\n", nowpara->paraDesc);
      nowpara++;
    }
  }
} CmdParser, *pCmdParser;

#define DEFAULT_LISTEN_PORT    9000
#define DEFAULT_LOG_PATH       "log/"
#define DEFAULT_LOG_LEVEL      LEVEL_LOG_EROR
#define DEFAULT_PROCESS_NUMBER 0
#define DEFAULT_BUFFER_COUNT   (DEFAULT_PROCESS_NUMBER * 10)
#define DEFAULT_IDLE_WAIT      1000             // ms for epoll_wait, could not change by configure
#define DEFAULT_IDLE_TIMEOUT   10               // s for close idle pdf file
#define DEFAULT_IPC_NAME       "pdfservice.ipc" // could not change by configure
#define DEFAULT_LOW_MEMORY     100


typedef class PDFCmdPara {
private:
  int argc;
  char **argv;
public:
  char *protocolList[CMD_PARA_MAX_PARA_NUMBER];
  unsigned long protocolNumber;
  long listenPort;
  char *logPath;
  unsigned long logLevel;
  unsigned long processNumber;
  unsigned long epollBufferCount;
  unsigned long idleWait;
  unsigned long idleTimeout;
  char *ipcName;
  unsigned long lowMemory;
  
public:
  CmdParser cmdparser;

public:
  PDFCmdPara(int argc, char **argv) {
    this->argc = argc;
    this->argv = argv;

    memset(protocolList, 0, sizeof(protocolList));
    protocolNumber = CMD_PARA_MAX_PARA_NUMBER;  // get from end to begin
    listenPort = DEFAULT_LISTEN_PORT;
    logPath = (char*)DEFAULT_LOG_PATH;
    logLevel = DEFAULT_LOG_LEVEL;
    processNumber = DEFAULT_PROCESS_NUMBER;
    epollBufferCount = DEFAULT_BUFFER_COUNT;
    idleWait = DEFAULT_IDLE_WAIT;
    idleTimeout = DEFAULT_IDLE_TIMEOUT;
    ipcName = (char*)DEFAULT_IPC_NAME;
    lowMemory = DEFAULT_LOW_MEMORY;

    cmdparser.SetCmdParaStrings("protocol=", NULL, protocolList, &protocolNumber);
    cmdparser.SetCmdParaLong("listenport=", NULL, &listenPort);
    cmdparser.SetCmdParaChar("logpath=", NULL, &logPath);
    cmdparser.SetCmdParaLong("loglevel=", NULL, (long*)&logLevel);
    cmdparser.SetCmdParaLong("processnumber=", NULL, (long*)&processNumber);
    cmdparser.SetCmdParaLong("connectnumber=", NULL, (long*)&epollBufferCount);
    cmdparser.SetCmdParaLong("closeidle=", NULL, (long*)&idleTimeout);
    cmdparser.SetCmdParaLong("lowmemory=", NULL, (long*)&lowMemory);    
  }

  char *GetParaFromFile(void) {
    char *result;

    if (argc != 2) return ERROR_CMD_FORMAT;
    result = cmdparser.SetCmdBuffer(argv[0], argv[1]);
    if (result) return result;
    result = cmdparser.GetCmdPara();
    if (result) return result;

    if (processNumber == 0) {
      processNumber = get_nprocs();
    }
    if (epollBufferCount == 0) {
      epollBufferCount = processNumber * 50;
    }
    return NULL;    
  }

  char** GetProtocol(unsigned long *number) {
    *number = CMD_PARA_MAX_PARA_NUMBER - protocolNumber;
    return &protocolList[protocolNumber];
  }
  
} PDFCmdPara, *pPDFCmdPara;

#endif  // __RAYMON_SHAN_CMD_PARA_H

