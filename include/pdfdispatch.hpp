
#ifndef __RAYMON_SHAN_PDF_DISPATCH_H
#define __RAYMON_SHAN_PDF_DISPATCH_H

// this class in running in main thread, to schedule request, and send ipc msg to other process

#include "raymoncommon.hpp"
#include "raymonlog.hpp"
#include "raymonhelper.hpp"
#include "raymoncmdparser.hpp"
#include "raymonparser.hpp"
#include "raymonepoll.hpp"
#include "raymonipc.hpp"

//#define MAX_COMMAND_COUNT   16
typedef struct HashValue {
  time_t seconds;
  long mType;
} HashValue, *pHashValue;


unsigned long HttpResponse(int socket, char *httphead, char *httpbody, unsigned long bodysize) {
  char headbuffer[MAX_VALID_COMMAND_LENGTH];

  int headsize = snprintf(headbuffer, MAX_VALID_COMMAND_LENGTH, httphead, bodysize);
  if (WriteFinish(socket, headbuffer, headsize)) {
    return REQUEST_CLOSE;
  } else if (WriteFinish(socket, httpbody, bodysize)) {
    return REQUEST_CLOSE;
  }
  if (httphead != HTTP200) return REQUEST_CLOSE;
  return REQUEST_COMPLETE;;
}

unsigned long ErrorHandle(int socket, char *httphead, char *errormsg, char *cmd) {
  char httpbody[2 * MAX_VALID_COMMAND_LENGTH];
  int httpbodysize;
  httpbodysize = snprintf(httpbody, 2 * MAX_VALID_COMMAND_LENGTH, "%s : %s", errormsg, cmd);
  // ATTENTION snprintf returen REAL size without limit
  if (httpbodysize > 2 * MAX_VALID_COMMAND_LENGTH - 1) httpbodysize = 2 * MAX_VALID_COMMAND_LENGTH - 1;
  _LOG_WARN("%s", httpbody);
  return HttpResponse(socket, httphead, httpbody, httpbodysize);
}

typedef class __gnu_cxx::hash_map<std::string, HashValue> HashmapProcess, *pHashmapProcess;
typedef HashmapProcess::iterator iteratorProcess;

