
#ifndef __RAYMON_SHAN_COMMON_H
#define __RAYMON_SHAN_COMMON_H

#include <stdint.h>
#include <arpa/inet.h> 
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>

#define __MY__VERSION__         "V0.6"

#define _TOSTRING(x)            #x
#define TOSTRING(x)             _TOSTRING(x)
#define _JOIN(x,y)              x ## y
#define JOIN(x,y)               _JOIN(x,y)

typedef union {
  long l_value;
  long *l_addr;
  void *v_addr;
  char *p_addr;
  char **pp_addr;
}ADDR, *PADDR;

#define CACHE_LINE_SIZE         64

#define CMD_PARA_MAX_KEY_LENGTH 15
#define CMD_PARA_MAX_PARA_NUMBER 128

typedef struct OnePara {
  char paraType;
  const char *paraKey;
  unsigned long paraKeyLen;
  const char *paraDesc;
  ADDR paraValue;
  ADDR paraValue1;
} OnePara, *pOnePara;


char ERROR_LOW_MEMORY[] = "free memory low, request cancel";

char ERROR_WITHOUT_EQUAL[] = "without \'=\' in protocol define";
char ERROR_WITHOUT_SLASH[] = "without / at the end of protocol define";
char ERROR_NOT_DIRECTORY[] = "is not a valid directory";
char ERROR_PARENT_DIRECTORY[] = "../ is not permit in filename";
char ERROR_IN_OPENFILE[] = "error in open file";
char ERROR_IN_MALLOC[] = "error in malloc, no memory";
char ERROR_IN_READFILE[] = "error in read file";

char ERROR_TRACKER_PARSER[] = "error in parser tracker list";
char ERROR_TRACKER_CONNECT[] = "error in connect tracker";
char ERROR_STORAGE_CONNECT[] = "error in connect storage";
char ERROR_FASTDFS_DOWNLOAD[] = "error in download from fastdfs";

char ERROR_UNKONW_CMD[] = "unkonwn command or command length invalid";
char ERROR_INVALID_PARA[] = "invalid parameter scale & quality";
char ERROR_OPEN_PDF[] = "invalid pdf file";
char ERROR_OPEN_PDF_HANDLE[] = "invalid pdf file formhandle";
char ERROR_INVALID_PAGE[] = "out of page range";
char ERROR_OPEN_PAGE[] = "invalid pdf page";
char ERROR_CLOSE_PDF[] = "error in close pdf";

char ERROR_INVALID_PROTOCOL[] = "ERROR in protocol parser, unkonwn protocol";

char ERROR_CMD_FORMAT[] = "ERROR in command line format \n ./pdfservice config.file";

#define PROTOCOL_FILE    "file"
#define PROTOCOL_FASTDFS "fastdfs"
#define PROTOCOL_CLOSE   "CLOSE"               // for send close pdf doc to process
#define PROTOCOL_INITIALIZE "INITIALIZE"       // used when clone process, first ipc msg for read

//  char HTTP200PDF[] = "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: application/pdf\r\n\r\n";
char HTTP200[] = "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n";
char HTTP200JPEG[] = "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: image/jpeg\r\n\r\n";
char HTTP400[] = "HTTP/1.1 400 Bad Request\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n";
char HTTP404[] = "HTTP/1.1 404 File Not Found\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n";
char HTTP500[] = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n";


#define EPOLL_BUFFER_MAX_SIZE  (1024 * 10)

// if bufferBegin >= bufferSize -1, the socketFd will be CLOSED
// the malloc and free of bufferPointer should be done by UgetPPER program
typedef struct EpollBuffer {
  int socketFd;
  long timeout;                         // NOT USE yet
  char *bufferPointer;                  // pointer to real buffer for read
  unsigned long bufferSize;             // total size of buffer  
  unsigned long bufferBegin;            // valid buffer is buffer[bufferBegin] to buffer[EPOLL_BUFFER_MAX_SIZE - 1]
  unsigned long bufferEnd;              // readed in buffer[bufferBegin] to buffer[bufferEnd]
} EpollBuffer, *pEpollBuffer;

#define INIT_HANDLE_NUMBER  1000
#define MAX_VALID_COMMAND_LENGTH 256
#define MIN_VALID_COMMAND_LENGTH 16
#define MAX_VALID_PROTOCOL_LENGTH 16
#define MAX_VALID_ADDRESS_LENGTH  16


typedef struct PDFIPCMsg {
  long mType;                                    // = processid , 1 base
  int socketFd;                                  // client socket
  int page;                                      // page number ( -1 for close the file)
  char protocol[MAX_VALID_PROTOCOL_LENGTH];      // fastdfs , ipfs ...
  int scale;                                     // scale size default=100
  int quality;                                   // jpeg quality 1-95, default=75
  char filename[MAX_VALID_COMMAND_LENGTH];       // fastdfs filename 
} PDFIPCMsg, *pPDFIPCMsg;


#define IPC_FILENAME_MAX     32
// #define IPC_MSG_MAX_LENGTH   256    // should >= sizeof(PDFIPCMsg) - sizeof(long)
#define IPC_MSQ_PERMS        0666
#define IPC_PROCESS_STACK_SIZE (1024 * 1024 * 32)

class PDFCmdPara;
class LogParser;
class InterfaceProcess;
typedef struct IPCProcessPara {
  char ipcName[IPC_FILENAME_MAX];
  long ipcType;                    // only is the sequence of process
  //int (*ipcprocess)(void*);
  pid_t ipcPid;
  PDFCmdPara *cmdparser;
  LogParser *logparser;
  InterfaceProcess *process;
} IPCProcessPara, *pIPCProcessPara;

// used in cmdpara and pdfprocess
#define CMD_PARA_MAX_PROTOCOL_NUMBER 16
#define CMD_PARA_MAX_PROTOCOL_LENGTH 512


// URL COMMAND DEFINE
#define HTTP_MASK " HTTP/1."
#define HTTP_GET  "GET "
#define SERVICE_STATUS     "/pdfstat.php?"
#define SERVICE_FUNCTION   "/pdf2jpg.php?"

#define REQUEST_PROTOCOL   "protocol="
#define REQUEST_FILE       "file="
#define REQUEST_PAGE       "page="
#define REQUEST_SCALE      "scale="
#define REQUEST_QUALITY    "quality="

#define REQUEST_CLOSE     1
#define REQUEST_CONTINUE  2
#define REQUEST_COMPLETE  0



#endif  // __RAYMON_SHAN_COMMON_H
