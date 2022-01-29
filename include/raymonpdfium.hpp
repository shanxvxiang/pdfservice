
#ifndef __RAYMON_SHAN_PDFIUM_H
#define __RAYMON_SHAN_PDFIUM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "raymoncommon.hpp"
#include "raymonlog.hpp"
#include "../include/pdfium/fpdfview.h"
#include "../include/pdfium/fpdf_formfill.h"

// https://pdfium-review.googlesource.com/c/pdfium/+/76290/3/samples/simple_no_v8.cc#16
// step for pdfium

typedef class PDFDocInfo {
public:
  FPDF_DOCUMENT doc;
  FPDF_FORMHANDLE formhandle;
  FPDF_FORMFILLINFO forminfo;  
  void* buffer;
  int pagecount;

public:
  char *OpenPDF(char* filebuffer, long filesize) {
    doc = FPDF_LoadMemDocument64(filebuffer, filesize, NULL);
    if (!doc) return ERROR_OPEN_PDF;
    sleep(0);
    memset(&forminfo, 0, sizeof(forminfo));
    forminfo.version = 1;
    formhandle = FPDFDOC_InitFormFillEnvironment(doc, &forminfo);
    if (!formhandle) return ERROR_OPEN_PDF_HANDLE;
    buffer = filebuffer;
    pagecount = FPDF_GetPageCount(doc);
    return NULL;
  }

  char *OpenPage(int pageindex, char **rgbxbuffer, int *iwidth, int *iheight, int scale) {
    FPDF_PAGE page = NULL;
    FPDF_BITMAP bitmap = NULL;
    
    if (pageindex >= pagecount || pageindex < 0) return ERROR_INVALID_PAGE;
      
    double width = 0, height = 0;
    if (!FPDF_GetPageSizeByIndex(doc, pageindex, &width, &height)) return ERROR_INVALID_PAGE;
    *iwidth = (int)(width * scale / 100);
    *iheight = (int)(height * scale / 100);

    page = FPDF_LoadPage(doc, pageindex);
    if (!page) return ERROR_OPEN_PAGE;
    FORM_OnAfterLoadPage(page, formhandle);
    FORM_DoPageAAction(page, formhandle, FPDFPAGE_AACTION_OPEN);

    *rgbxbuffer = (char*)malloc( (*iwidth) * (*iheight) * 4);
    if (!(*rgbxbuffer)) {
      if (page) FPDF_ClosePage(page);
      return ERROR_IN_MALLOC;
    }
    bitmap = FPDFBitmap_CreateEx(*iwidth, *iheight, FPDFBitmap_BGRA, *rgbxbuffer, (*iwidth) * 4);
    FPDFBitmap_FillRect(bitmap, 0, 0, *iwidth, *iheight, 0xFFFFFFFF);

    FPDF_RenderPageBitmap(bitmap, page, 0, 0, *iwidth, *iheight, 0, FPDF_ANNOT);
    FPDF_FFLDraw(formhandle, bitmap, page, 0, 0, *iwidth, *iheight, 0, 0);

    FPDFBitmap_Destroy(bitmap);

    FORM_DoPageAAction(page, formhandle, FPDFPAGE_AACTION_CLOSE);
    FORM_OnBeforeClosePage(page, formhandle);
    FPDF_ClosePage(page);
    _LOG_TRAC("pdfium translate to bitmap ok, width:%d height:%d", *iwidth, *iheight);
    return NULL;;
    
  }
  
  char *ClosePDF() {
    if (formhandle) {
      FORM_DoDocumentAAction(formhandle, FPDFDOC_AACTION_WC);      
      FPDFDOC_ExitFormFillEnvironment(formhandle);
    }
    if (doc) FPDF_CloseDocument(doc);
    if (buffer) free(buffer);
    return NULL;
  } 
  
    
} PDFDocInfo, *pPDFDocInfo;

#endif  // __RAYMON_SHAN_PDFIUM_H


