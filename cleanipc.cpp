
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/msg.h>

#include <malloc.h>
#include "include/raymoncommon.hpp"

int main (int argc, char **argv) {
  char defaultipcname[] = "pdfservice.ipc";
  char *ipcname = defaultipcname;
  int readmsqid;
  key_t readkey;

  char *msgbuffer;
  msgbuffer = (char*)malloc(1024*1024);
  int ret;

  if (argc == 2) ipcname = argv[1];
  if ((readkey = ftok(ipcname, 'B')) == -1 || (readmsqid = msgget(readkey, IPC_MSQ_PERMS)) == -1) {
    printf("ERROR in create ipc\n");
  }

  do {
    ret = msgrcv(readmsqid, msgbuffer, 1024*1023, 0, IPC_NOWAIT);
    printf("recv msg %d byte\n", ret);
  } while (ret != -1);

  msgctl(readmsqid, IPC_RMID, NULL);
  
  return 0;
}  