typedef class PDFDispatch : public InterfaceParser{
private:
  IPCServer *ipcserver;
  unsigned long processnum;
  unsigned long nowprocessnum;
  long idletimeout;
  pHelper helper;
  pHashmapProcess hashmap;
  
  unsigned long nextprocess(void) {    // mType = nextprocess, mType is 1 base
    unsigned long now = nowprocessnum;
    nowprocessnum++;
    if (nowprocessnum == processnum) nowprocessnum = 0;
    return now + 1;
  }
  
public:
  PDFDispatch(IPCServer * ipcserver, unsigned long processnum, long idletimeout, pHelper helper) {
    this->ipcserver = ipcserver;
    this->processnum = processnum;
    this->nowprocessnum = 0;
    this->idletimeout = idletimeout;
    this->helper = helper;
    hashmap = new HashmapProcess(INIT_HANDLE_NUMBER);
    //    this->cmdparser = cmdparser;
  }
  unsigned long HashmapNumber() {
    return hashmap->size();
  }
  
  virtual unsigned long Request(pEpollBuffer buffer) {
    char *requestprotocol = 0, *requestfile = 0, *requestpage = 0, *requestscale = 0, *requestquality = 0;
    char originmsg[MAX_VALID_COMMAND_LENGTH];
    
    char *start = buffer->bufferPointer;
    unsigned long length = buffer->bufferEnd;
    start[length] = 0;                       // MARK END
    char *end = strstr(start, HTTP_MASK);
    if (!end || strncmp(start, HTTP_GET,  sizeof(HTTP_GET) - 1) ||
	end - start > MAX_VALID_COMMAND_LENGTH || end - start < MIN_VALID_COMMAND_LENGTH ) {
      if (end) length = end - start;
      if (length > MAX_VALID_COMMAND_LENGTH - 1) length = length - 1;
      start[length] = 0;
      return ErrorHandle(buffer->socketFd, HTTP400, ERROR_UNKONW_CMD, start);
    }

    *end = 0;
    strncpy(originmsg, start, MAX_VALID_COMMAND_LENGTH - 1);
    start += sizeof(HTTP_GET) - 1;
    
    if (!strncmp(start, SERVICE_STATUS, sizeof(SERVICE_STATUS) - 1)) {
      char *retstatus = helper->ReturnHelper();
      return HttpResponse(buffer->socketFd, HTTP200, retstatus, strlen(retstatus));
    }
    if (!strncmp(start, SERVICE_FUNCTION, sizeof(SERVICE_STATUS) - 1)) {
      start += sizeof(SERVICE_FUNCTION) - 1;

      char *now, *lastnow;
      std::string hashkey;
      HashValue hashvalue;
      time_t seconds = time(NULL);
      iteratorProcess finded;
      PDFIPCMsg msg;
      int filelength = 0, protocollength = 0;
      
      now = lastnow = start;
      msg.scale = 100;
      msg.quality = 75;

      for (; now <= end; now++) {
	if (*now == '&' || now == end) {
	  *now = 0;
	  if (!strncmp(lastnow, REQUEST_PROTOCOL, sizeof(REQUEST_PROTOCOL) - 1)) {
	    requestprotocol = lastnow + sizeof(REQUEST_PROTOCOL) - 1;
	    protocollength =  now - lastnow - sizeof(REQUEST_PROTOCOL) + 2;  // with \0
	  }
	  if (!strncmp(lastnow, REQUEST_FILE, sizeof(REQUEST_FILE) - 1)) {
	    requestfile = lastnow + sizeof(REQUEST_FILE) - 1;
	    filelength = now - lastnow - sizeof(REQUEST_FILE) + 2;  // with \0
	  }
	  if (!strncmp(lastnow, REQUEST_PAGE, sizeof(REQUEST_PAGE) - 1)) {
	    requestpage = lastnow + sizeof(REQUEST_PAGE) - 1;
	    msg.page = atoi(requestpage);
	    if ((now - lastnow - sizeof(REQUEST_PAGE) + 1 > 5) || msg.page < -1 || msg.page > 99999) {
	      return ErrorHandle(buffer->socketFd, HTTP400, ERROR_INVALID_PAGE, originmsg);
	    }
	  }
	  if (!strncmp(lastnow, REQUEST_SCALE, sizeof(REQUEST_SCALE) - 1)) {
	    requestscale = lastnow + sizeof(REQUEST_SCALE) - 1;
	    msg.scale = atoi(requestscale);
	    if ((now - lastnow - sizeof(REQUEST_SCALE) + 1 > 4) || msg.scale < 1 || msg.scale > 9999) {
	      return ErrorHandle(buffer->socketFd, HTTP400, ERROR_INVALID_PARA, originmsg);	      
	    }
	  }
	  if (!strncmp(lastnow, REQUEST_QUALITY, sizeof(REQUEST_QUALITY) - 1)) {
	    requestquality = lastnow + sizeof(REQUEST_QUALITY) - 1;
	    msg.quality = atoi(requestquality);
	    if ((now -lastnow - sizeof(REQUEST_QUALITY) + 1 > 2) || msg.quality < 1 || msg.quality > 95) {
	      return ErrorHandle(buffer->socketFd, HTTP400, ERROR_INVALID_PARA, originmsg);	      
	    }
	  }
	  lastnow = now + 1;
	}
      }
      if (protocollength > MAX_VALID_PROTOCOL_LENGTH) {    // MAX size with \0
	return ErrorHandle(buffer->socketFd, HTTP400, ERROR_INVALID_PROTOCOL, originmsg);	
      }
      if (!requestfile || !requestprotocol || !requestpage) {
	return ErrorHandle(buffer->socketFd, HTTP400, ERROR_UNKONW_CMD, originmsg);
      }

      hashkey = requestprotocol;
      hashkey += requestfile;

      finded = hashmap->find(hashkey);  // finded is iterator
      if (finded != hashmap->end()) {
	hashvalue = finded->second;     // get all uint128
	msg.mType = hashvalue.mType;
	finded->second.seconds = seconds;
      } else {
	msg.mType = nextprocess();
	hashvalue.mType = msg.mType;
	hashvalue.seconds = seconds;
	hashmap->insert(std::pair<std::string, HashValue>(hashkey, hashvalue));
      }
      msg.socketFd = buffer->socketFd;
      strcpy(msg.protocol, requestprotocol);      
      strcpy(msg.filename, requestfile);
      ipcserver->Sendto(&msg, sizeof(PDFIPCMsg) - sizeof(long)/*mType*/ - MAX_VALID_COMMAND_LENGTH + filelength);
      return REQUEST_COMPLETE;
    }

    return ErrorHandle(buffer->socketFd, HTTP400, ERROR_UNKONW_CMD, originmsg);    
  }
  
  virtual unsigned long Idle (void) {
    iteratorProcess finded = hashmap->begin();
    iteratorProcess now;
    HashValue hashvalue;
    time_t seconds = time(NULL);
    std::string hashkey;
    PDFIPCMsg msg;

    while (finded != hashmap->end()) {
      now = finded;
      hashvalue = now->second;
      finded++;
      if (seconds - hashvalue.seconds > idletimeout) {
	hashkey = now->first;
	msg.mType = hashvalue.mType;
	msg.page = -1;
	strcpy(msg.protocol, PROTOCOL_CLOSE);
	strcpy(msg.filename, hashkey.c_str());
	ipcserver->Sendto(&msg, sizeof(PDFIPCMsg) - sizeof(long) - MAX_VALID_COMMAND_LENGTH + hashkey.length() + 1);
	hashmap->erase(now);
      }
    }
    return 0;
  }
} PDFDispatch, *pPDFDispatch;



#endif  // __RAYMON_SHAN_PDF_DISPATCH_H



