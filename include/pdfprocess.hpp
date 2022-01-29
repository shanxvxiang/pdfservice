
#ifndef __RAYMON_SHAN_PDF_PROCESS_H
#define __RAYMON_SHAN_PDF_PROCESS_H

#include <stdio.h>
#include <unistd.h>

#include "raymoncommon.hpp"
#include "raymonlog.hpp"
#include "raymontest.hpp"
#include "raymonfile.hpp"
#include "raymonfdfs.hpp"
#include "raymonpdfium.hpp"
#include "raymonjpeg.hpp"
#include "../include/std/hash_map_string"

#define INIT_PDFHANDLE_NUMBER 200

typedef class __gnu_cxx::hash_map<std::string, pPDFDocInfo> HashmapPDF, *pHashmapPDF;
typedef HashmapPDF::iterator iteratorPDF;

//#define CMD_PARA_MAX_PROTOCOL_NUMBER 16
//#define CMD_PARA_MAX_PROTOCOL_LENGTH 512



typedef class InterfaceProcess {
  
public:
  InterfaceProcess() {};
  ~InterfaceProcess() {};
  
  virtual unsigned long Initialize(void* para) = 0;
  virtual unsigned long Process(void *para) = 0;
} InterfaceProcess, *pInterfaceProcess;

typedef class PDFProcess : public InterfaceProcess {
private:
  pFileProcess fileprocess[CMD_PARA_MAX_PROTOCOL_NUMBER];
  unsigned long nowprocess;
  unsigned long lowmemory;

  FPDF_LIBRARY_CONFIG config;

  //  PDFium pdfium;
  pHashmapPDF hashmap;
  
public:
  PDFProcess(unsigned long lowmemory) {
    nowprocess = 0;
    this->lowmemory = lowmemory * 1024 * 1024;   // MB to byte
    hashmap = new HashmapPDF(INIT_PDFHANDLE_NUMBER);

    memset(&config, 0, sizeof(config));
    config.version = 3;
    FPDF_InitLibraryWithConfig(&config);    
  }
  ~PDFProcess() {
    FPDF_DestroyLibrary();
  }

  // return match protocol -1 for not found
  int GetProtocol(char *protocol) {
    for (unsigned long i = 0; i < nowprocess; i++) {
      if (!fileprocess[i]->CmpProtocol(protocol)) return i;
    }
    return -1;
  }

  // find in hashmap or create new
  char *ReturnDocInfo(char *protocol, char *filename, pPDFDocInfo *docinfo) {
    int protocolnum = GetProtocol(protocol);
    if (protocolnum == -1) return ERROR_INVALID_PROTOCOL;

    std::string hashkey;
    iteratorPDF finded;
    char *result;
    char *filebuffer = 0;
    long filesize = 0;

    *docinfo = 0;
    hashkey = protocol;
    hashkey += filename;
    finded = hashmap->find(hashkey);  // finded is iterator
    if (finded == hashmap->end()) {
       result = fileprocess[protocolnum]->GetFile(filename, &filebuffer, &filesize);
       if (result) return result;

       if (filesize < 1024) {
	 return (char*)"file too small < 1024";
       }
       *docinfo = new PDFDocInfo;
       (*docinfo)->OpenPDF(filebuffer, filesize);

       hashmap->insert(std::pair<std::string, pPDFDocInfo>(hashkey, *docinfo));
       return NULL;
    } else {
      _LOG_TRAC("found handle in hashmap, file %s:%s", protocol, filename);
      *docinfo = finded->second;
      return NULL;
    }
  }

  char *CloseDocInfo(char *protocol, char *filename) {
    std::string hashkey;
    iteratorPDF finded;
    pPDFDocInfo docinfo;    

    if (strcmp(protocol, PROTOCOL_CLOSE)) {
      hashkey = protocol;
      hashkey += filename;
    } else {
      hashkey = filename;
    }
    finded = hashmap->find(hashkey);  // finded is iterator
    if (finded == hashmap->end()) {
      return (char*)"empty file for close";
    } else {
      docinfo = finded->second;
      hashmap->erase(finded);      
      docinfo->ClosePDF();
      delete docinfo;
    }
    return NULL;
  }
  
  // Initialize will be call multi times for every protocol
  virtual unsigned long Initialize(void* para) {
    char *protocol = (char*)para;
    do {
      if (!strncmp(protocol, PROTOCOL_FILE, sizeof(PROTOCOL_FILE) - 1)) {
	fileprocess[nowprocess] = new HostFileProcess();
	break;
      }
      if (!strncmp(protocol, PROTOCOL_FASTDFS, sizeof(PROTOCOL_FASTDFS) - 1)) {
	fileprocess[nowprocess] = new FastdfsProcess();
	break;
      }
      _LOG_ATTE("HALT in protocol parser, unkonwn protocol: %s", protocol);
      return 1;
    } while(0);

    char *result = fileprocess[nowprocess]->Initialize(protocol);
    if (result) {
      _LOG_ATTE("HALT in protocol parser, %s: %s", result, protocol);
      return 1;
    }
    nowprocess++;
    return 0;
  }
  
  virtual unsigned long Process(void *para) {
    pPDFIPCMsg msgbuffer = pPDFIPCMsg(para);
    char headbuffer[MAX_VALID_COMMAND_LENGTH];
    char errorbuffer[MAX_VALID_COMMAND_LENGTH];
    int headsize, errorsize;
    
    char *result = 0;
    char *rgbxbuffer = 0;
    unsigned char *jpegbuffer = 0;
    unsigned long jpegsize = 0;
    int width, height;

    _LOG_INFO("processing file protocol:%s, filename:%s, page:%d", msgbuffer->protocol, msgbuffer->filename, msgbuffer->page);
    // printf("process: processing file protocol:%s, filename:%s, page:%d\n", msgbuffer->protocol, msgbuffer->filename, msgbuffer->page);
    pPDFDocInfo docinfo;

    do {
      if (msgbuffer->page < 0) {
	result = CloseDocInfo(msgbuffer->protocol, msgbuffer->filename);  // here filename is protocol + filename
	if (!result) {
	  _LOG_INFO("pdfium closed document, protocol:%s, filename:%s ", msgbuffer->protocol, msgbuffer->filename);
	} 
	if (!strcmp(msgbuffer->protocol, PROTOCOL_CLOSE)) {  // close by dispatch, no return to user
	  return 0;
	}
	break;
      }
      if (GetMeminfo() < lowmemory) {
	result = ERROR_LOW_MEMORY;
	break;
      }
      result = ReturnDocInfo(msgbuffer->protocol, msgbuffer->filename, &docinfo);   // read file to docinfo.buffer and loadpdf to docinfo.doc ...
      if (result) break;

      result = docinfo->OpenPage(msgbuffer->page, &rgbxbuffer, &width, &height, msgbuffer->scale);
      if (result) break;
      result = RGBX2Jpeg(width, height, rgbxbuffer, &jpegbuffer, &jpegsize, msgbuffer->quality);
      if (result) break;

      headsize = snprintf(headbuffer, 100, HTTP200JPEG, jpegsize);
      if (WriteFinish(msgbuffer->socketFd, headbuffer, headsize)) {
	close(msgbuffer->socketFd);
	_LOG_EROR("write error, write socket %d", msgbuffer->socketFd);
      } else if (WriteFinish(msgbuffer->socketFd, (char*)jpegbuffer, jpegsize)) {
	close(msgbuffer->socketFd);
	_LOG_EROR("write error, write socket %d", msgbuffer->socketFd);	
      } else {
	_LOG_INFO("processing file OK file size %ld", jpegsize);
      }

      if (rgbxbuffer) free(rgbxbuffer);
      if (jpegbuffer) free(jpegbuffer);
       return 0;      

    } while (0);

    if (result) {
      _LOG_EROR("%s, protocol:%s, filename:%s, page:%d", result, msgbuffer->protocol, msgbuffer->filename, msgbuffer->page);
    } else {
      _LOG_INFO("file closed, protocol:%s, filename:%s", msgbuffer->protocol, msgbuffer->filename);
      result = (char*)"file closed ";
    }
    errorsize = snprintf(errorbuffer, MAX_VALID_COMMAND_LENGTH, "%s protocol:%s, filename:%s, page:%d",
			 result, msgbuffer->protocol, msgbuffer->filename, msgbuffer->page);
    headsize = snprintf(headbuffer, MAX_VALID_COMMAND_LENGTH, HTTP500, errorsize);
    if (WriteFinish(msgbuffer->socketFd, headbuffer, headsize)) {
      _LOG_EROR("write error, write socket %d", msgbuffer->socketFd);
    } else if (WriteFinish(msgbuffer->socketFd, errorbuffer, errorsize)) {
      _LOG_EROR("write error, write socket %d", msgbuffer->socketFd);
    }
    if (rgbxbuffer) free(rgbxbuffer);
    if (jpegbuffer) free(jpegbuffer);
    return 0;
  }

} PDFProcess, *pPDFProcess;

#endif  // __RAYMON_SHAN_PDF_PROCESS_H
