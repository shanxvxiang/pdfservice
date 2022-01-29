// https://www.boost.org/doc/libs/master/doc/html/interprocess/sharedmemorybetweenprocesses.html  share memory boost
// https://www.tutorialspoint.com/inter_process_communication/inter_process_communication_message_queues.htm  inter process


// #define _GLIBCXX_USE_CXX11_ABI 1			// https://zhuanlan.zhihu.com/p/142876490

#include "include/raymoncommon.hpp"
#include "include/raymonlog.hpp"
#include "include/raymonhelper.hpp"
#include "include/raymonlockbuffer.hpp"
#include "include/raymonepoll.hpp"
#include "include/raymonipc.hpp"
#include "include/pdfdispatch.hpp"
#include "include/pdfprocess.hpp"


class PDFService {
private:

  friend class Helper;
  pPDFCmdPara parser;
  pHelper helper;
  pPDFProcess pdfprocess;
  pIPCServer ipcserver;
  pPDFDispatch pdfdispatch;
  pOwnBufferEpoll ownbufferepoll;
  pWriteSeparateEpoll epollservice;

public:
  PDFService() {

  }
  unsigned long Initialize(int argc, char **argv) {
    char *result;
    
    SignalHandle signalhandle;
    parser = new PDFCmdPara(argc, argv);
    result = parser->GetParaFromFile();
    if (result) {
      printf("pdfservice   %s Compiled %s %s\n%s\n", __MY__VERSION__,  __DATE__, __TIME__, result);
      exit(1);
    }

    result = LogParser::Initialize(parser->logLevel, parser->logPath);
    if (result) {
      printf("pdfservice   %s Compiled %s %s\n%s\n", __MY__VERSION__,  __DATE__, __TIME__, result);
      exit(1);
    }
    
    pdfprocess = new PDFProcess(parser->lowMemory);
    helper = new Helper(this);
    ipcserver = new IPCServer(parser->ipcName, parser->processNumber, parser, pdfprocess);
    pdfdispatch = new PDFDispatch(ipcserver, parser->processNumber, parser->idleTimeout, helper);
    ownbufferepoll = new OwnBufferEpoll(parser->epollBufferCount, EPOLL_BUFFER_MAX_SIZE);
    epollservice = new WriteSeparateEpoll(ownbufferepoll, pdfdispatch, parser->idleWait);

    return 0;
  }
  unsigned long DoProcess(void) {
    epollservice->BeginListen(parser->listenPort);
    return 0;
  }
};

char *Helper::ReturnHelper(void) {
  snprintf(statusBuffer, MAX_VALID_COMMAND_LENGTH, "<html>Free Connect:%ld <br>Opened File:%ld</html>",
	   service->ownbufferepoll->FreeNumber(), service->pdfdispatch->HashmapNumber());
  return statusBuffer;
}

int main(int argc, char **argv) {
  PDFService service;

  service.Initialize(argc, argv);
  service.DoProcess();

  return 0;
}
