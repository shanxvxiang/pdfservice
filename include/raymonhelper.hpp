
#ifndef __RAYMON_SHAN_HELPER_H
#define __RAYMON_SHAN_HELPER_H

#include "raymoncommon.hpp"

class PDFService;
typedef class Helper {
  PDFService *service;
  char statusBuffer[MAX_VALID_COMMAND_LENGTH];
public:

  Helper(PDFService* service) {
    this->service = service;
  };
  char *ReturnHelper(void);  
} Helper, *pHelper;


#endif  // __RAYMON_SHAN_HELPER_H
