
#ifndef __RAYMON_SHAN_FASTDFS_H
#define __RAYMON_SHAN_FASTDFS_H

#include "raymonlog.hpp"
#include "raymonfile.hpp"
#include "fdfs_client.h"

class FastdfsProcess : public FileProcess {
private:
  const short defaultport = 22122;
  char trackerlist[CMD_PARA_MAX_PROTOCOL_LENGTH];
  
public:
  virtual char *Initialize(char *name) {
    char *result = FileProcess::Initialize(name);
    if (result) return result;    

    unsigned long trackernow = 0;
    char *now, *lastnow, *end, *portplace;
    ConnectionInfo *pTrackerServer = NULL;
    // ConnectionInfo *pStorageServer = NULL;
    // int fdfsresult;

    g_log_context.log_level = LOG_EMERG;     // DO NOT LOGGER for fastdfs, for not config path for fastdfs log
    signal(SIGPIPE, SIG_IGN);                // ignore pipe, for not exit process when send to CLOSED socket

    now = lastnow = protocolAttr;
    end = now + strlen(now);
    for (; now <= end; now++) {
      if (*now == ',' || now == end) {
	*now = 0;
	if ((portplace = strchr(lastnow, ':')) == 0) {
	  trackernow += snprintf(&trackerlist[trackernow], CMD_PARA_MAX_PROTOCOL_LENGTH - trackernow - 1,
				 "tracker_server=%s:%d\r\n", lastnow, defaultport);
	} else {
	  trackernow += snprintf(&trackerlist[trackernow], CMD_PARA_MAX_PROTOCOL_LENGTH - trackernow - 1,
				 "tracker_server=%s\r\n", lastnow); 
	}
	lastnow = now + 1;
      }
    }
    if (trackernow >= CMD_PARA_MAX_PROTOCOL_LENGTH - 1) return ERROR_TRACKER_PARSER;

    fdfs_client_init_from_buffer(trackerlist);
    pTrackerServer = tracker_get_connection();
    if (pTrackerServer == NULL) {
      fdfs_client_destroy();
      return ERROR_TRACKER_CONNECT;
    }
    // pStorageServer = tracker_make_connection(pTrackerServer, &fdfsresult);
    // if (pStorageServer == NULL) {
    //  tracker_close_connection_ex(pTrackerServer, true);
    //  fdfs_client_destroy();
    //  return ERROR_TRACKER_CONNECT;
    //}
    tracker_close_all_connections();
    // tracker_close_connection_ex(pStorageServer, true);
    return 0;
  }
  
  virtual char *GetFile(char *filename, char** filebuffer, long *filesize) {
    static thread_local ConnectionInfo *pTrackerServer = NULL;
    // static thread_local ConnectionInfo *pStorageServer = NULL;    
    int result;
       
    *filebuffer = 0;
    if (pTrackerServer == NULL) pTrackerServer = tracker_get_connection();
    if (pTrackerServer == NULL) return ERROR_TRACKER_CONNECT;

    // if (pStorageServer == NULL) pStorageServer = tracker_make_connection(pTrackerServer, &result);
    // if (pStorageServer == NULL) return ERROR_STORAGE_CONNECT;

    result = storage_do_download_file1_ex(pTrackerServer, NULL, FDFS_DOWNLOAD_TO_BUFF,
					  filename, 0, 0, filebuffer, NULL, filesize);
    if (result != 0) {
      _LOG_EROR("ERROR in get file %s from fastdfs", filename);
      tracker_close_all_connections();
      // tracker_close_connection_ex(pStorageServer, true);
      pTrackerServer = NULL;
      // pStorageServer = NULL;
      *filebuffer = 0;
      filesize = 0;
      return ERROR_FASTDFS_DOWNLOAD;
    }
    _LOG_INFO("get file %s from fastdfs", filename);
    return 0;
  }  
};


#endif  // __RAYMON_SHAN_FASTDFS_H
