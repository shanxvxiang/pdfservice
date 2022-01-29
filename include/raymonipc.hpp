
#ifndef __RAYMON_SHAN_IPC_H
#define __RAYMON_SHAN_IPC_H

#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/msg.h>
#include <unistd.h>

#include "raymoncommon.hpp"
#include "raymonlog.hpp"
#include "raymoncmdparser.hpp"
#include "pdfprocess.hpp"


typedef class IPCServer {
private:
  key_t ipckey;
  int ipcmsqid;
  pPDFCmdPara cmdparser;
  pInterfaceProcess process;
  
public:
  int Sendto(void* msgbuffer, unsigned long msglength) {   // msglength not include mType 8 byte
    int ret;
    if ((ret = msgsnd(ipcmsqid, msgbuffer, msglength, 0)) == -1) {
      _LOG_EROR("send ipc message error, errno:%d, %s", errno, strerror(errno));
    }
    return ret;
  }

  static int Sendto(int ipcid, pPDFIPCMsg msgbuffer, unsigned long msglength) {
    int ret;
    if ((ret = msgsnd(ipcid, msgbuffer, msglength, 0)) == -1) {
      _LOG_EROR("send ipc message error, errno:%d, %s", errno, strerror(errno));
    }
    return ret;    
  }
  
  /*
  int Sendto(unsigned long msgtype, char *msg, unsigned long msglength) {   // msgtype base 1
    PDFIPCMsg msgbuffer;
    if (msglength > IPC_MSG_MAX_LENGTH) return -1;
    msgbuffer.mType = msgtype;
    memcpy(&(msgbuffer.socketFd), msg, msglength);    // socketFd is the place for message
    return Sendto(&msgbuffer, msglength);
  }
  */

  static int ReceiveFrom(int ipcid, /*pIPCMsgBuffer*/ pPDFIPCMsg msgbuffer, long msgtype) {
    int ret;
    if ((ret = msgrcv(ipcid, msgbuffer, sizeof(PDFIPCMsg) - sizeof(long), msgtype, 0)) == -1 ) {
      _LOG_EROR("receive ipc message error, errno:%d, %s", errno, strerror(errno));
    }
    return ret;
  }

  static int IPCProcess(void* para) {
    //    IPCMsgBuffer readmsg;
    PDFIPCMsg readmsg;
    int readmsqid;
    key_t readkey;
    int msgreaded;
    char **cmdprotocol;
    unsigned long protocolnumber;
    long masterType; // mType of dispatch

    //char logpath[] = "log/";
    //char fileprefix[IPC_FILENAME_MAX];

    pIPCProcessPara ipcpara = (pIPCProcessPara)para;
    InterfaceProcess *process = ipcpara->process;

    //snprintf(fileprefix, IPC_FILENAME_MAX, "process%03ld",  ipcpara->ipcType);
    //LogHandle log(logpath, fileprefix, 5);

    if ((readkey = ftok(ipcpara->ipcName, 'B')) == -1 || (readmsqid = msgget(readkey, IPC_MSQ_PERMS)) == -1) {
      _LOG_ATTE("CRITICAL ERROR in create ipc, exit ! errno:%d, %s", errno, strerror(errno));
      // should send back to main process to notify
      exit(1);
    }
    cmdprotocol = ipcpara->cmdparser->GetProtocol(&protocolnumber);
    if (protocolnumber) {
      for (unsigned long i = 0; i < protocolnumber; i++) {
	//	printf("protocol name %s\n", cmdprotocol[i]);
	if (process->Initialize(cmdprotocol[i])) {
	  // should send back to main process to notify
	  exit(1);
	}
      }
    }
    while (1) { // initialize read msg from dispatch
      msgreaded = ReceiveFrom(readmsqid, &readmsg, ipcpara->ipcType);
      if (msgreaded > 0) {
	if (strcmp(readmsg.protocol, PROTOCOL_INITIALIZE)) continue;    // passby old CLOSE msg
	masterType = readmsg.socketFd;

	readmsg.mType = masterType;
	readmsg.socketFd = ipcpara->ipcType;
	Sendto(readmsqid, &readmsg, sizeof(PDFIPCMsg) - sizeof(long));
	_LOG_ATTE("PROCESS %ld OK ! listen for ipc", ipcpara->ipcType);
	break;
      }
    }

    while (1) {
      msgreaded = ReceiveFrom(readmsqid, &readmsg, ipcpara->ipcType);
      //      printf("2 %s\n", readmsg.protocol);      
      if (msgreaded > 0) {
	_LOG_TRAC("process %ld received from ipc %d byte", ipcpara->ipcType, msgreaded);
	// _LOG_TRAC_DUMP(readmsg.mText);
	readmsg.mType = msgreaded;    // lazy way for save space
	process->Process(&readmsg);
      }
    }
  }
  
  IPCServer(char *ipcname, unsigned long processnum, pPDFCmdPara cmdparser, pInterfaceProcess process) {
    char *stack, *stacktop;
    pIPCProcessPara ipcpara;

    this->cmdparser = cmdparser;
    this->process = process;
    PDFIPCMsg msg;
    int msgreaded;

    if (strlen(ipcname) >= IPC_FILENAME_MAX) {
      _LOG_ATTE("CRITICAL ERROR ipcname TOO LONG > %d", IPC_FILENAME_MAX - 1);
      exit(1);
    }
    int ipcfile = open(ipcname, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
    if (ipcfile < 0) {
      _LOG_ATTE("CRITICAL ERROR CREATE IPC FILE: %s, errno:%d, %s", ipcname, errno, strerror(errno));
      exit(1);
    }
    close(ipcfile);

    if ((ipckey = ftok(ipcname, 'B')) == -1 || (ipcmsqid = msgget(ipckey, IPC_MSQ_PERMS | IPC_CREAT)) == -1) {
      _LOG_ATTE("CRITICAL ERROR CREATE IPC %s, errno:%d, %s", ipcname, errno, strerror(errno));
      exit(1);
    }
    _LOG_DBUG("create ipc ok");

    ipcpara = new IPCProcessPara[processnum];
    if (ipcpara == 0) {
      _LOG_ATTE("CRITICAL ERROR new ipcpara");
      exit(1);
    }

    for (unsigned long i = 0; i < processnum; i++) {
      msg.mType = i + 1;
      msg.socketFd = processnum + 1;       // here socketFd is the mType send back to dispatch
      strcpy(msg.protocol, PROTOCOL_INITIALIZE);
      Sendto(&msg, sizeof(PDFIPCMsg) - sizeof(long)/*mType*/ - MAX_VALID_COMMAND_LENGTH);
      
      stack = (char*)mmap(NULL, IPC_PROCESS_STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
      if (stack == MAP_FAILED) {
	_LOG_ATTE("CRITICAL ERROR mmap FOR PROCESS. errno:%d, %s", errno, strerror(errno));
	exit(1);
      }
      stacktop = stack + IPC_PROCESS_STACK_SIZE;
      strncpy(ipcpara[i].ipcName, ipcname, IPC_FILENAME_MAX);
      ipcpara[i].ipcType = i + 1;                             // for 0 is all type, type from 1
      ipcpara[i].cmdparser = this->cmdparser;
      ipcpara[i].process = process;
      ipcpara[i].ipcPid = clone(IPCProcess, stacktop,  CLONE_FILES | CLONE_FS, &ipcpara[i]);
      if (ipcpara[i].ipcPid == -1) {
	_LOG_ATTE("CRITICAL ERROR clone PROCESS %ld", i);
	exit(1);
      }
      while (1) {
	msgreaded = ReceiveFrom(ipcmsqid, &msg, processnum + 1);
	if (msgreaded > 0) {
	  if (strcmp(msg.protocol, PROTOCOL_INITIALIZE)) continue;
	  if (msg.socketFd < 0) {
	    _LOG_ATTE("HALT create process fail, system quit !!!");
	    exit(1);
	  }
	  break;
	}
      }
      usleep(1000);
    }
  }
  
  ~IPCServer() {
    msgctl(ipcmsqid, IPC_RMID, NULL);
  }

} IPCServer, *pIPCServer;

#endif  // __RAYMON_SHAN_IPC_H
