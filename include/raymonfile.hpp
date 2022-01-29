
#ifndef __RAYMON_SHAN_FILE_H
#define __RAYMON_SHAN_FILE_H

#include <fcntl.h>
#include <malloc.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "raymoncommon.hpp"
#include "raymontest.hpp"

// for error, create the message and return char* for the message to caller.
// caller DO LOG and error control. 

typedef class FileProcess {
protected:
  char *protocolName;
  char *protocolAttr;
  char *newname;
  
public:
  FileProcess() {
    protocolName = protocolAttr = 0;
  }
  ~FileProcess() {
    if (newname) free(newname);
  }
  long CmpProtocol(char *protocol) {
    if (protocolName) return strncmp(protocol, protocolName, MAX_VALID_PROTOCOL_LENGTH);
    else return -1;
  }
  virtual char *Initialize(char *name) {
    newname = strdup(name);
    if ((protocolAttr = strchr(newname, '=')) == 0) {
      if (newname) free(newname);
      newname = 0;
      return ERROR_WITHOUT_EQUAL;
    }
    protocolName = newname;
    *protocolAttr = 0;
    protocolAttr++;
    return 0;
  }
  virtual char *GetFile(char *filename, char** filebuffer, long *filesize) = 0;
  
} FileProcess, *pFileProcess;

typedef class HostFileProcess : public FileProcess {
public:

  virtual char *Initialize(char *name) {
    char *result = FileProcess::Initialize(name);
    if (result) return result;

    if (name[strlen(name) - 1] != '/') return ERROR_WITHOUT_SLASH;    
    struct stat statbuffer;
    if (!(stat(protocolAttr, &statbuffer) == 0) || !S_ISDIR(statbuffer.st_mode)) {
      return ERROR_NOT_DIRECTORY;
    }
    return 0;
  }
  
  virtual char *GetFile(char *filename, char** filebuffer, long *filesize) {
    char fullname[CMD_PARA_MAX_PROTOCOL_LENGTH];
    //    struct stat statbuffer;
    //    int filehandle, fileread;

    *filebuffer = 0;
    if (strstr(filename, "../")) return ERROR_PARENT_DIRECTORY;
    
    snprintf(fullname, CMD_PARA_MAX_PROTOCOL_LENGTH, "%s%s", protocolAttr, filename);

    return LoadFileToBuffer(fullname, filebuffer, filesize);
    /*
    if (stat(fullname, &statbuffer)) return ERROR_IN_OPENFILE;
    *filesize = statbuffer.st_size;

    filehandle = open(fullname, O_RDONLY);
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
    */
  }
} HostFileProcess, *pHostFileProcess;

#endif  // __RAYMON_SHAN_FILE_H 

